#!/usr/bin/env python3
"""Prepare YX5200 playback files from normally named source audio without formatting disks."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
MANIFEST = "audio-manifest.json"
EXTENSIONS = {".mp3", ".wav", ".wma", ".flac", ".aac", ".m4a", ".ogg", ".opus",
              ".aif", ".aiff", ".caf", ".mp4", ".webm", ".mka", ".mov"}


def read_layout(config=ROOT / "include/Config.h"):
    text = config.read_text(encoding="utf-8")
    layout = {}
    for name in ("RewardTrack", "BootTrack", "ErrorTrack", "MaxTrack"):
        match = re.search(r"constexpr\s+(?:uint16_t|int)\s+" + name + r"\s*=\s*(\d+)\s*;", text)
        if not match:
            raise ValueError(f"Config::{name} must be a decimal integer literal for SD preparation")
        layout[name] = int(match[1])
    if not (0 < layout["RewardTrack"] < min(layout["BootTrack"], layout["ErrorTrack"]) and
            layout["BootTrack"] != layout["ErrorTrack"] and
            max(layout["BootTrack"], layout["ErrorTrack"]) <= layout["MaxTrack"]):
        raise ValueError("Invalid or overlapping music/system track IDs in Config.h")
    return layout


def collect_music(inputs):
    files = []
    for item in inputs:
        path = Path(item).expanduser().resolve(strict=True)
        if path.is_dir():
            files.extend(sorted((p for p in path.rglob("*") if p.is_file() and
                                 not any(part.startswith(".") for part in p.relative_to(path).parts) and
                                 p.suffix.lower() in EXTENSIONS), key=lambda p: str(p).casefold()))
        elif path.is_file():
            files.append(path)
        else:
            raise ValueError(f"Not a regular audio file or directory: {path}")
    return list(dict.fromkeys(files))


def ffmpeg_executable():
    executable = shutil.which("ffmpeg")
    if executable:
        return executable
    try:
        import imageio_ffmpeg
        return imageio_ffmpeg.get_ffmpeg_exe()
    except (ImportError, RuntimeError) as exc:
        raise ValueError("Install requirements-dev.txt or put ffmpeg on PATH") from exc


def convert(source, destination, ffmpeg):
    args = [ffmpeg, "-nostdin", "-hide_banner", "-loglevel", "error", "-xerror", "-n",
            "-i", str(source), "-map", "0:a:0", "-vn", "-map_metadata", "-1",
            "-ar", "44100", "-ac", "2", "-c:a", "libmp3lame", "-b:a", "128k",
            "-id3v2_version", "0", "-write_id3v1", "0", "-write_xing", "0", str(destination)]
    result = subprocess.run(args, capture_output=True, text=True)
    if result.returncode or not destination.is_file() or destination.stat().st_size == 0:
        raise ValueError(f"Cannot convert {source.name}: {result.stderr.strip()}")
    result = subprocess.run([ffmpeg, "-nostdin", "-hide_banner", "-loglevel", "error", "-xerror",
                             "-i", str(destination), "-f", "null", "-"], capture_output=True, text=True)
    if result.returncode:
        raise ValueError(f"Converted audio did not decode: {source.name}: {result.stderr.strip()}")


def digest(path):
    with path.open("rb") as file:
        return hashlib.file_digest(file, "sha256").hexdigest() if hasattr(hashlib, "file_digest") else _digest(file)


def _digest(file):
    value = hashlib.sha256()
    for block in iter(lambda: file.read(1024 * 1024), b""):
        value.update(block)
    return value.hexdigest()


def verify(destination, manifest):
    for entry in manifest["files"]:
        relative = Path(entry["path"])
        if relative.is_absolute() or ".." in relative.parts:
            raise ValueError("Unsafe path in audio manifest")
        path = destination / relative
        if any((destination / parent).is_symlink() for parent in [relative, *relative.parents]):
            raise ValueError(f"Symbolic links are not allowed in prepared audio: {path}")
        if path.is_symlink() or not path.is_file() or digest(path) != entry["sha256"]:
            raise ValueError(f"Verification failed: {path}")


def existing_files(output, replace):
    present = [name for name in ("MP3", "system", MANIFEST)
               if (output / name).exists() or (output / name).is_symlink()]
    if not present:
        return set()
    if not replace:
        raise ValueError(f"Destination already contains {present[0]}; use --replace for a previously prepared library")
    if set(present) != {"MP3", "system", MANIFEST} or any((output / name).is_symlink() for name in present):
        raise ValueError("Cannot replace a library without its complete, original manifest and directories")
    manifest = json.loads((output / MANIFEST).read_text(encoding="utf-8"))
    if manifest.get("format_version") != 1 or not isinstance(manifest.get("files"), list):
        raise ValueError("Unrecognized audio manifest")
    names = set()
    for entry in manifest["files"]:
        name = entry.get("path", "")
        if not re.fullmatch(r"MP3/\d{4}\.mp3|system/(?:boot|error)\.[a-z0-9]+", name):
            raise ValueError("Manifest contains an unmanaged path")
        names.add(name)
    verify(output, manifest)
    return names


def prepare(output, boot, error, music=(), ffmpeg=None, convert_fn=convert, replace=False):
    output = Path(output).expanduser().absolute()
    if output.is_symlink():
        raise ValueError("Output cannot be a symbolic link")
    output = output.resolve()
    if output == Path(output.anchor) or output == Path.home() or output == ROOT:
        raise ValueError("Output must be a dedicated SD volume or staging folder")
    layout = read_layout()
    sources = collect_music(music)
    limit = min(layout["BootTrack"], layout["ErrorTrack"])
    if layout["RewardTrack"] + len(sources) > limit:
        raise ValueError("Music files would overlap reserved system tracks")
    boot, error = Path(boot).expanduser().resolve(strict=True), Path(error).expanduser().resolve(strict=True)
    if not boot.is_file() or not error.is_file():
        raise ValueError("Boot and error sources must be regular files")
    previous = existing_files(output, replace)
    if output.exists() and not output.is_dir():
        raise ValueError("Output must be a directory")
    for source in [boot, error, *sources]:
        if source == output or output in source.parents:
            raise ValueError("Keep original audio outside the destination volume/folder")
    ffmpeg = ffmpeg or ffmpeg_executable()
    with tempfile.TemporaryDirectory(prefix="robot-audio-") as temporary:
        staged = Path(temporary)
        (staged / "MP3").mkdir()
        (staged / "system").mkdir()
        manifest = {"format_version": 1, "encoding": "MP3 CBR 128 kbps, 44100 Hz, stereo",
                    "tracks": [], "files": []}
        jobs = [("music", layout["RewardTrack"] + i, source) for i, source in enumerate(sources)]
        jobs += [("boot", layout["BootTrack"], boot), ("error", layout["ErrorTrack"], error)]
        for role, track, source in jobs:
            relative = f"MP3/{track:04d}.mp3"
            convert_fn(source, staged / relative, ffmpeg)
            manifest["tracks"].append({"role": role, "track": track, "source_name": source.name,
                                       "playback_file": relative})
            if role != "music":
                shutil.copyfile(source, staged / "system" / f"{role}{source.suffix.lower()}")
        for path in sorted(staged.rglob("*")):
            if path.is_file():
                manifest["files"].append({"path": path.relative_to(staged).as_posix(),
                                          "bytes": path.stat().st_size, "sha256": digest(path)})
        (staged / MANIFEST).write_text(json.dumps(manifest, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
        current = {entry["path"] for entry in manifest["files"]}
        for name in sorted(current):
            if ((output / name).exists() or (output / name).is_symlink()) and name not in previous:
                raise ValueError(f"Refusing to overwrite an unmanaged file: {name}")
        if output.exists() and shutil.disk_usage(output).free < sum(f["bytes"] for f in manifest["files"]) + 1024 * 1024:
            raise ValueError("Not enough free space for the prepared audio")
        output.mkdir(parents=True, exist_ok=True)
        for name in sorted(current):
            destination = output / name
            destination.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(staged / name, destination)
        for name in previous - current:
            (output / name).unlink()
        shutil.copyfile(staged / MANIFEST, output / MANIFEST)
        verify(output, manifest)
    return manifest


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True, help="Fresh SD volume or staging directory (never formatted by this script)")
    parser.add_argument("--boot", type=Path, required=True)
    parser.add_argument("--error", type=Path, required=True)
    parser.add_argument("--music", type=Path, nargs="*", default=[], help="Arbitrarily named audio files or folders; first imported track is the reward")
    parser.add_argument("--replace", action="store_true", help="Replace only files owned by a verified previous audio manifest; supply the entire desired music collection")
    args = parser.parse_args()
    try:
        manifest = prepare(args.output, args.boot, args.error, args.music, replace=args.replace)
    except (OSError, ValueError) as exc:
        parser.exit(1, f"Error: {exc}\n")
    for track in manifest["tracks"]:
        print(f"{track['role']:5s} {track['track']:4d}: {track['source_name']} -> {track['playback_file']}")
    if not args.music:
        print("No reward music supplied; system sounds only. Import music before testing a high-five reward.")
    print(f"Verified {len(manifest['files'])} files in {args.output}")


if __name__ == "__main__":
    main()

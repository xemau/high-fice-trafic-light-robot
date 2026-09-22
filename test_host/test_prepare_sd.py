import json
from pathlib import Path
import subprocess
import tempfile
import unittest
import wave

from scripts import prepare_sd as sd


class PreparationTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.output = self.root / "card"

    def prepare(self, music=(), **kwargs):
        return sd.prepare(self.output, music,
                          ffmpeg="fake-ffmpeg", convert_fn=kwargs.get("convert_fn", self.fake_conversion),
                          replace=kwargs.get("replace", False))

    @staticmethod
    def fake_conversion(source, target, ffmpeg):
        target.write_bytes(b"converted:" + source.read_bytes())

    def test_original_names_and_configured_track_ids(self):
        song = self.root / "My favourite song 日本語.wav"
        song.write_bytes(b"reward")
        manifest = self.prepare([song])
        self.assertEqual(manifest["tracks"][0]["source_name"], song.name)
        self.assertEqual([1], [track["track"] for track in manifest["tracks"]])
        self.assertFalse((self.output / "system").exists())
        self.assertEqual((self.output / "MP3/0001.mp3").read_bytes(), b"converted:reward")
        sd.verify(self.output, json.loads((self.output / sd.MANIFEST).read_text()))

    def test_folder_import_deduplicates_and_skips_metadata(self):
        folder = self.root / "songs"
        folder.mkdir()
        for name in ["z.WAV", "a.mp3", "._extra.mp3", "notes.txt"]:
            (folder / name).touch()
        (folder / ".hidden").mkdir()
        (folder / ".hidden/ignored.wav").touch()
        sources = sd.collect_music([folder, folder / "a.mp3"])
        self.assertEqual(["a.mp3", "z.WAV"], [p.name for p in sources])

    def test_failed_conversion_does_not_touch_card(self):
        def fail(source, target, ffmpeg):
            raise ValueError("Unsupported audio")
        song = self.root / "song.mp3"
        song.write_bytes(b"song")
        with self.assertRaisesRegex(ValueError, "Unsupported"):
            self.prepare([song], convert_fn=fail)
        self.assertFalse(self.output.exists())

    def test_existing_library_is_not_overwritten(self):
        song = self.root / "song.mp3"
        song.write_bytes(b"song")
        self.prepare([song])
        before = (self.output / "MP3/0001.mp3").read_bytes()
        with self.assertRaisesRegex(ValueError, "already contains"):
            self.prepare([song])
        self.assertEqual(before, (self.output / "MP3/0001.mp3").read_bytes())

    def test_explicit_replace_preserves_unmanaged_files_and_removes_stale_tracks(self):
        song = self.root / "old.wav"
        song.write_bytes(b"old song")
        self.prepare([song])
        legacy = ["MP3/2998.mp3", "MP3/2999.mp3", "system/boot.mp3", "system/error.mp3"]
        manifest_path = self.output / sd.MANIFEST
        manifest = json.loads(manifest_path.read_text())
        for name in legacy:
            path = self.output / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(name.encode())
            manifest["files"].append({"path": name, "bytes": path.stat().st_size,
                                      "sha256": sd.digest(path)})
        manifest_path.write_text(json.dumps(manifest))
        note = self.output / "notes.txt"
        note.write_text("keep")
        manifest = self.prepare(replace=True)
        self.assertFalse((self.output / "MP3/0001.mp3").exists())
        for name in legacy:
            self.assertFalse((self.output / name).exists())
        self.assertFalse((self.output / "system").exists())
        self.assertEqual("keep", note.read_text())
        sd.verify(self.output, manifest)
        (self.output / "MP3/0001.mp3").write_bytes(b"someone else's file")
        with self.assertRaisesRegex(ValueError, "unmanaged file"):
            self.prepare([song], replace=True)

    def test_replace_rejects_modified_files_and_tampered_manifest(self):
        song = self.root / "song.mp3"
        song.write_bytes(b"song")
        self.prepare([song])
        manifest_path = self.output / sd.MANIFEST
        manifest = json.loads(manifest_path.read_text())
        manifest["files"][0]["path"] = "../outside"
        manifest_path.write_text(json.dumps(manifest))
        with self.assertRaisesRegex(ValueError, "unmanaged path"):
            self.prepare(replace=True)

    def test_metadata_cleanup_is_limited_to_generated_files(self):
        song = self.root / "song.mp3"
        song.write_bytes(b"song")
        self.prepare([song])
        sidecar = self.output / "MP3/._0001.mp3"
        unrelated = self.output / "MP3/._other.mp3"
        sidecar.write_bytes(b"AppleDouble metadata")
        unrelated.write_bytes(b"keep")
        sd.clean_metadata(self.output, {"MP3/0001.mp3"})
        self.assertFalse(sidecar.exists())
        self.assertEqual(b"keep", unrelated.read_bytes())

    def test_output_symlink_and_sources_inside_destination_rejected(self):
        directory = self.root / "real-card"
        directory.mkdir()
        self.output.symlink_to(directory)
        with self.assertRaisesRegex(ValueError, "symbolic link"):
            self.prepare()
        self.output.unlink()
        self.output.mkdir()
        song = self.output / "song.wav"
        song.touch()
        with self.assertRaisesRegex(ValueError, "outside"):
            self.prepare([song])

    def test_missing_sources_and_tampered_outputs_rejected(self):
        with self.assertRaises(FileNotFoundError):
            self.prepare([self.root / "missing.wav"])
        song = self.root / "song.mp3"
        song.write_bytes(b"song")
        manifest = self.prepare([song])
        (self.output / "MP3/0001.mp3").write_bytes(b"corrupt")
        with self.assertRaisesRegex(ValueError, "Verification failed"):
            sd.verify(self.output, manifest)
        with self.assertRaisesRegex(ValueError, "Unsafe path"):
            sd.verify(self.output, {"files": [{"path": "../outside", "sha256": ""}]})

    def test_config_changes_are_read_and_invalid_expressions_rejected(self):
        config = self.root / "Config.h"
        config.write_text("constexpr uint16_t RewardTrack = 1;\nconstexpr int MaxTrack = 9999;\n")
        self.assertEqual(1, sd.read_layout(config)["RewardTrack"])
        config.write_text(config.read_text().replace("RewardTrack = 1", "RewardTrack = 10000"))
        with self.assertRaisesRegex(ValueError, "range"):
            sd.read_layout(config)
        config.write_text(config.read_text().replace("RewardTrack = 10000", "RewardTrack = 5 + 5"))
        with self.assertRaisesRegex(ValueError, "literal"):
            sd.read_layout(config)


class RealConversionTests(unittest.TestCase):
    def test_wav_mp3_flac_and_m4a_with_arbitrary_names(self):
        ffmpeg = sd.ffmpeg_executable()
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            wav = root / "A reward with spaces.wav"
            with wave.open(str(wav), "wb") as file:
                file.setnchannels(1)
                file.setsampwidth(2)
                file.setframerate(22050)
                file.writeframes(b"\x00\x00" * 4410)
            sources = [wav]
            for extension in ("mp3", "flac", "m4a"):
                path = root / f"another name.{extension}"
                subprocess.run([ffmpeg, "-nostdin", "-hide_banner", "-loglevel", "error", "-i", str(wav), str(path)], check=True)
                sources.append(path)
            output = root / "prepared"
            manifest = sd.prepare(output, sources, ffmpeg)
            self.assertEqual(4, len(manifest["tracks"]))
            self.assertFalse((output / "system").exists())
            sd.verify(output, manifest)
            invalid = root / "broken.mp3"
            invalid.write_bytes(b"This is not audio")
            with self.assertRaisesRegex(ValueError, "Cannot convert"):
                sd.prepare(root / "failed", [invalid], ffmpeg=ffmpeg)
            self.assertFalse((root / "failed").exists())


if __name__ == "__main__":
    unittest.main()

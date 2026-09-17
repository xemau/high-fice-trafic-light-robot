import json
import math
from pathlib import Path
import subprocess
import struct
import tempfile
import unittest
import wave

from scripts import prepare_sd as sd


class PreparationTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.boot = self.root / "boot.mp3"
        self.error = self.root / "error.mp3"
        self.boot.write_bytes(b"original boot")
        self.error.write_bytes(b"original error")
        self.output = self.root / "card"

    def prepare(self, music=(), **kwargs):
        return sd.prepare(self.output, self.boot, self.error, music,
                          ffmpeg="fake-ffmpeg", convert_fn=kwargs.get("convert_fn", self.fake_conversion),
                          replace=kwargs.get("replace", False), eq_config=kwargs.get("eq_config", sd.DEFAULT_EQ))

    @staticmethod
    def fake_conversion(source, target, ffmpeg, audio_filter=None):
        target.write_bytes(b"converted:" + source.read_bytes())

    def test_original_names_and_configured_system_ids(self):
        song = self.root / "My favourite song 日本語.wav"
        song.write_bytes(b"reward")
        manifest = self.prepare([song])
        self.assertEqual(manifest["tracks"][0]["source_name"], song.name)
        self.assertEqual([1, sd.read_layout()["BootTrack"], sd.read_layout()["ErrorTrack"]],
                         [track["track"] for track in manifest["tracks"]])
        self.assertEqual((self.output / "system/boot.mp3").read_bytes(), self.boot.read_bytes())
        self.assertEqual((self.output / "system/error.mp3").read_bytes(), self.error.read_bytes())
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
        with self.assertRaisesRegex(ValueError, "Unsupported"):
            self.prepare(convert_fn=fail)
        self.assertFalse(self.output.exists())

    def test_existing_library_is_not_overwritten(self):
        self.prepare()
        before = (self.output / "MP3/2998.mp3").read_bytes()
        with self.assertRaisesRegex(ValueError, "already contains"):
            self.prepare()
        self.assertEqual(before, (self.output / "MP3/2998.mp3").read_bytes())

    def test_explicit_replace_preserves_unmanaged_files_and_removes_stale_tracks(self):
        song = self.root / "old.wav"
        song.write_bytes(b"old song")
        self.prepare([song])
        note = self.output / "notes.txt"
        note.write_text("keep")
        manifest = self.prepare(replace=True)
        self.assertFalse((self.output / "MP3/0001.mp3").exists())
        self.assertEqual("keep", note.read_text())
        sd.verify(self.output, manifest)
        (self.output / "MP3/0001.mp3").write_bytes(b"someone else's file")
        with self.assertRaisesRegex(ValueError, "unmanaged file"):
            self.prepare([song], replace=True)

    def test_replace_rejects_modified_files_and_tampered_manifest(self):
        self.prepare()
        manifest_path = self.output / sd.MANIFEST
        manifest = json.loads(manifest_path.read_text())
        manifest["files"][0]["path"] = "../outside"
        manifest_path.write_text(json.dumps(manifest))
        with self.assertRaisesRegex(ValueError, "unmanaged path"):
            self.prepare(replace=True)

    def test_metadata_cleanup_is_limited_to_generated_files(self):
        self.prepare()
        sidecar = self.output / "MP3/._2998.mp3"
        unrelated = self.output / "MP3/._other.mp3"
        sidecar.write_bytes(b"AppleDouble metadata")
        unrelated.write_bytes(b"keep")
        sd.clean_metadata(self.output, {"MP3/2998.mp3"})
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
        manifest = self.prepare()
        (self.output / "MP3/2998.mp3").write_bytes(b"corrupt")
        with self.assertRaisesRegex(ValueError, "Verification failed"):
            sd.verify(self.output, manifest)
        with self.assertRaisesRegex(ValueError, "Unsafe path"):
            sd.verify(self.output, {"files": [{"path": "../outside", "sha256": ""}]})

    def test_config_changes_are_read_and_invalid_expressions_rejected(self):
        config = self.root / "Config.h"
        config.write_text("constexpr uint16_t RewardTrack = 1;\nconstexpr uint16_t BootTrack = 10;\n"
                          "constexpr uint16_t ErrorTrack = 11;\nconstexpr int MaxTrack = 9999;\n")
        self.assertEqual(10, sd.read_layout(config)["BootTrack"])
        config.write_text(config.read_text().replace("ErrorTrack = 11", "ErrorTrack = 10"))
        with self.assertRaisesRegex(ValueError, "overlapping"):
            sd.read_layout(config)
        config.write_text(config.read_text().replace("BootTrack = 10", "BootTrack = 5 + 5"))
        with self.assertRaisesRegex(ValueError, "literal"):
            sd.read_layout(config)

    def test_eq_is_music_only_recorded_and_can_be_disabled(self):
        song = self.root / "song.wav"
        song.write_bytes(b"reward")
        calls = []

        def record(source, target, ffmpeg, audio_filter=None):
            calls.append((source.name, audio_filter))
            self.fake_conversion(source, target, ffmpeg)

        manifest = self.prepare([song], convert_fn=record)
        self.assertIn("bass=f=120:g=-6", calls[0][1])
        self.assertEqual([None, None], [entry[1] for entry in calls[1:]])
        self.assertEqual([True, False, False], [entry["eq_applied"] for entry in manifest["tracks"]])
        self.assertEqual(sd.read_eq(sd.DEFAULT_EQ), manifest["music_eq"])
        self.assertEqual(calls[0][1], manifest["music_filter"])
        calls.clear()
        manifest = self.prepare([song], convert_fn=record, replace=True, eq_config=None)
        self.assertIsNone(manifest["music_filter"])
        self.assertTrue(all(value is None for _, value in calls))
        self.assertEqual(b"reward", song.read_bytes())

    def test_invalid_eq_fails_before_touching_destination(self):
        valid = sd.read_eq(sd.DEFAULT_EQ)
        cases = [[], {}, {**valid, "typo": 1}, {**valid, "enabled": 1},
                 {**valid, "bass_db": "-6"}, {**valid, "mids_db": float("nan")},
                 {**valid, "highpass_hz": float("inf")}, {**valid, "lows_q": 0},
                 {**valid, "bass_db": True}, {**valid, "mids_hz": 10 ** 1000},
                 {**valid, "bass_db": 6, "mids_db": 6}]
        path = self.root / "eq.json"
        for config in cases:
            path.write_text(json.dumps(config))
            with self.subTest(config=config), self.assertRaises(ValueError):
                self.prepare(eq_config=path)
            self.assertFalse(self.output.exists())
        path.write_text(json.dumps({**valid, "enabled": False}))
        self.assertIsNone(sd.eq_filter(sd.read_eq(path)))
        path.write_text(json.dumps({**valid, "bass_db": 6, "preamp_db": -6}))
        self.assertIn("g=6", sd.eq_filter(sd.read_eq(path)))


class RealConversionTests(unittest.TestCase):
    def test_music_eq_reduces_bass_relative_to_mids(self):
        ffmpeg = sd.ffmpeg_executable()
        frequencies = (100, 300, 1500)
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            source = root / "test tones.wav"
            samples = [int(4000 * sum(math.sin(2 * math.pi * f * i / 44100) for f in frequencies))
                       for i in range(44100)]
            with wave.open(str(source), "wb") as wav:
                wav.setnchannels(1)
                wav.setsampwidth(2)
                wav.setframerate(44100)
                wav.writeframes(struct.pack("<" + "h" * len(samples), *samples))
            original = sd.digest(source)
            measured = []
            for name, config in [("flat", None), ("eq", sd.DEFAULT_EQ)]:
                destination = root / name
                manifest = sd.prepare(destination, source, source, [source], ffmpeg=ffmpeg, eq_config=config)
                sd.verify(destination, manifest)
                result = subprocess.run([ffmpeg, "-v", "error", "-i", str(destination / "MP3/0001.mp3"),
                                         "-ac", "1", "-ar", "44100", "-f", "f32le", "-"],
                                        check=True, capture_output=True)
                decoded = struct.unpack("<" + "f" * (len(result.stdout) // 4), result.stdout)[8820:35280]
                amplitudes = []
                for frequency in frequencies:
                    real = sum(value * math.cos(2 * math.pi * frequency * i / 44100) for i, value in enumerate(decoded))
                    imaginary = sum(value * math.sin(2 * math.pi * frequency * i / 44100) for i, value in enumerate(decoded))
                    amplitudes.append(math.hypot(real, imaginary))
                measured.append(amplitudes)
            gains = [20 * math.log10(eq / flat) for eq, flat in zip(measured[1], measured[0])]
            self.assertLess(gains[0], gains[2] - 3)
            self.assertLess(gains[1], gains[2] - 1.5)
            self.assertAlmostEqual(-3, gains[2], delta=1)
            self.assertEqual(original, sd.digest(source))
            for name in ("2998.mp3", "2999.mp3"):
                self.assertEqual(sd.digest(root / "flat/MP3" / name), sd.digest(root / "eq/MP3" / name))

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
            manifest = sd.prepare(output, wav, sources[1], sources, ffmpeg)
            self.assertEqual(6, len(manifest["tracks"]))
            self.assertEqual(wav.read_bytes(), (output / "system/boot.wav").read_bytes())
            sd.verify(output, manifest)
            invalid = root / "broken.mp3"
            invalid.write_bytes(b"This is not audio")
            with self.assertRaisesRegex(ValueError, "Cannot convert"):
                sd.prepare(root / "failed", invalid, wav, ffmpeg=ffmpeg)
            self.assertFalse((root / "failed").exists())


if __name__ == "__main__":
    unittest.main()

# SD audio prepared on 2026-09-22

The Desktop `TEMPROBOT` staging folder contains 1,000 family-friendly music excerpts under `MP3/`, numbered continuously from `0001.mp3` through `1000.mp3`. The previous collections are preserved beside it as `TEMPROBOT-backup-before-75-songs`, `TEMPROBOT-backup-75-tracks`, and `TEMPROBOT-backup-150-tracks`.

Each excerpt is a metadata-free 44.1 kHz stereo, 128 kbps constant-bitrate MP3. All 1,000 files were decoded after final numbering; every decoded duration was 30.302 seconds. The installed copy was then checked against every size and SHA-256 value in its manifest. Its total MP3 payload is 484,832,000 bytes. Audio files are not committed to the repository.

`audio-manifest.json` is the complete track list. Each track record contains its playback number, artist, title, source page and channel, excerpt start and duration, selection method, and playback filename. Each file record contains its exact byte size and SHA-256 hash.

The collection starts with Sabrina Carpenter's “Espresso” as track 1. The original 150 tracks remain unchanged, a clean KIDZ BOP version of “Dance Monkey” begins the additions at track 151, and Electric Bloom's “My Beat My Drum” is track 1000. The added selection mixes clean pop covers, Disney and Pixar soundtracks, dance tracks, seasonal songs, international versions, and instrumental soundtrack cues. Obvious alternate-video duplicates and non-song material were removed before final numbering. The obsolete `MP3/2998.mp3`, `MP3/2999.mp3`, and `/system` directory are absent.

The staging folder must be copied to the FAT32 / MBR card named `ROBOT` before hardware playback. Copy both `MP3/` and `audio-manifest.json`, then verify the copied files before ejecting. Device numbers can change, so identify the card again before any formatting operation.

The YX5200 addresses these files with MP3-folder command `0x12`. Firmware selects a random number in 1–1000 and avoids an immediate repeat. The source filenames do not need numeric prefixes when using `scripts/prepare_sd.py`; the module itself cannot convert audio or open arbitrary paths.

Next hardware check: safely eject and insert the copied card with the YX5200 unpowered. Reset the uploaded firmware, wait for FULL to start and for `[OK] YX5200`, then issue several `play N` commands across the 1–1000 range before testing repeated physical high-fives. Actual module decoding and audible output remain unverified until that bench test.

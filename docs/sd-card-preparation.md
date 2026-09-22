# SD audio prepared on 2026-09-22

The Desktop `TEMPROBOT` staging folder contains 596 curated, family-friendly dance music excerpts under `MP3/`, numbered continuously from `0001.mp3` through `0596.mp3`. The unfiltered 1,000-track collection is preserved beside it as `TEMPROBOT-backup-1000-unfiltered`; earlier 3-, 75-, and 150-track collections are also preserved in their existing backup folders.

Each excerpt is a metadata-free 44.1 kHz stereo, 128 kbps constant-bitrate MP3. All 596 files were decoded after final numbering; every decoded duration was 30.302 seconds. The installed copy was then checked against every size and SHA-256 value in its manifest. Its total MP3 payload is 288,959,872 bytes. Audio files are not committed to the repository.

`audio-manifest.json` is the complete track list. Each track record contains its new playback number, original collection number, artist, title, source page and channel, excerpt start and duration, rhythm-analysis data, curation decision, and playback filename. Each file record contains its exact byte size and SHA-256 hash.

The collection starts with Sabrina Carpenter's “Espresso” as track 1. The original 150-track pop collection remains unchanged. The additions contain 371 clean KIDZ BOP recordings and 75 Disney vocal or cast recordings selected for sustained rhythm. KIDZ BOP music-video edits were excluded because they can contain spoken scenes; this removes the old track 205, whose excerpt contained children talking. Instrumental and cinematic soundtrack cues were also excluded, including the old track 190, “Bundle of Joy.” The retained tracks were renumbered continuously, ending with Electric Bloom's “My Beat My Drum” at track 596. The obsolete `MP3/2998.mp3`, `MP3/2999.mp3`, and `/system` directory are absent.

The staging folder must be copied to the FAT32 / MBR card named `ROBOT` before hardware playback. Copy both `MP3/` and `audio-manifest.json`, then verify the copied files before ejecting. Device numbers can change, so identify the card again before any formatting operation.

The YX5200 addresses these files with MP3-folder command `0x12`. Firmware selects a random number in 1–596 and avoids an immediate repeat. The source filenames do not need numeric prefixes when using `scripts/prepare_sd.py`; the module itself cannot convert audio or open arbitrary paths.

Next hardware check: safely eject and insert the copied card with the YX5200 unpowered. Reset the uploaded firmware, wait for FULL to start and for `[OK] YX5200`, then issue several `play N` commands across the 1–596 range before testing repeated physical high-fives. Actual module decoding and audible output remain unverified until that bench test.

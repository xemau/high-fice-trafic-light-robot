# SD card prepared on 2026-09-15, updated 2026-09-22

The removable 15.6 GB card was formatted with an MBR partition table and FAT32 filesystem, volume name `ROBOT`. Device numbers can change, so identify the card again before any future formatting operation.

The three reward tracks were converted to 44.1 kHz stereo, 128 kbps constant-bitrate MP3 and decoded to check the generated files. The source files in Downloads were not modified. Audio files are not committed to the repository.

| Role | Original source | Playback copy | Playback ID |
| --- | --- | --- | --- |
| Reward | Pufino - Rock Me Now (freetouse.com).mp3 | MP3/0001.mp3 | 1 |
| Reward | Aetheric - Snap Crackle (freetouse.com).mp3 | MP3/0002.mp3 | 2 |
| Reward | Moavii - Root (freetouse.com).mp3 | MP3/0003.mp3 | 3 |

The card's `audio-manifest.json` records these three files with sizes and SHA-256 hashes. All hashes were verified by reading back the card. The obsolete `MP3/2998.mp3`, `MP3/2999.mp3`, and `/system` directory were removed on 2026-09-22.

`diskutil verifyVolume` completed with filesystem check exit code 0 after the update.

| File | SHA-256 |
| --- | --- |
| MP3/0001.mp3 | 6db7f132eb40ab2ce7322c0efd0117623f87defccd471e848d779b13b382a967 |
| MP3/0002.mp3 | 20f5a34d85572eda7c0cbf690d9ab26e32e33dcc047053e1374ad3eb69cae97d |
| MP3/0003.mp3 | 48630da5120067a16e0b8f42a179531ef57dd6a5c6dc4a546404178d44320019 |

The source filenames do not need numeric prefixes. `scripts/prepare_sd.py` performs conversion and numeric addressing automatically. The YX5200 cannot itself convert arbitrary audio formats or open arbitrary paths.

Next hardware check: safely eject and insert the card with the YX5200 unpowered. Reset the uploaded firmware, wait for FULL to start and for `[OK] YX5200`, then optionally issue `play 1`, `play 2`, and `play 3` before testing a physical high-five. Actual module decoding and audible output remain unverified until that bench test.

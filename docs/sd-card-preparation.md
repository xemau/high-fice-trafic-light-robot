# SD card prepared on 2026-09-15

The removable 15.6 GB card in the built-in Secure Digital reader was identified as `/dev/disk12` at preparation time and formatted with an MBR partition table and FAT32 filesystem, volume name `ROBOT`. Device numbers can change: identify the card again before any future formatting operation.

All audio was converted to 44.1 kHz stereo, 128 kbps constant-bitrate MP3 and decoded again to check the generated files. `/system` contains byte-identical copies of the supplied system MP3s. The source WAV and MP3s in Downloads were not modified. Audio files are not committed to the repository.

| Role | Original source | Playback copy | Playback ID |
| --- | --- | --- | --- |
| Temporary reward | high-enough.wav | MP3/0001.mp3 | 1 |
| Boot | system/boot.mp3 | MP3/2998.mp3 | 2998 |
| Error | system/error.mp3 | MP3/2999.mp3 | 2999 |

The card's `audio-manifest.json` records the five files with sizes and SHA-256 hashes. All five hashes were verified by reading back the card. The original boot/error copies were also compared byte-for-byte against the supplied files.

`diskutil verifyVolume` completed with filesystem check exit code 0. macOS-created AppleDouble `._` audio sidecars were removed from the playback and system directories.

| File | SHA-256 |
| --- | --- |
| MP3/0001.mp3 | a3724f8d8a4048a173b7fd4aa33bba6dca78b71a4a3b5b59777cc596e6bc1cab |
| MP3/2998.mp3 | 4cdc96fdb4c37e41b3a5326cd56c81b044635a965e7e1e16821e719f738a6ecb |
| MP3/2999.mp3 | c4fb0627292551d5d4cddcef2a7516dc0599655486cccc2f790e2cf4efdfa0a0 |
| system/boot.mp3 | 42f6807a4ca04dac647ae192de5b0d04483af07fb510b6be72fa832c8e74cfa0 |
| system/error.mp3 | 3ec21ad945f8496b48d287eb8a4b5291e9f76f08061f3096c178f932eca37195 |

The user-facing filenames do not need numeric prefixes. `scripts/prepare_sd.py` performs conversion and numeric addressing automatically. The YX5200 cannot itself convert arbitrary audio formats or open arbitrary paths. The `/system` copies are source assets; the firmware plays their numbered MP3 copies.

Next hardware check: insert the card with the YX5200 unpowered, then run AUDIO TEST and issue `boot`, `error`, and `play 1`. Updated firmware is required for the new automatic boot/error behavior. Actual module decoding and audible output remain unverified until that bench test.

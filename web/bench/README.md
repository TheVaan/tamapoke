# Hardware test bench

`index.html` installs a pull request's firmware on a real board and checks it
over USB (Web Serial — Chrome or Edge on a computer). It is a testing tool, not
the player-facing installer: that is `web/index.html`, and nothing here touches
it.

## What it does

1. **Connect** to the board.
2. **Back up the save** with `EXPORT` (downloadable, and restorable from the page
   with `IMPORT`). Installing is only offered once a backup exists, or the
   "nothing to lose" box is ticked.
3. **Install** the build picked from `builds/index.json` — one per pull request
   labelled *test on hardware*. Each `builds/pr<N>/manifest.json` has the same
   shape `tools/check_installer.py` enforces for the real installer: four parts
   at their own offsets (the save at `0x9000` is never written) and
   `new_install_prompt_erase: true`, which — despite the name — is the setting
   that does *not* silently erase the chip. Every build here was run through
   that check.
4. **Automatic tests** over the serial console:
   - the console answers, and `HEALTH` reports what the selected PR adds;
   - memory headroom, last restart reason, SD card, PMU read errors;
   - **saves after a battle** — the reported "progress lost" bug: starts a
     console battle, changes one value with `MISS`, resets the board (RTS, or
     asks for the RESET button) and checks the value survived, then puts it
     back. Against v3.11 this fails; with the fix it passes;
   - frames that did not change are not pushed (performance PR);
   - `BEEP`, confirmed by ear.
5. **Manual checks** for what only a person can see (flicker, the badge after a
   gym win and a reset, music stopping, energy, party dialogs, PWR, dimming).
6. **Soak test**: `HEALTH` once a minute for as long as the tab stays open,
   charted, with the heap trend in bytes per hour and any restart marked.
7. **Report**: everything above as one text block to paste back.

## Serving it

Web Serial needs a secure context. Either enable GitHub Pages for the repo, or
open the page through a raw-file CDN that serves HTML with the right type, e.g.
`https://raw.githack.com/<owner>/<repo>/<commit>/web/bench/index.html`.
It cannot run inside a chat artifact: those are sandboxed iframes without the
serial permission.

## Rebuilding a PR's firmware

The binaries are the four parts `arduino-cli compile --fqbn
"esp32:esp32:esp32s3:CDCOnBoot=cdc,FlashSize=16M,PSRAM=opi,PartitionScheme=app3M_fat9M_16MB"
--export-binaries` produces for that PR's head commit — `bootloader.bin`,
`partitions.bin`, `boot_app0.bin` and `app.bin`, exactly what
`tools/build_web.sh` copies for the real installer — plus a `build.json`
(`pr`, `title`, `branch`, `commit`, `version`, `built`, `dir`, `expects`) and a
regenerated `builds/index.json`. `expects` lists what that firmware's `HEALTH`
must print (`last_boot`, `pmu_err`, `saves`, `frames`), so the page can tell
whether the selected build is really the one on the board.

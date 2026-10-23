# Suspected dead code

A record, not a to-do list. **Nothing here has been removed, on purpose**: the
owner asked for dead code to be documented only. Some of it may be kept
deliberately (a debug hook, a future feature, a fallback for old SD cards), and
removing a fallback that real cards still rely on would turn "unused" into
"broken" on somebody's board.

Every entry was checked with `grep` across `*.ino`, `*.cpp` and `*.h` at the
repository root (the firmware). The emulator under `tools/emu/` compiles the
same sources, so a use that exists only there is called out separately. Line
numbers are as of this commit and will drift; the names will not.

How to re-check an entry: `grep -n 'name' *.ino *.cpp *.h` and, for test-only
uses, `grep -ln 'name' tools/emu/tests/*.cpp`.

## Never called

| What | Where | Evidence | Note |
|---|---|---|---|
| `Pet::feed()` | `pet.cpp`, `pet.h` | definition + declaration only | Superseded by `feedBerry()`; the comment calls it "compatibility". |
| `Pet::play()` | `pet.cpp`, `pet.h` | definition + declaration only | The minigames go through `playResult()`. |
| `Party::isFull()` | `party.h` | inline definition only | The farewell path asks `add()`/`boxAdd()` directly. |
| `saveExportSize()` | `save.cpp`, `save.h` | definition + declaration only | `EXPORT` uses a fixed buffer. |
| `CLASSIC_DEX[]`, `NUM_CLASSIC_DEX` | `dex.h` (generated) | defined, never read | `REGIONS[].starters` replaced it. Removing it means changing the generator (`tools/gen_dex.py`), not `dex.h`. |
| `renderCardMedals()` | `TamaPoke.ino` | definition only | Medals moved from the creature's card to the player card (`renderPlayerMedals()`); `CARD_PAGES` is 4 and this page is not among them. |
| `drawMedalBadge()` | `TamaPoke.ino` | definition only | Not even `renderCardMedals()` calls it; the player card's medal page draws its own. |
| `playTone()` | `audio.cpp` | definition only | The old single-square tone path; everything plays through `pump()`/`GbSynth` now. |
| `M_BATTLE[]`, `M_VICTORY[]`, `TuneDef`, `MUSIC[]` | `audio.cpp` | defined, never read | The old tone-table music. The task plays `MUSIC_TBL` from `music.h`. |
| `linkNowUp()`, `linkNowStats()` | `linknow.cpp`, `linknow.h` | no callers | **Probably keep.** CLAUDE.md describes `linkNowStats()` as written for the first two-board session; it is a debugging hook waiting for hardware, not leftovers. |

## Used by tests only

| What | Where | Used in | Note |
|---|---|---|---|
| `Pet::pendingLearnables()` | `pet.cpp` | `moves_test`, `touch_test` | Also has trap #1 built in: it ignores `moveUnlockLevel()`, so it would offer level-0 TMs at any level. If the firmware ever calls it, fix that first. |
| `Pet::badgeCount()` | `pet.h` | `touch_test` | The firmware counts per region with `badgeCountIn()`. |

## Code that runs but cannot do anything

| What | Where | Why |
|---|---|---|
| `holdStart` | `TamaPoke.ino` (global) | Declared, never read or written; the long-press uses `tStart`. |
| `BTN_TRAIN` | `TamaPoke.ino` | Defined, never compared: the tap handler's final `else` is the train button. |
| `foeSwitched` + `(void)foeSwitched` | `btlResolve()` | Set, then explicitly discarded. |
| `btlWinUntil = millis() + 60000` | `btlResolve()` | The deadline is never compared with anything; only "non-zero" matters, so it is a flag that looks like a timer. |
| `passive` in `renderTrain()` | `TamaPoke.ino` | Always `false` ("every row opens a game now, DEF included"), so the `UI_TRACK` branch is unreachable. The comments near the training globals and in CLAUDE.md § "Training mechanics" still describe DEF as passive. |
| `tmLevelFor(const MoveEntry &m)` | `pet.cpp` | The parameter is ignored (`(void)m;`, flat `TM_LEVEL`). The comment above `relearnFromLevel()` still says "roughly power/2". |
| `missed` | `tools/emu/tests/lossy_test.cpp` | Unused local (test code). |

## Probably dead, cannot be confirmed from here

| What | Where | Why it is uncertain |
|---|---|---|
| `SdMon` and the `mon` fallback (`mon.load()`, `drawPetSD()`, the `mon.loaded` branch of `renderGame()`) | `sdmon.cpp`, `TamaPoke.ino` | It loads `/mons/NNN.bin` (TPK1). No tool in `tools/` writes that name -- `pack_pmd.py` and `make_thumbs.py` write `pNNN.bin` and `thumbs.bin` -- so with any card filled by the current installer it never loads anything, and costs one failed SD open per species change. A card prepared by an **older** pipeline might still carry those files, and on such a card this is the only sprite path, which is why it is listed here and not removed. |
| The trainer and wild-battle themes in `music.h` (`MUSIC_TBL[1]`, `[2]`) | `music.h` | The firmware only plays indices 0 and 3 (`audio.cpp`). The emulator's `--wav` export (`tools/emu/wavout.cpp`) plays all of them, so they are reachable for listening, just not in a game. |

## Duplication worth knowing about (not dead, but drifting)

- `startTrainerBattle()`, `startBattle()` and `startLinkBattle()` each carry
  their own copy of the ~15-line battle reset, and they have already drifted:
  the link one does not reset the lunge/hit timers.
- `startBattle()` re-implements `foeFromSpecies()` inline.
- The leader's top level is computed in three places.
- `AIL_STR[]` is defined twice (`btlNarrate()` and `btlSide()`).
- `renderBox()` and `boxTap()` use a literal `88` where `PARTY_GRID_Y` exists.

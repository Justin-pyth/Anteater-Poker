# Anteater Poker — Alpha → Beta Migration Guide

A step-by-step guide for a junior developer building the **beta** version on top of
the **alpha** codebase. Each phase points to the matching `diff_*.c` file in this
folder, where every changed/added function is documented in detail with
before/after snippets and implementation steps.

---

## 0. What these two branches are

Alpha and beta are two clones of the **same GitHub repository**. This guide
documents a comparison **pinned to two immutable commit hashes**, not to branch
names (branches move; hashes do not):

| | Commit | Branch when written | Location |
|---|---|---|---|
| Alpha | `200932f` | `gladeUI` | `/home/duc/Anteater-Poker/alpha` |
| Beta  | `aa84c01` | `test` *(see warning)* | `/home/duc/Anteater-Poker/beta` |

They diverged at the **last common commit (merge-base)**:
`5ca712d "restructure GUI to use Glade UI"`. Both sides committed after that
point (28 commits on beta, 27 on alpha) — see **section 1, Commit summary**. The
`diff_*.c` headers say "BETA (test @aa84c01)"; "test" is just the branch name as
of writing — the load-bearing identifier is the hash `aa84c01`.

To see the real diff for any file, run from inside `/home/duc/Anteater-Poker/beta`:

```sh
git diff 200932f aa84c01 -- <filename>      # e.g. game.c
git diff 200932f aa84c01 --stat             # full overview
```

> The alpha repo is registered as a git remote named `alpha` inside the beta
> clone and fetched, which is what makes the cross-clone `git diff` above work.

> ⚠️ **Branch-state note (verified 2026-05-28).** Two things changed since this
> guide was first written:
>
> 1. **The `test` branch was deleted**, so no branch points at `aa84c01` anymore.
>    It is now **secured by the tag `beta-snapshot`** (created 2026-05-28 via
>    `git tag -a beta-snapshot aa84c01`), so it is safe from `git gc` and every
>    `git diff 200932f aa84c01` command in this guide keeps working. You may use
>    `beta-snapshot` anywhere this guide writes `aa84c01`. To restore the branch
>    itself: `git branch test beta-snapshot`.
> 2. **The beta working directory is currently checked out to `main` (`4d893e0`)**,
>    which is a *different, divergent* line that does **not** contain the beta
>    features documented here (no `server_gui.c`, no `tests/`, etc.). To actually
>    build/inspect the documented beta, check out the snapshot first:
>    ```sh
>    git -C /home/duc/Anteater-Poker/beta checkout beta-snapshot   # detached HEAD
>    ```

> 💾 **Local history backup.** The full history this guide's diffs depend on is
> saved as a self-contained git bundle: **`diff/beta-guide-history.bundle`** (~1.5 MB).
> It carries tags `alpha-snapshot` (`200932f`) and `beta-snapshot` (`aa84c01`) plus
> every commit between them, so the guide survives even if the `beta` clone is lost
> or further edited. It is **local only** — nothing is pushed to any remote.
> Restore and verify:
> ```sh
> git clone /home/duc/Anteater-Poker/diff/beta-guide-history.bundle restored
> cd restored && git diff 200932f aa84c01 --stat   # verified: 98 files, +5198/-1054
> ```
> Refresh the bundle if you ever extend the documented range:
> `git -C /home/duc/Anteater-Poker/beta bundle create diff/beta-guide-history.bundle refs/tags/alpha-snapshot refs/tags/beta-snapshot`

---

## 1. Commit summary

Beta is **28 commits** ahead of the merge-base; alpha is **27 commits** ahead on
its own line. The two branches genuinely diverged — this is **not** a clean
fast-forward, and "beta" is **not** a strict superset of "alpha" (read the
divergence note below before assuming a migration is loss-free).

Reproduce the full logs from inside `/home/duc/Anteater-Poker/beta`:

```sh
git log 200932f..aa84c01 --oneline --no-merges     # 28 commits unique to beta
git log aa84c01..200932f --oneline --no-merges     # 27 commits unique to alpha
```

### Beta commits, grouped by theme

| Theme | Key commits | What landed | Diff file |
|---|---|---|---|
| Server-GUI binary (new) | `ccbf088` `107ced6` `28819c0` `196596e` `830e4d5` `3c95799` | `server_gui` monitor: Cairo poker-table view, read-only chat log, per-player chip editor, split from headless `server` | diff_server_gui.c |
| `.ready` + auto-start | `50344b4` `269503f` | `.ready` chat command + 30s countdown replaces the `/ready` button; server hello + join announcements | diff_protocol.c |
| Dynamic blinds | `092732f` | Server-adjustable small/big blinds; `[SB]`/`[BB]` tags in client; fix restart hang | diff_uds.c, diff_rules.c |
| All-in runout + showdown | `aa84c01` `00a7db6` `5749f7c` | Deferred all-in reveal, reveal all hands at runout, 2s suspense delay between stages | diff_protocol.c |
| Join/leave + scoreboard | `c94e279` | Mid-match disconnect handling, end-game scoreboard, stop bots being evicted as disconnects | diff_protocol.c, diff_game.c |
| Spectating fixes | `f7ee849` `60156f0` `7ab5fc4` | Zero-chip players spectate cleanly without blocking new hands | diff_protocol.c |
| Client-GUI polish | `91d38b9` `0ff5bf7` `a7b8883` | Folding fix, chip-win popup sizing, winner display via `winnerID` | diff_gui.c, diff_gui_helpers.c |
| Glade / CSS | `08ba731` `102dfbf` | Extract CSS to `anteater_poker.css`; client UI file updates | diff_glade_client.c, diff_client.c |
| Timing fix | `83820c8` | Stop the GUI 100 ms cap from crushing runout/countdown/bot timing | diff_server_gui.c |

(Plus scaffolding/README commits `375b8a7` `9fbc9f6` `8b63a57` `e3e0072`.)

### ⚠️ Divergence note — what migrating *away from* alpha would drop

Because alpha kept committing after the merge-base too, a naive "make beta win"
migration silently **loses alpha-only work**. Check whether any of these need to
be carried over before you treat beta as the source of truth:

- **`eec558e` "adding special card implementation"** — alpha actually *implemented*
  special cards; beta **stripped** them (see [`diff_removed.c`](diff_removed.c)).
  This is the biggest divergence: real alpha logic with no beta counterpart.
- **`374ae3d` autofold-at-timeout + brief showdown pause** — alpha's timeout/showdown
  approach differs from beta's `.ready`/`showdown`-state design.
- **`5d89c81` / `f3a96e4` / `8cf14a2` ready-button removal** — *both* branches
  independently reworked the ready flow. They are not the same implementation;
  don't assume alpha's and beta's ready handling are interchangeable.
- **`3b25859` / `e466ba7` / `bd5d04e` / `1ee3634` / `200932f` label & CSS work** —
  alpha's `yourInfo_Box` + "big labels" UI is the *other side* of the client-glade
  divergence ([`diff_glade_client.c`](diff_glade_client.c)).
- **`1240b0a` "Major fixes with protocol", `56fca88` timer-over-TCP fix,
  `f59e0c9` connect-crash fix** — alpha bug fixes; confirm beta has equivalents.

---

## 2. The big picture — what beta adds

1. **Separate server-GUI binary** — `make` now builds three binaries: `server`
   (headless), `server_gui` (GTK monitor window + same TCP logic), `client`.
2. **Dynamic blind levels** — `smallBlind`/`bigBlind` stored in `GameState`,
   changeable at runtime by the server operator.
3. **Blind seat tracking** — `smallBlindIndex`/`bigBlindIndex` so the client can
   show `[SB]`/`[BB]` tags.
4. **End-game scoreboard** — players get a finishing `place`; a modal lists final
   standings when the tournament ends.
5. **Mid-match join/leave handling** — disconnects during a hand advance the
   action or end the hand cleanly instead of stalling.
6. **All-in runout reveal timing** — community cards dealt one per second; all-in
   hole cards revealed one tick later for effect.
7. **`.ready` command + 30s auto-start countdown** — replaces the old `/ready`;
   AFK players become spectators.
8. **Showdown state** — cards stay revealed after a hand until everyone `.ready`s,
   replacing the old blocking `sleep(3)`.
9. **Spectating** — zero-chip players watch instead of blocking the table.
10. **Animated card widgets** — deal slide-in + flip-reveal.
11. **Turn pulse + chip-win popup** in the client GUI.
12. **Timer colour change** — green → yellow → red as time runs low.
13. **Expanded test suite** — `tests/test_protocol.c`, `test_system.c`, `test_gui.c`.

### File-level adds / removes

- **Added:** `server_gui.c`, `server_gui.h`, `server_gui_main.c`,
  `server_gui.glade`, `tests/*`, `README.md`, `images/*.png`.
- **Removed:** `gui_extensions.c`/`.h` (folded into `gui_helpers.c` / Glade),
  `specialCards.c` (deleted). `specialCards.h` is **kept but refactored** — its
  signatures changed from `const Card*` params to `PlayerHand`/`Deck`/`CommunityCards`
  and `swapCard()` was dropped; it is a re-declared (unimplemented) API, not an
  untouched placeholder. See [`diff_removed.c`](diff_removed.c).
- **Heavily modified (not add/remove):** `anteater_poker.glade` — the client
  window's "your hand info" box was restructured and its label IDs renamed
  (breaking change). See [`diff_glade_client.c`](diff_glade_client.c).

---

## 3. Recommended migration order

Work bottom-up: shared data structures first, then game logic, then the wire
protocol, then server logic, then the client GUI, then the new server-GUI binary,
then build/cleanup. Run `make test` after each phase.

| Phase | Module | Diff file | Why this order |
|------:|--------|-----------|----------------|
| 1 | `uds.h` — shared structs | [`diff_uds.c`](diff_uds.c) | New `Player.place` and six `GameState` fields drive **everything** below. Do this first. |
| 2 | `game.c` / `game.h` — game logic | [`diff_game.c`](diff_game.c) | Showdown flow, blind amounts, `place` ranking, `resolveNoActStep`, `updatePlaces`. |
| 3 | `rules.c` — turn order & blinds | [`diff_rules.c`](diff_rules.c) | `nextActive` infinite-loop fix; `initBlinds` records blind seats + amounts. |
| 4 | `com.c` / `com.h` — wire protocol | [`diff_com.c`](diff_com.c) | Serialise the new fields + add `MSG_TYPE_JOIN`. **Encode and decode must stay in lockstep.** |
| 5 | `protocol.c` / `protocol.h` — server logic | [`diff_protocol.c`](diff_protocol.c) | The heart of beta: showdown flow, `.ready` countdown, all-in runout, join/leave. |
| 6 | `server.c` — headless main loop | [`diff_server.c`](diff_server.c) | Three-case timed loop + `reconcile_disconnects`. |
| 7 | `gui_assets.h` — widget structs / CSS | [`diff_gui_assets.c`](diff_gui_assets.c) | Struct & CSS changes the client code below depends on. |
| 8 | `client.c` — GUI wiring | [`diff_client.c`](diff_client.c) | `init_gui` widget lookups, CSS-from-file, timer callback. |
| 9 | `gui_helpers.c` / `.h` — widgets/timers/anims | [`diff_gui_helpers.c`](diff_gui_helpers.c) | Card images, deal/flip animations, turn pulse, chip-win popup, `sendNameToServer`. |
| 10 | `gui.c` / `gui.h` — client GUI logic | [`diff_gui.c`](diff_gui.c) | `refresh_ui` (the biggest change), `.ready`, scoreboard, blind tags. |
| 11 | `server_gui.*` — NEW server GUI | [`diff_server_gui.c`](diff_server_gui.c) | New `server_gui` binary; depends on the protocol `chat_notify` hook from phase 5. |
| 12 | Removed modules | [`diff_removed.c`](diff_removed.c) | Read only if migrating code that used `gui_extensions` / `specialCards`. |
| 13 | `Makefile` | [`diff_makefile.c`](diff_makefile.c) | New `server_gui` rules, expanded `test` target, prerequisites. |
| 14 | `anteater_poker.glade` — client UI | [`diff_glade_client.c`](diff_glade_client.c) | **Do alongside phases 7–9.** The `.glade` widget IDs (renamed `ammount_to_call`/`player_ammount`) must match the `client.c` lookups, or labels silently never update. |
| 15 | `tests/*` — test suite | [`diff_tests.c`](diff_tests.c) | Migrate with the `Makefile` `test` target (phase 13). Not really "last" — run `make test` after *every* phase; it maps a red test back to the phase that broke it. |

---

## 4. Cross-cutting gotchas (read before you start)

- **Encode/decode symmetry (phase 4):** every field added to `encode_server_data`
  / `encode_player_data` must be mirrored in the matching `decode_*` at the same
  position, or the client decodes shifted garbage. `make test` (`test_protocol.c`)
  catches mismatches.
- **No blocking in the server loop (phase 5):** the old `sleep(3)` was removed
  because it froze all client I/O. Beta replaces it with the async
  `ready_countdown`. Never reintroduce a blocking sleep in `protocol.c`/`server.c`.
- **`nextActive` double-modulo (phase 3):** `(((curr+step)%N)+N)%N` is required —
  C's `%` can be negative, and a stale `uint8_t` dealer index of 255 will otherwise
  loop. Do not "simplify" it.
- **Never call `gtk_dialog_run()` (phases 10–11):** it spins a nested event loop
  and blocks the network. Use `gtk_widget_show_all` + a `"response"` → `destroy`
  callback instead.
- **Server-GUI manual pump (phase 11):** `server_gui` has **no** `gtk_main()`. The
  select() loop calls `server_gui_refresh()`, which drains
  `gtk_events_pending()` / `gtk_main_iteration()` every pass. If you don't call it
  often enough the window freezes — hence the 100ms select timeout.

---

## 5. Prerequisites

Before you build either branch, make sure you have:

- [ ] **GCC or Clang with C11** support.
- [ ] **GTK 3 development libraries** — `sudo apt install libgtk-3-dev` (Debian/Ubuntu).
- [ ] **pkg-config** can find GTK: `pkg-config --cflags gtk+-3.0` prints flags.
- [ ] **Runtime assets in the working directory:** `./images/*.png`,
      `anteater_poker.glade` (client) and `server_gui.glade` (server GUI). The
      binaries load these by relative path, so run them from the repo directory.
- [ ] **Basic git** (`checkout`, `remote`, `fetch`, `diff`). The cross-clone diff
      needs the `alpha` remote already fetched into the beta clone (section 0).
- [ ] **The `aa84c01` snapshot is reachable** (`git rev-parse aa84c01` succeeds).
      If not, recover it from the reflog and tag it — see the section 0 warning.

---

## 6. Verifying as you go

```sh
cd /home/duc/Anteater-Poker/beta
git checkout beta-snapshot  # IMPORTANT: the working dir defaults to main, which
                            # lacks server_gui/tests (section 0). Skip only if you
                            # are already on a branch/commit that contains aa84c01.
make            # build server, server_gui, client
make test       # build + run the five test binaries (see diff_tests.c)
./server        # headless, or:
./server_gui    # GTK monitor window
./client        # connect a player
```

Each diff file ends its entries with concrete "HOW TO IMPLEMENT" steps; treat the
file for a phase as the checklist for that phase, and confirm against the real
diff (`git diff 200932f aa84c01 -- <file>`) whenever a snippet is unclear.

---

## 7. Troubleshooting

| Symptom | Likely cause & fix |
|---|---|
| `git diff 200932f aa84c01` → *"bad object aa84c01"* | The commit isn't in this clone. Restore from the local backup: `git clone diff/beta-guide-history.bundle restored` (carries tags `alpha-snapshot`/`beta-snapshot`). The live `beta` clone also has it tagged `beta-snapshot` (section 0). |
| `cd beta && make` produces no `server_gui` / no `tests/` | The working dir is on `main` (`4d893e0`), not the documented beta. `git checkout aa84c01` first (section 0). |
| Stack / "amount to call" labels never update in the client | Widget-ID mismatch: code asks for `ammount_to_call`/`player_ammount` but the `.glade` still defines `label_call_amnt`/`label_your_stack` (or vice-versa). Migrate `.glade` + `client.c` lookups together — [`diff_glade_client.c`](diff_glade_client.c). `make test` (test_gui) catches this. |
| Client shows shifted / garbage field values | `encode_*`/`decode_*` out of lockstep (phase 4). Run `./tests/test_protocol` — it fails on the exact mismatched field. |
| Server freezes, no client I/O | A blocking call crept into the loop — a reintroduced `sleep()` (phase 5) or `gtk_dialog_run()` (phases 10–11). Use the async countdown / non-blocking dialog instead. |
| `server_gui` window freezes / stops repainting | The select() loop isn't draining `gtk_events_pending()` often enough. Keep the 100 ms `select` timeout and call `server_gui_refresh()` every pass (phase 11). |
| `make test` hangs on `test_system` | A stale server is holding TCP port **10160**. Kill it (`fuser -k 10160/tcp`, or `pkill -f ./server`) and re-run. |
| `fatal error: gtk/gtk.h: No such file` / `cannot find -lgtk-3` | GTK 3 dev libs missing or pkg-config can't see them — install `libgtk-3-dev` (section 5). |
| Merge conflict inside `anteater_poker.glade` | Don't hand-merge the XML. Take beta's file wholesale and re-verify it loads in the Glade editor — [`diff_glade_client.c`](diff_glade_client.c). |

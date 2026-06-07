/*
 * ============================================================================
 *  MODULE DIFF:  tests/   —  NEW automated test suite
 *  ALPHA (gladeUI @200932f)  ->  BETA (test @aa84c01)
 * ============================================================================
 *
 *  TEACHING ARTIFACT — not compilable code.  Every file under tests/ is NEW in
 *  beta (alpha had only a minimal tests/test_game.c + tests/flow_demo per the
 *  old Makefile).  Beta turns `make test` into a real safety net: five binaries
 *  covering game logic, the wire protocol, the full client-server loop, and the
 *  GUI handlers.  Run them after EVERY phase of the migration — they are how you
 *  catch an encode/decode mismatch or a NULL widget lookup before it costs you
 *  an afternoon.
 *
 *  Reproduce the raw diff from inside /home/duc/Anteater-Poker/beta:
 *      git diff 200932f aa84c01 -- tests/
 *      git show aa84c01:tests/test_protocol.c        # view any one file
 *
 *  Build + run everything:   make test
 *  (Makefile changes that wire these up are documented in diff_makefile.c.)
 *
 *  All five files share one tiny harness macro — no framework needed:
 *      #define TEST(name, expr) do { ...print PASS/FAIL, count failures... } while (0)
 *  Each binary returns non-zero if any TEST failed, so `make test` stops on red.
 * ============================================================================
 */


/* $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
 * $   FILE 1 — tests/test_game.c   (~417 lines)   unit: game logic          $
 * $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$ */

/*
 * Drives game.c + rules.c directly (no network, no GTK).  Covers move
 * validation, move application, hand evaluation, award(), showdown, and a full
 * hand progression.  Every move type — FOLD, CHECK, CALL, RAISE, ALL_IN — is
 * exercised in both a valid and an invalid case.
 *
 * Representative assertions (names are the TEST() labels):
 *   - "fold is valid for current player" / "fold is invalid for other player"
 *   - "fold sets PLAYER_FOLDED", "showdown set when last player folds",
 *     "winnerID correct after fold-win"
 *   - "check valid when bet matched" / "check invalid when behind on bet"
 *   - "call deducts exact chips", "call increases pot correctly"
 *
 * WHY IT MATTERS FOR MIGRATION: this is the first thing that breaks if you get
 * the showdown flow or the new GameState fields wrong in phase 2 (diff_game.c).
 */


/* $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
 * $   FILE 2 — tests/test_protocol.c (~284 lines)  unit: encode/decode      $
 * $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$ */

/*
 * Round-trips every message type through com.c (prepare_payload ->
 * receive_payload) and asserts the decoded struct equals the original.
 *   - test_gamestate_roundtrip: pot, currentBet, stage, currentPlayer,
 *     yourPlayerID, handPlaying, SHOWDOWN (the new field!), communityCount,
 *     community card rank/suit, and per-player chips/status/name.
 *   - test_action_roundtrip, test_chat_roundtrip, test_error_roundtrip,
 *     test_ready_roundtrip.
 *
 * WHY IT MATTERS: this is THE guard for the cross-cutting "encode/decode must
 * stay in lockstep" gotcha (README section 4, phase 4 / diff_com.c).  Add a
 * field to GameState and forget to mirror it in decode -> this test goes red on
 * the exact field.  Run it the instant you touch com.c or uds.h.
 */


/* $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
 * $   FILE 3 — tests/test_system.c (~458 lines)  integration: client/server $
 * $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$ */

/*
 * The real thing: fork()s an actual server child, then drives TWO test clients
 * over real TCP through the whole loop:
 *   1. connect + JOIN name exchange   2. .ready -> hand starts (state broadcast)
 *   3. PLAYER_ACTION FOLD/CHECK/CALL/RAISE round-trips
 *   4. CHAT echoed to all clients     5. invalid move -> error message back
 *   6. .ready after showdown -> next hand starts
 *
 * Requires _POSIX_C_SOURCE 200809L.  Uses TCP port 10160 and KILLS any server
 * already on that port first — so do not run a manual ./server on 10160 while
 * this test runs.  If it hangs, a stale child is usually the cause (see
 * README Troubleshooting).
 */


/* $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
 * $   FILE 4 — tests/test_gui.c (~379 lines)   headless GUI handlers        $
 * $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$ */

/*
 * Runs GTK headless via GDK_BACKEND=offscreen, loads anteater_poker.glade, wires
 * the AppWidgets struct, then calls every event handler to prove it works
 * without a display and produces the right state:
 *   - test_refresh_basic/showdown/waiting: refresh_ui sets the stage label
 *     (FLOP / SHOWDOWN / WAITING), pot label, and player-chips label.
 *   - test_append_chat, test_anteater_count.
 *   - test_action_handlers: on_fold/on_check/on_call/on_allin/on_raise "no crash".
 *
 * WHY IT MATTERS: this is what catches the diff_glade_client.c silent-NULL trap
 * — if a renamed widget ID (ammount_to_call / player_ammount) does not resolve,
 * the label assertions fail here instead of in production.
 */


/* $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
 * $   FILE 5 — tests/flow_demo.c (~123 lines)   bot-driven hand simulation  $
 * $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$ */

/*
 * Not an assertion test — a demo/smoke run.  Drives game.c + rules.c + bot.c to
 * play complete hands with bot AI and no network, printing the play-by-play.
 * Useful for eyeballing that a full hand reaches a sane showdown after you
 * change game logic.  Present in alpha too, but kept/updated in beta.
 */


/* $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
 * $   HOW TO USE DURING MIGRATION                                          $
 * $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$ */

/*
 *   After each README phase, run:    make test
 *   Map a red test back to its phase:
 *     test_protocol fails  -> phase 4 (com.c) field mismatch.
 *     test_game fails      -> phase 2 (game.c) showdown/award/blind logic.
 *     test_system hangs/fails -> phase 5/6 (protocol.c/server.c) loop or
 *                                 join/leave handling; check for a stray
 *                                 process on port 10160 first.
 *     test_gui label fail  -> phase 7-9 widget IDs (diff_glade_client.c /
 *                                 diff_gui_assets.c / diff_client.c).
 *
 *   Migrate the tests/ directory and the Makefile `test` target together
 *   (diff_makefile.c) — a test binary is useless if `make test` does not build
 *   and run it.
 */

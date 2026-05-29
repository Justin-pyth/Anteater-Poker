/*
 * ============================================================================
 *  MODULE DIFF:  Makefile   —  Build system changes
 *  ALPHA (gladeUI @200932f)  ->  BETA (test @aa84c01)
 * ============================================================================
 *
 *  TEACHING ARTIFACT — not compilable code (this documents Makefile changes).
 *
 *  Reproduce the raw diff from inside /home/duc/Anteater-Poker/beta:
 *      git diff 200932f aa84c01 -- Makefile
 * ============================================================================
 */


/* $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
 * $   PART 1 — MODIFIED TARGETS                                            $
 * $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$ */

/*
 * DEFAULT TARGET (all):
 *   ALPHA: builds server and client.
 *   BETA:  builds server, server_gui, and client.
 *
 * TEST TARGET (test):
 *   ALPHA: compiles and runs tests/test_game and tests/flow_demo.
 *   BETA:  compiles and runs test_game, test_protocol, flow_demo, test_system,
 *          test_gui — all five built and run in sequence by `make test`.
 *
 * CLEAN TARGET:
 *   Updated to also remove server_gui and all new test binaries.
 *
 * REMOVED: the `run` phony target (was `clean && ./server`; not in beta Makefile).
 */


/* $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
 * $   PART 2 — NEW RULES                                                   $
 * $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$ */

/*
 * server_gui binary link rule (new):
 *   server_gui: server_gui_main.o server_gui.o protocol.o game.o rules.o bot.o com.o
 *       $(CC) $(CFLAGS) $^ $(GTK_LIBS) -rdynamic -o $@
 *
 * server_gui.o compile rule (new, requires GTK_CFLAGS):
 *   server_gui.o: server_gui.c server_gui.h protocol.h uds.h
 *       $(CC) $(CFLAGS) $(GTK_CFLAGS) -c server_gui.c -o server_gui.o
 *
 *   NOTE: server_gui_main.o does NOT need GTK_CFLAGS — it only includes protocol.h
 *   and server_gui.h, not gtk/gtk.h directly.  This keeps the entry point lean and
 *   fast to compile.
 */

/*
 * HOW TO BUILD / RUN:
 *   Build everything:        make
 *   Run all tests:           make test
 *   Run headless server:     ./server
 *   Run GUI server:          ./server_gui
 *   Run client:              ./client
 *   Clean build artefacts:   make clean
 *
 * PREREQUISITES:
 *   - GCC (or Clang) with C11 support.
 *   - GTK 3 dev libraries:   sudo apt install libgtk-3-dev   (Debian/Ubuntu)
 *   - pkg-config must find gtk+-3.0:   pkg-config --cflags gtk+-3.0
 *   - ./images/*.png present in the working directory when running.
 *   - server_gui.glade present when running server_gui.
 *   - anteater_poker.glade present when running client.
 */

/*
 * ============================================================================
 *  MODULE DIFF:  server_gui.c / server_gui.h / server_gui_main.c  — NEW FILES
 *                —  GTK server monitor window + its binary entry point
 *  ALPHA (gladeUI @200932f)  ->  BETA (test @aa84c01)
 * ============================================================================
 *
 *  TEACHING ARTIFACT — not compilable code.
 *  These three files do NOT exist in alpha.  They add a second server binary
 *  (`server_gui`) that runs the SAME TCP logic as the headless `server` but
 *  also opens a GTK monitor window.  Prerequisite reading: diff_protocol.c
 *  (chat_notify hook, runout/countdown fields) and diff_server.c (the loop this
 *  one mirrors).
 *
 *  These are NEW files, so there is no before/after — read them in full in the
 *  beta tree:
 *      /home/duc/Anteater-Poker/beta/server_gui.c
 *      /home/duc/Anteater-Poker/beta/server_gui.h
 *      /home/duc/Anteater-Poker/beta/server_gui_main.c
 * ============================================================================
 */


/* $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
 * $   PART 2 — NEW FILES / FUNCTIONS                                       $
 * $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$ */

/*
 * NEW FILE: server_gui.h
 *
 * Public API for the server monitor window.  Exactly four functions:
 *   void server_gui_init(ServerState *state)      — load glade, show window
 *   void server_gui_refresh(ServerState *state)   — update all labels + redraw
 *   void server_gui_append_chat(const char *sender, const char *msg)  — add chat line
 *   int  server_gui_active(void)                  — returns 1 if window is open
 *
 * Included only by server_gui.c and server_gui_main.c.  The headless server.c
 * does NOT include it — clean separation.
 */

/*
 * NEW FILE: server_gui.c  (~843 lines)
 *
 * OVERVIEW: A GTK 3 window with two tabs:
 *   1. "Game View" tab — a 700x620 Cairo drawing area showing the full table
 *      (all hole cards face-up = "god view"), community cards, blind/dealer
 *      buttons, plus a chat log beside it.
 *   2. "Monitor" tab — a grid of labels showing each player's name, chips, status
 *      and hole cards (Pango markup for colour), per-player chip-editing spin
 *      buttons, a restart button, and blind-level controls.
 *
 * STATIC GLOBALS:
 *   g_active        — 1 if the GTK window is open.
 *   g_game_view     — the Cairo drawing area widget.
 *   g_chat_view     — the read-only chat log text view.
 *   g_server_state  — pointer to the ServerState passed to server_gui_init().
 *   g_stage_label, g_pot_label, g_deck_label, g_turn_label — info labels.
 *   g_community[5]  — community card label widgets (Pango markup).
 *   g_p_name/chips/status/card0/card1/dealer/spin[MAX_PLAYERS] — per-player widgets.
 *   g_start_spin, g_small_blind_spin, g_big_blind_spin — control widgets.
 *
 * CAIRO HELPERS (all static):
 *   rrect(cr, x, y, w, h, r)        — rounded rectangle path.
 *   text_ctr(cr, s, cx, cy)         — text centred at a point.
 *   draw_card(cr, x, y, w, h, c, face_up) — one card (image or font fallback).
 *   draw_card_slot(cr, x, y, w, h)  — empty card placeholder.
 *   draw_felt(cr)                   — oval green felt + brown rail.
 *   draw_center_info(cr, gs)        — stage, pot, blind levels in felt centre.
 *   draw_community(cr, gs)          — 5 community card slots.
 *   set_status_color(cr, st)        — Cairo colour for a PlayerStatus.
 *   status_name(st)                 — short status string.
 *   draw_seat(cr, idx, gs)          — one seat (box, name, chips, status,
 *                                     dealer/SB/BB buttons, hole cards).
 *   on_draw_game_view(da, cr, data) — the "draw" signal callback; calls all above.
 *
 * MONITOR TAB HELPERS (all static):
 *   format_card_mu(buf, len, c)     — Pango markup for a card (coloured suit).
 *   set_mon_card_empty(lbl)         — set a card label to "  —  ".
 *   set_mon_card_hidden(lbl)        — set a card label to " ?? ".
 *   mon_status_mu(st)               — Pango markup for a PlayerStatus.
 *   stage_str(s, playing)           — "PREFLOP" / "FLOP" etc.
 *
 * BUTTON CALLBACKS (all static):
 *   on_restart(btn, state)          — reads starting chips from g_start_spin,
 *                                     resetGame(), resets chip amounts, clears
 *                                     runout/countdown, broadcasts.
 *   on_set_blinds(btn, state)       — reads SB/BB, clamps BB >= SB, writes to
 *                                     game state, broadcasts.
 *   on_window_destroy(w, d)         — sets g_active = 0 (stops the GUI pump).
 *   on_set_chips_N  (N=0..5)        — macro-generated per-player chip-set callbacks.
 *     (MAKE_SET_CHIPS_CB macro expands 6 identical functions with hard-coded indices.)
 *
 * MAIN PUBLIC FUNCTIONS:
 *
 *   server_gui_init(state):
 *     Step 1.  gtk_init(NULL, NULL).
 *     Step 2.  Load "server_gui.glade" with gtk_builder_new_from_file().
 *     Step 3.  Get the top-level window; connect its "destroy" signal.
 *     Step 4.  Look up all monitor tab widgets by Glade ID.
 *     Step 5.  Wire per-player set-chips buttons to on_set_chips_N (via a static
 *              GCallback array).
 *     Step 6.  Wire restart and set-blinds buttons.
 *     Step 7.  Look up the Cairo drawing area; connect "draw" with the ServerState
 *              pointer as user data.
 *     Step 8.  Look up the chat log text view.
 *     Step 9.  gtk_widget_show_all(); set g_active = 1.
 *
 *   server_gui_refresh(state):
 *     Step 1.  Guard: if !g_active return.
 *     Step 2.  Update stage, pot, deck-count, turn labels.
 *     Step 3.  Update community card labels via format_card_mu().
 *     Step 4.  Per player: update name, chips, status (Pango markup), card labels,
 *              dealer label.  Toggle spin/set-btn sensitivity.
 *     Step 5.  gtk_widget_queue_draw(g_game_view) to repaint the Cairo view.
 *     Step 6.  while (gtk_events_pending()) gtk_main_iteration();
 *              ^ The CRITICAL GTK event pump.  There is NO gtk_main() running —
 *              the server uses a plain select() loop.  Draining pending events on
 *              every call keeps the window responsive.
 *
 *   server_gui_append_chat(sender, msg):
 *     Lazily creates "server"/"player" text tags, inserts the coloured sender
 *     prefix + message body, and auto-scrolls to the bottom.
 *
 * IMPLEMENTATION GUIDE FOR server_gui.c — the key insight:
 *   server_gui_refresh() drives the GTK event loop instead of gtk_main().  Pattern:
 *
 *     // In server main loop (server_gui_main.c):
 *     ... select() ...
 *     ... handle I/O ...
 *     server_gui_refresh(&state);   // pump GTK events + redraw
 *
 *   This "manual pump" means:
 *   - The server loop controls timing (not GTK's main loop).
 *   - Button clicks are processed during the gtk_main_iteration() calls inside
 *     server_gui_refresh().
 *   - No threads are needed.
 *
 *   PITFALL: If server_gui_refresh() is not called frequently enough (e.g. the
 *   server blocks in select() too long), the window freezes.  That is why
 *   server_gui_main.c uses a 100ms timeout when g_active is true.
 *
 *   PITFALL: The on_set_chips_N callbacks use MAKE_SET_CHIPS_CB with hard-coded
 *   seat indices.  If MAX_PLAYERS ever changes from 6, extend them manually.
 *   There are exactly 6 generated functions: 0 through 5.
 */

/*
 * NEW FILE: server_gui_main.c  (~137 lines)
 *
 * OVERVIEW: The `server_gui` binary's entry point.  Structurally identical to
 * server.c (diff_server.c) except:
 *   1. Calls server_gui_init(&state) after init_server() and create_socket().
 *   2. Registers state.chat_notify = server_gui_append_chat.
 *   3. Uses wall-clock timestamps (time_t) to schedule timed events rather than
 *      relying solely on select() timeout counting:
 *        runout_fire_at    — next time step_runout() should fire
 *        countdown_fire_at — next time tick_ready_countdown() should fire
 *        bot_fire_at       — next time doOneBotTurn() should fire
 *   4. When server_gui_active() returns 1, the select() timeout is shortened to
 *      100ms so the GTK window stays responsive.
 *   5. Calls server_gui_refresh(&state) after every significant event and at the
 *      end of every loop iteration.
 *
 * WHY TIMESTAMP-BASED SCHEDULING:
 *   The headless server can count select() timeouts directly because its loop is
 *   tight.  The GUI server's loop spends variable time in gtk_main_iteration().
 *   Using time(NULL) timestamps ensures timed events (runout steps, countdown
 *   ticks, bot moves) fire at the right wall-clock moment regardless of GTK time.
 *
 * IMPLEMENTATION GUIDE:
 *   Step 1.  Copy server.c main() as a starting point.
 *   Step 2.  #include "server_gui.h".
 *   Step 3.  Declare time_t runout_fire_at=0, countdown_fire_at=0, bot_fire_at=0.
 *   Step 4.  After init_server() and create_socket(), call server_gui_init().
 *   Step 5.  state.chat_notify = server_gui_append_chat.
 *   Step 6.  Inside the loop, read time_t now = time(NULL) after select().
 *   Step 7.  Replace direct step_runout/tick/bot calls with timestamp-guarded
 *            versions (fire only if now >= fire_at; reset fire_at after firing).
 *   Step 8.  Call server_gui_refresh(&state) after every non-continue path and at
 *            the bottom of the loop.
 */

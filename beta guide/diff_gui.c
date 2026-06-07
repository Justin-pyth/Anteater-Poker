/*
 * ============================================================================
 *  MODULE DIFF:  gui.c   —  Client GUI logic
 *  ALPHA (gladeUI @200932f)  ->  BETA (test @aa84c01)
 * ============================================================================
 *
 *  TEACHING ARTIFACT — not compilable code.
 *  Prerequisite reading: diff_uds.c, diff_gui_assets.c, diff_gui_helpers.c.
 *  refresh_ui() is the single most heavily-changed client function.
 *
 *  Reproduce the raw diff from inside /home/duc/Anteater-Poker/beta:
 *      git diff 200932f aa84c01 -- gui.c gui.h
 * ============================================================================
 */


/* $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
 * $   PART 1 — MODIFIED FUNCTIONS                                          $
 * $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$ */

/*
 * MODIFIED FUNCTION: on_server_data()
 *
 * WHAT IT DOES: GIOChannel callback — called when data arrives on the server
 * socket.  Reads bytes, parses them into Message objects, dispatches each one.
 *
 * CHANGE: Comment clarification only.  The code is identical; the alpha inline
 * comment was replaced with a multi-line explanation of why the while-loop is
 * needed (TCP can coalesce multiple server messages into one recv()).
 *
 * WHY (for the junior dev): TCP is a stream protocol; a single recv() may return
 * 1, 2, or 10 messages' worth of bytes.  The while-loop parses each message until
 * the buffer is exhausted.
 *
 * HOW TO IMPLEMENT: No code change required — just update the comment.
 */

/*
 * MODIFIED FUNCTION: refresh_ui()
 *
 * WHAT IT DOES: Called on every MSG_TYPE_GAME_STATE; updates every visible widget
 * to reflect the current GameState.  Many independent changes, listed by sub-topic.
 *
 * A. Static tracking variables (new at function entry):
 *      static int prev_showdown   = 0;
 *      static int prev_cur_player = -1;
 *      static int prev_gameover   = 0;
 *    WHY: allow one-shot side effects (pop scoreboard once, start turn pulse when
 *    the active player changes) without running them every tick.
 *
 * B. Tournament-over scoreboard:
 *      if (game->gameOver && !prev_gameover) show_scoreboard(game);
 *      prev_gameover = game->gameOver;
 *    WHY: calls show_scoreboard() exactly once, when gameOver first becomes 1.
 *
 * C. Stage label shows "SHOWDOWN" during showdown:
 *    ALPHA: always showed the stage name or "WAITING".
 *    BETA:  if (game->showdown) shows "SHOWDOWN"; else the existing logic.
 *
 * D. Local player card visibility:
 *    ALPHA: always showed the cards if has_cards, back otherwise.
 *    BETA:  if folded or spectating, set opacity to 0.0 (invisible) instead.
 *    WHY: a folded player should not see their own cards; a spectator has none.
 *
 * E. Opponent name label now includes blind tags:
 *    ALPHA: gtk_label_set_text(W.opp_name[slot], p->name)
 *    BETA:  snprintf(buf, ..., "%s%s", p->name, seat_blind_tag(game, pi));
 *           gtk_label_set_text(W.opp_name[slot], buf)
 *    WHY: players can see which opponent is SB/BB/Dealer.
 *
 * F. Opponent card display — four cases instead of two:
 *    ALPHA: show face if card_is_known, else show back.
 *    BETA:
 *      Case 1: FOLDED or SPECTATING -> opacity 0 (invisible)
 *      Case 2: showdown or ALL_IN + has cards -> face-up (flip anim during showdown)
 *      Case 3: local player is spectating and opponent has known cards -> face-up
 *      Case 4: otherwise -> back
 *    WHY: all-in cards are revealed; folded/spec hidden; showdown shows everything;
 *    spectators see all cards.
 *
 * G. Showdown one-shot effects:
 *    if (game->showdown && !prev_showdown):
 *        start_turn_pulse(-1)               // clear any active pulse
 *        if valid winnerID: start_chip_win_anim()
 *    prev_showdown = game->showdown;
 *    WHY: chip-win popup and pulse-clear happen once, at the moment showdown begins.
 *
 * H. Turn pulse on active opponent:
 *    When currentPlayer changes, find the opponent slot index and call
 *    start_turn_pulse(slot).  On !handPlaying, call start_turn_pulse(-1).
 *    WHY: visual feedback for which seat is acting.
 *
 * I. Log label — expanded status messages:
 *    ALPHA: "Your turn" / "Waiting for player N" / "Connected. Waiting for hand." /
 *           "Not connected."
 *    BETA adds:
 *      - "Spectating — Press Ready to continue watching."  (spectating+showdown)
 *      - "Spectating — out of chips."                       (spectating mid-hand)
 *      - "You won! +$N  Press Ready for next hand."         (showdown, I won)
 *      - "You lost.  X wins!  Press Ready for next hand."   (showdown, opponent won)
 *      - "Tie! Press Ready for next hand."                  (showdown, tie)
 *
 * J. All-in button sensitivity:
 *    BETA adds: gtk_widget_set_sensitive(W.btn_allin, my_turn);
 *
 * K. player_ammount / ammount_to_call labels (renamed from label_your_stack /
 *    label_call_amnt):
 *    ALPHA: wrote to W.label_call_amnt and W.label_your_stack.
 *    BETA:  writes to W.player_ammount ("Your amount: $N [SB]") and
 *           W.ammount_to_call ("To call: $N", capped at all-in amount).
 *    WHY: field renames align with new Glade IDs; "[SB]" uses seat_blind_tag();
 *    the all-in cap prevents "To call: $999" when you only have $50.
 *
 * HOW TO IMPLEMENT ALL OF THE ABOVE:
 *   Step 1.  Add the three static int tracking variables at function entry.
 *   Step 2.  Add the scoreboard one-shot block after infer_my_player_id().
 *   Step 3.  Add the showdown check for stage_label.
 *   Step 4.  Change the local player card section to check FOLDED/SPECTATING first.
 *   Step 5.  In the opponent loop, call seat_blind_tag() for the name label.
 *   Step 6.  Replace the two-case card display with the four-case version.
 *   Step 7.  After the opponents loop, add the showdown one-shot + turn-pulse blocks.
 *   Step 8.  Replace the log label if-chain with the expanded version.
 *   Step 9.  Add btn_allin to the sensitivity block.
 *   Step 10. Replace label_call_amnt / label_your_stack writes with the new
 *            player_ammount / ammount_to_call versions.
 */

/*
 * MODIFIED FUNCTION: on_send_chat()
 *
 * WHAT IT DOES: GTK signal callback when the player presses Send in chat.
 *
 * CHANGE:
 *   ALPHA: if (strcmp(text, "/ready") == 0) { sendReadyToServer(); append_chat("SERVER", "You are ready."); }
 *   BETA:  if (strcasecmp(text, ".ready") == 0) { sendReadyToServer(); }   // no local echo
 *
 * WHY:
 *   - Command renamed /ready -> .ready (different prefix).
 *   - strcasecmp: case-insensitive so ".Ready"/".READY" both work.
 *   - The local "You are ready." echo removed; the server now announces readiness
 *     via broadcast_chat_message, so the client receives it naturally.
 *
 * HOW TO IMPLEMENT:
 *   Step 1.  Change strcmp -> strcasecmp.
 *   Step 2.  Change "/ready" -> ".ready".
 *   Step 3.  Remove the append_chat("SERVER", "You are ready.") call.
 */

/*
 * MODIFIED FUNCTION: on_connect_clicked()
 *
 * WHAT IT DOES: Callback for the Connect button on the login screen.  Creates the
 * socket, registers the GIOChannel, transitions to the game screen.
 *
 * CHANGE: One new line after show_game_screen():
 *   sendNameToServer(name);
 *
 * WHY: the player's display name is set on the login screen.  As soon as the
 * client is in the game screen, it must send a MSG_TYPE_JOIN with that name so
 * the server can store and announce it.  Without this, all players appear as
 * empty-name "Player N" on everyone else's screen.
 *
 * HOW TO IMPLEMENT:
 *   Step 1.  After show_game_screen() in on_connect_clicked(), call
 *            sendNameToServer(name) (name = string from the username entry).
 *   Step 2.  sendNameToServer() is defined in gui_helpers.c — see diff_gui_helpers.c.
 */


/* $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
 * $   PART 2 — NEW FUNCTIONS                                               $
 * $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$ */

/*
 * NEW FUNCTION (static): seat_blind_tag()
 *   static const char *seat_blind_tag(const GameState *g, int seat)
 *
 * WHAT IT DOES: Returns a short position-tag string for a given seat during an
 * active hand: " [SB]", " [BB]", " [D]", or "".  Blinds take priority over dealer
 * so that in heads-up the dealer who is also the big blind shows as "[BB]".
 *
 * DECLARED: static in gui.c (not exposed in gui.h).
 *
 * IMPLEMENTATION GUIDE:
 *   if (!g->handPlaying) return "";
 *   if (seat == (int)g->smallBlindIndex) return " [SB]";
 *   if (seat == (int)g->bigBlindIndex)   return " [BB]";
 *   if (seat == (int)g->dealerIndex)     return " [D]";
 *   return "";
 *
 * Called from refresh_ui() for both the local player's money label and each
 * opponent's name label.
 */

/*
 * NEW FUNCTION (static): show_scoreboard()
 *   static void show_scoreboard(const GameState *game)
 *
 * WHAT IT DOES: Creates a GTK modal info dialog listing all players in tournament
 * finishing order (best -> worst).  Shown once when game->gameOver first becomes 1.
 *
 * DECLARED: static in gui.c.
 *
 * IMPLEMENTATION GUIDE:
 *   Step 1.  Collect indices of all players who participated (place != 0 or not
 *            EMPTY).  Filter out EMPTY players with place==0 and chips==0.
 *   Step 2.  Sort ascending by Player.place.  Treat place==0 as 255 so unranked
 *            stragglers sort last.
 *   Step 3.  Build a GString.  For each sorted player append e.g.
 *            "1. Alice  ($0) — WINNER   (you)\n".  Use g_string_append_printf().
 *   Step 4.  gtk_message_dialog_new() with GTK_MESSAGE_INFO and GTK_BUTTONS_OK.
 *            Use gtk_message_dialog_format_secondary_text() for the ranked list.
 *   Step 5.  Connect "response" to gtk_widget_destroy so the dialog closes on OK
 *            WITHOUT blocking the GTK main loop (use gtk_widget_show_all, NOT
 *            gtk_dialog_run).
 *   Step 6.  g_string_free(s, TRUE).
 *
 * PITFALL: Do NOT use gtk_dialog_run().  It enters a nested event loop and blocks
 * all GIO/network events while open.  show_all + response callback keeps the
 * network alive.
 */

/*
 * NEW FUNCTION: on_timer_expire()
 *   void on_timer_expire(void)
 *
 * WHAT IT DOES: Called when the local player's turn timer reaches zero.  Sends a
 * FOLD action to the server on the player's behalf.
 *
 * DECLARED IN: gui.h
 *
 * IMPLEMENTATION GUIDE:
 *   void on_timer_expire(void) { send_gui_move(FOLD, 0); }
 *
 * This replaces the is_my_timer boolean check that was inline in timer_tick_cb()
 * in alpha's gui_extensions.c.  In init_gui() (client.c), wire it up:
 *   W.my_timer.on_expire = (void(*)(void))on_timer_expire;
 */

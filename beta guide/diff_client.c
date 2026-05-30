/*
 * ============================================================================
 *  MODULE DIFF:  client.c   —  Client startup / GUI wiring
 *  ALPHA (gladeUI @200932f)  ->  BETA (test @aa84c01)
 * ============================================================================
 *
 *  TEACHING ARTIFACT — not compilable code.
 *  Prerequisite reading: diff_gui_assets.c (the AppWidgets/SeatTimer changes
 *  that init_gui() must now wire up).
 *
 *  Reproduce the raw diff from inside /home/duc/Anteater-Poker/beta:
 *      git diff 200932f aa84c01 -- client.c
 * ============================================================================
 */


/* $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
 * $   PART 1 — MODIFIED FUNCTIONS                                          $
 * $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$ */

/*
 * MODIFIED FUNCTION: init_gui()
 *
 * WHAT IT DOES: Called at startup; loads the Glade file, wires up all widget
 * pointers in the AppWidgets struct (W), and initialises timers.
 *
 * CHANGES:
 *   A. CSS loading: first tries to load "anteater_poker.css" from disk; falls
 *      back to the inline CSS string if the file is missing.
 *      ALPHA: always used the inline CSS.
 *      WHY: allows theming without recompiling; graceful fallback if CSS missing.
 *
 *   B. Two new widget lookups (old label_call_amnt / label_your_stack removed):
 *        W.player_ammount  = GET("player_ammount");
 *        W.ammount_to_call = GET("ammount_to_call");
 *
 *   C. All-in button lookup:
 *        W.btn_allin = GET("btn_ALLIN");
 *      (Note the uppercase "ALLIN" matches the Glade widget ID.)
 *
 *   D. SeatTimer.is_my_timer replaced by on_expire:
 *      ALPHA: W.my_timer.is_my_timer = TRUE;
 *      BETA:  W.my_timer.on_expire   = (void(*)(void))on_timer_expire;
 *      Opponent timers no longer set is_my_timer = FALSE (field gone).
 *
 * HOW TO IMPLEMENT:
 *   Step 1.  Add the CSS-from-file attempt block before the inline fallback.
 *   Step 2.  Replace the two removed GET() calls with the two new ones.
 *   Step 3.  Add the btn_allin GET() call.
 *   Step 4.  Change the my_timer initialisation as shown.
 *   Step 5.  Remove the W.opp_timer[i].is_my_timer = FALSE; lines in the loop.
 */

/* (client.c has no NEW functions.) */

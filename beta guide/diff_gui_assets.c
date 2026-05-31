/*
 * ============================================================================
 *  MODULE DIFF:  gui_assets.h   —  Widget structs, constants, CSS
 *  ALPHA (gladeUI @200932f)  ->  BETA (test @aa84c01)
 * ============================================================================
 *
 *  TEACHING ARTIFACT — not compilable code.
 *  gui_assets.h holds the AppWidgets struct (single container for all widget
 *  pointers, referenced everywhere as `W`), the SeatTimer and CardDrawData
 *  structs, app constants, and the inline CSS string.  These struct changes
 *  drive code in client.c, gui.c, and gui_helpers.c.
 *
 *  Reproduce the raw diff from inside /home/duc/Anteater-Poker/beta:
 *      git diff 200932f aa84c01 -- gui_assets.h
 * ============================================================================
 */


/* $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
 * $   PART 1 — MODIFIED (struct / constant / CSS changes)                  $
 * $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$ */

/*
 * CHANGE 1 — TURN_SECONDS: 30 -> 90
 *   WHY: 30 seconds was too short to think; 90 gives reasonable time per hand.
 *
 * CHANGE 2 — SeatTimer.is_my_timer (gboolean) replaced by on_expire (fn pointer)
 *   ALPHA: gboolean is_my_timer;       // used to call send_gui_move(FOLD,0) on expire
 *   BETA:  void (*on_expire)(void);    // NULL = no-op; set to on_timer_expire for local timer
 *   WHY: a function pointer is more flexible and testable than a boolean flag.
 *   The local player's timer sets on_expire = on_timer_expire (which calls
 *   send_gui_move(FOLD, 0)); opponent timers leave it NULL.
 *
 * CHANGE 3 — CardDrawData gained animation fields:
 *   was_face_up, deal_t, deal_cb_id, deal_start_us, flip_scale, flip_cb_id,
 *   flip_start_us, flip_next_card, flip_next_face, flip_midpoint.
 *   WHY: the new deal and flip animations (gui_helpers.c) track progress here.
 *
 * CHANGE 4 — AppWidgets: label_call_amnt / label_your_stack replaced by
 *            player_ammount / ammount_to_call.
 *   WHY: Glade widget IDs were renamed to match the new label layout.
 *
 * CHANGE 5 — AppWidgets: btn_allin widget pointer added.
 *   WHY: the all-in button now has a separate Glade ID and must be wired up.
 *
 * CHANGE 6 — CSS additions in the inline CSS string:
 *   - timer bar colour transition classes: timer-yellow, timer-red.
 *   - active-pulse frame class (turn-pulse blink).
 *   - #player_ammount and #ammount_to_call ID rules.
 *   - #chat_log styling.
 *
 * HOW TO IMPLEMENT:
 *   For each struct change, edit gui_assets.h, then update every read/write site:
 *     - client.c (init_gui: widget lookups, timer init)  -> diff_client.c
 *     - gui.c (refresh_ui: CardDrawData / label access)   -> diff_gui.c
 *     - gui_helpers.c (timer + animation fields)          -> diff_gui_helpers.c
 *   For CSS, append the new rules to the CSS string (or to anteater_poker.css,
 *   which init_gui now prefers to load from disk — see diff_client.c).
 */

/* (gui_assets.h declares no functions — it is data/struct/CSS only.) */

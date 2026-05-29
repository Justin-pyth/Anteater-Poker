/*
 * ============================================================================
 *  MODULE DIFF:  gui_helpers.c / gui_helpers.h
 *                —  Card widgets, timers, animations
 *  ALPHA (gladeUI @200932f)  ->  BETA (test @aa84c01)
 * ============================================================================
 *
 *  TEACHING ARTIFACT — not compilable code.
 *  Prerequisite reading: diff_gui_assets.c (CardDrawData animation fields,
 *  SeatTimer.on_expire).  Much of the alpha gui_extensions.c content was folded
 *  into this file — see diff_removed.c.
 *
 *  Reproduce the raw diff from inside /home/duc/Anteater-Poker/beta:
 *      git diff 200932f aa84c01 -- gui_helpers.c gui_helpers.h
 * ============================================================================
 */


/* $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
 * $   PART 1 — MODIFIED FUNCTIONS                                          $
 * $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$ */

/*
 * MODIFIED FUNCTION: draw_card_cb()  (now split into draw_card_content + draw_card_cb)
 *
 * WHAT IT DOES: The GTK "draw" signal handler for card drawing areas.  Renders a
 * single card face-up or face-down using Cairo.
 *
 * ALPHA: One monolithic draw_card_cb().  Always drew with Cairo text/shapes only.
 *
 * BETA: Refactored into TWO functions:
 *   draw_card_content(cr, w, h, d):
 *     - NEW: tries to load "images/XY.png" via card_image_path() first.
 *     - Falls back to the original Cairo text/shape rendering if PNG missing.
 *   draw_card_cb(widget, cr, data):
 *     - Delegates to draw_card_content() for base rendering.
 *     - NEW: handles two animations before drawing:
 *         deal animation: scale+fade the card in over 300ms when first revealed.
 *         flip animation: x-axis collapse then expand (for showdown reveal).
 *       If an animation is active, applies a Cairo transform and calls
 *       draw_card_content() inside that transform.
 *
 * WHY: image loading belongs in a reusable helper; animation state in CardDrawData
 * drives the tick callbacks, and draw_card_cb adjusts the Cairo transform.
 *
 * HOW TO IMPLEMENT:
 *   Step 1.  Extract the drawing code from draw_card_cb into
 *            static void draw_card_content(cairo_t *cr, int w, int h, CardDrawData *d).
 *   Step 2.  At the start of draw_card_content(), add the image-load block using
 *            gdk_pixbuf_new_from_file_at_scale() + gdk_cairo_set_source_pixbuf().
 *            If the image loads, paint and return; else fall through to Cairo.
 *   Step 3.  In draw_card_cb(), before draw_card_content():
 *     - If deal_cb_id != 0: smoothstep ease from deal_t; cairo_scale +
 *       cairo_push_group + cairo_paint_with_alpha.
 *     - Else if flip_cb_id != 0: cairo_scale(flip_scale, 1.0) centred.
 *     - Else: call draw_card_content() directly.
 */

/*
 * MODIFIED FUNCTION: make_card_widget() / init_card_widget()
 *
 * CHANGE: Two new field inits after g_new0(CardDrawData, 1):
 *   d->deal_t     = 1.0f;   // 1.0 = idle (no animation)
 *   d->flip_scale = 1.0f;   // 1.0 = not collapsed
 *
 * WHY: g_new0 zero-fills the struct, so without this deal_t starts at 0.0, making
 * the deal animation run immediately for every card on startup instead of only
 * when a new card is first revealed.
 *
 * HOW TO IMPLEMENT: after g_new0 in both functions, add the two lines.
 */

/*
 * MODIFIED FUNCTION: set_card_face()
 *
 * WHAT IT DOES: Sets the card's content and triggers a redraw.
 *
 * ALPHA:
 *   d->card = c; d->face_up = face_up; gtk_widget_queue_draw(da);
 *
 * BETA adds transition detection and animation start/cancel:
 *   int transition = face_up && !d->was_face_up;   // back->face transition
 *   d->card = c; d->face_up = face_up;
 *   if (!face_up) {
 *       d->was_face_up = 0; d->deal_t = 1.0f; cancel both animations
 *   } else if (transition) {
 *       d->was_face_up = 1; d->deal_t = 0.0f; d->deal_start_us = 0;
 *       cancel animations; start deal_tick_cb
 *   } // else: already face-up, update card value silently
 *   gtk_widget_queue_draw(da);
 *
 * WHY: the deal animation only makes sense on a hidden->visible transition.  If
 * already face-up and just changed rank, no animation.  If going face-down, all
 * animations cancelled immediately.
 *
 * HOW TO IMPLEMENT:
 *   Step 1.  Detect the back->face transition.
 *   Step 2.  Update card/face_up fields.
 *   Step 3.  Add the three-branch logic (not face-up / transition / steady).
 *   Step 4.  Use gtk_widget_remove_tick_callback to cancel running animations
 *            before starting a new one (avoids dangling callbacks).
 */

/*
 * MODIFIED FUNCTION: set_card_back()
 *
 * ALPHA:
 *   d->face_up = 0; gtk_widget_queue_draw(da);
 *
 * BETA:
 *   d->face_up = 0; d->was_face_up = 0; d->deal_t = 1.0f;
 *   cancel deal_cb_id and flip_cb_id if running
 *   gtk_widget_queue_draw(da);
 *
 * WHY: resetting to a back card must cancel any in-flight animation and reset
 * was_face_up so the NEXT face-up transition triggers the deal animation.
 *
 * HOW TO IMPLEMENT: add the three cleanup lines and the two cancel-if-running
 * blocks (same pattern as set_card_face).
 */

/*
 * MODIFIED FUNCTION: timer_tick_cb()  (in gui_helpers.c)
 *
 * WHAT IT DOES: GLib timeout callback — decrements a seat timer by one second,
 * updates the progress bar and label, fires on_expire when time runs out.
 *
 * ALPHA:
 *   if time expired:
 *       if (t->is_my_timer) send_gui_move(FOLD, 0);
 *       g_free(cb); return G_SOURCE_REMOVE;
 *
 * BETA:
 *   if time expired:
 *       void (*expire)(void) = t->on_expire;   // capture BEFORE g_free
 *       g_free(cb);
 *       if (expire) expire();
 *       return G_SOURCE_REMOVE;
 *
 * ALSO NEW in the non-expired path: timer bar colour change.
 *   float frac = (float)t->seconds_left / (float)t->turn_seconds;
 *   remove "timer-yellow" and "timer-red" CSS classes
 *   if frac <= 0.25: add "timer-red"
 *   else if frac <= 0.50: add "timer-yellow"
 *
 * WHY:
 *   - Function pointer instead of is_my_timer (see diff_gui_assets.c).
 *   - Colour change: visual urgency indicator for the player about to time out.
 *
 * HOW TO IMPLEMENT:
 *   Step 1.  Replace the is_my_timer check with the function-pointer dispatch.
 *            IMPORTANT: capture t->on_expire into a local BEFORE g_free(cb),
 *            because g_free may corrupt the struct.
 *   Step 2.  In the non-expired path, after updating fraction and label, add the
 *            style-context class manipulation.
 */

/*
 * MODIFIED FUNCTION: start_seat_timer()
 *
 * CHANGE: Reset bar to green at timer start by removing CSS classes:
 *   GtkStyleContext *bctx = gtk_widget_get_style_context(t->bar);
 *   gtk_style_context_remove_class(bctx, "timer-yellow");
 *   gtk_style_context_remove_class(bctx, "timer-red");
 *
 * WHY: without the reset, a bar that was previously red (just expired) would stay
 * red when a new timer starts on the same seat.
 *
 * HOW TO IMPLEMENT: add the three lines immediately before gtk_widget_show(t->bar).
 */


/* $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
 * $   PART 2 — NEW FUNCTIONS                                               $
 * $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$ */

/*
 * NEW FUNCTION (static): card_image_path()
 *   static void card_image_path(char *buf, size_t sz, Card card, int face_up)
 *
 * WHAT IT DOES: Fills `buf` with the relative path to the PNG for a card, e.g.
 * "images/ah.png" for Ace of Hearts.  Returns "images/back.png" if face_up==0 or
 * the card is unknown.
 *
 * IMPLEMENTATION GUIDE:
 *   static const char *RANK_FILE[13] = {"2","3","4","5","6","7","8","9","10","j","q","k","a"};
 *   static const char  SUIT_FILE[4]  = {'h','d','c','s'};
 *   if (!face_up || !card_is_known(card)) { snprintf(buf, sz, "images/back.png"); return; }
 *   int ri = rank_index(card.rank);   // TWO=0 ... ACE=12
 *   int si = card.suit - HEARTS;      // HEARTS=0, DIAMONDS=1, CLUBS=2, SPADES=3
 *   snprintf(buf, sz, "images/%s%c.png", RANK_FILE[ri], SUIT_FILE[si]);
 *
 * NOTE: rank_index() converts the rank enum to a 0-based index (subtract 1 since
 * UNKNOW_R=0).  TWO -> "2", JACK -> "j", etc.
 */

/*
 * NEW FUNCTION (GTK tick callback): deal_tick_cb()
 *   static gboolean deal_tick_cb(GtkWidget *widget, GdkFrameClock *clock, gpointer data)
 *
 * WHAT IT DOES: Advances the deal animation (scale 0.85->1, fade in) over 300ms.
 * Each frame increments CardDrawData.deal_t from 0.0 toward 1.0.  When done,
 * clears deal_cb_id and stops the callback.
 *
 * IMPLEMENTATION GUIDE:
 *   Step 1.  On first call, record d->deal_start_us = gdk_frame_clock_get_frame_time(clock).
 *   Step 2.  elapsed = current_time - d->deal_start_us.
 *   Step 3.  d->deal_t = (float)elapsed / DEAL_DURATION_US   (DEAL_DURATION_US = 300000).
 *   Step 4.  If deal_t >= 1.0: clamp to 1.0, clear deal_cb_id, queue draw, return G_SOURCE_REMOVE.
 *   Step 5.  Else: queue draw, return G_SOURCE_CONTINUE.
 *
 * USED BY: set_card_face() on a back->face transition.  Callback ID stored in
 * d->deal_cb_id so it can be cancelled.
 */

/*
 * NEW FUNCTION (GTK tick callback): flip_tick_cb()
 *   static gboolean flip_tick_cb(GtkWidget *widget, GdkFrameClock *clock, gpointer data)
 *
 * WHAT IT DOES: Advances the flip-reveal animation (200ms collapse, 200ms expand)
 * used during showdown.  X-scale goes 1->0 (first half), swaps the card content at
 * the midpoint, then 0->1 (second half).
 *
 * IMPLEMENTATION GUIDE:
 *   Step 1.  Track d->flip_start_us on first call.
 *   Step 2.  First half (!d->flip_midpoint):
 *              t = elapsed / FLIP_HALF_US
 *              if t >= 1: flip_scale=0, flip_midpoint=1, swap card+face, reset start_us
 *              else: flip_scale = 1.0 - t
 *   Step 3.  Second half (d->flip_midpoint):
 *              t = elapsed / FLIP_HALF_US
 *              if t >= 1: flip_scale=1, midpoint=0, clear flip_cb_id, queue draw, return REMOVE
 *              else: flip_scale = t
 *   Step 4.  Queue draw, return CONTINUE.
 *
 * USED BY: flip_card_face().
 */

/*
 * NEW FUNCTION: flip_card_face()
 *   void flip_card_face(GtkWidget *da, Card c)
 *
 * WHAT IT DOES: Triggers a flip-reveal animation on a card widget — x-axis
 * collapse/expand to show the card face-up.  Used during showdown to reveal
 * opponent cards dramatically.
 *
 * DECLARED IN: gui_helpers.h
 *
 * IMPLEMENTATION GUIDE:
 *   Step 1.  Get CardDrawData *d from the widget.
 *   Step 2.  If already showing the same face-up card: return (no-op).
 *   Step 3.  Set d->flip_next_card = c, d->flip_next_face = 1.
 *   Step 4.  Reset d->flip_midpoint = 0, d->flip_start_us = 0, d->flip_scale = 1.0f.
 *   Step 5.  Set d->was_face_up = 1.
 *   Step 6.  Cancel any running deal or flip animation.
 *   Step 7.  Start a new flip_tick_cb via gtk_widget_add_tick_callback().
 *   Step 8.  Queue draw.
 *
 * CALLED FROM: refresh_ui() during showdown when displaying opponent cards.
 */

/*
 * NEW FUNCTIONS: Special card widget family
 *   static void special_image_path(char *buf, size_t sz, int special_type, int face_up)
 *   static gboolean draw_special_cb(GtkWidget *widget, cairo_t *cr, gpointer data)
 *   GtkWidget *make_special_widget(int w, int h)
 *   void       init_special_widget(GtkWidget *da)
 *   void       set_special_face(GtkWidget *da, int special_type, int face_up)
 *   void       set_special_back(GtkWidget *da)
 *
 * WHAT THEY DO: A parallel set of widget creation/drawing functions for "Anteater
 * special" cards (swap, reveal, redraw, instawin, swapops).  Mirrors the regular
 * card widget API but uses a separate SpecialCardDrawData struct and separate PNG
 * paths (images/special_*.png).
 *
 * DECLARED IN: gui_helpers.h
 *
 * SPECIAL TYPES: 0=swap1, 1=swap2, 2=reveal, 3=redraw, 4=instawin, 5=swapops;
 *                -1 = unknown/back.
 *
 * IMPLEMENTATION GUIDE:
 *   Step 1.  typedef struct { int special_type; int face_up; } SpecialCardDrawData;
 *   Step 2.  special_image_path() analogous to card_image_path() but
 *            "images/special_X.png".
 *   Step 3.  draw_special_cb() analogous to draw_card_cb(): try image, fall back
 *            to a purple rounded-rect with a text label.
 *   Step 4.  make_special_widget() / init_special_widget() analogous to the card
 *            versions, using the "special-data" data key.
 *   Step 5.  set_special_face() / set_special_back() analogous.
 *
 * NOTE: special card widgets have no deal/flip animations in beta; they could be
 * added later following the same tick-callback pattern.
 */

/*
 * NEW FUNCTIONS: Turn pulse animation
 *   static gboolean pulse_tick(gpointer data)
 *   void start_turn_pulse(int opp_slot)
 *
 * WHAT THEY DO: Toggle a CSS class "active-pulse" on an opponent's seat frame
 * every 500ms, creating a blinking border to indicate whose turn it is.
 * start_turn_pulse(-1) clears all pulses.
 *
 * DECLARED IN: gui_helpers.h
 *
 * STATIC GLOBALS (file-scope in gui_helpers.c):
 *   static guint pulse_ids  [GUI_OPPONENT_SLOTS] = {0};
 *   static int   pulse_phase[GUI_OPPONENT_SLOTS] = {0};
 *
 * IMPLEMENTATION GUIDE:
 *   pulse_tick(slot):
 *     XOR pulse_phase[slot] with 1.
 *     phase==1 -> add "active-pulse" to W.opp_frame[slot]; else remove it.
 *     Return G_SOURCE_CONTINUE.
 *   start_turn_pulse(slot):
 *     Cancel all existing pulse timers (g_source_remove) and remove the class
 *     from every opp_frame.
 *     If slot < 0 or >= GUI_OPPONENT_SLOTS: return (just clearing).
 *     pulse_phase[slot] = 0.
 *     g_timeout_add(500, pulse_tick, GINT_TO_POINTER(slot)); store ID in pulse_ids[slot].
 *
 * CSS (in gui_assets.h CSS string):
 *   .opp-frame { transition: background-color 300ms ease, border-color 300ms ease; }
 *   .opp-frame.active-pulse { border-color: #ffe066; background-color: rgba(255,224,102,0.18); }
 */

/*
 * NEW FUNCTIONS: Chip win floating popup
 *   static gboolean chip_win_tick(gpointer data)
 *   void start_chip_win_anim(const char *winner_name, uint32_t amount)
 *
 * WHAT THEY DO: Create a gold text popup (e.g. "Alice wins  +$250") that floats
 * upward and fades out over ~1 second at the centre of the game window.
 *
 * DECLARED IN: gui_helpers.h
 *
 * IMPLEMENTATION GUIDE:
 *   start_chip_win_anim(winner_name, amount):
 *     Step 1.  Create a GTK_WINDOW_POPUP (no decorations).
 *     Step 2.  GtkLabel with Pango markup for gold-coloured text.
 *     Step 3.  Wrap in a GtkEventBox named "chip-win-box" for background styling.
 *     Step 4.  Apply CSS via a per-widget GtkCssProvider (dark bg + gold border +
 *              rounded corners).
 *     Step 5.  Position at the centre of W.window via gtk_window_get_position() +
 *              gtk_window_get_size().
 *     Step 6.  Allocate a ChipWin struct (alpha=1.0, y=centre, id from g_timeout_add).
 *     Step 7.  gtk_widget_show_all().
 *   chip_win_tick(cw):
 *     Decrement cw->alpha by 0.025; move cw->y up 1px/tick.
 *     If alpha <= 0 or widget destroyed: destroy popup, g_free(cw), return REMOVE.
 *     gtk_widget_set_opacity() + gtk_window_move().  Return CONTINUE.
 *     (Timeout interval: 30ms = ~33 fps.)
 *
 * CALLED FROM: refresh_ui() on the first frame where showdown==1 and winnerID valid.
 */

/*
 * NEW FUNCTION: sendNameToServer()
 *   void sendNameToServer(const char *name)
 *
 * WHAT IT DOES: Sends a MSG_TYPE_JOIN message carrying the local player's display
 * name.  Called right after on_connect_clicked() transitions to the game screen.
 *
 * DECLARED IN: gui_helpers.h
 *
 * IMPLEMENTATION GUIDE:
 *   if (!C.connected) return;
 *   Message msg;
 *   msg.type      = MSG_TYPE_JOIN;
 *   msg.sender_id = C.my_player_id;
 *   strncpy(msg.chat, name, MAX_PAYLOAD_SIZE - 1);
 *   msg.chat[MAX_PAYLOAD_SIZE - 1] = '\0';
 *   uint8_t buffer[BUFFER_SIZE];
 *   uint32_t len = prepare_payload(buffer, MSG_TYPE_JOIN, &msg);
 *   send_to_server(&C, buffer, len);
 */

/*
 * ============================================================================
 *  MODULE DIFF:  anteater_poker.glade   —  CLIENT UI layout (Glade XML)
 *  ALPHA (gladeUI @200932f)  ->  BETA (test @aa84c01)
 * ============================================================================
 *
 *  TEACHING ARTIFACT — not compilable code.  This documents the CLIENT window's
 *  Glade file (anteater_poker.glade), which client.c -> init_gui() loads at
 *  startup with gtk_builder_new_from_file().  The widget IDs declared here are
 *  the SOURCE OF TRUTH that the AppWidgets struct (gui_assets.h) looks up by
 *  name.  If an ID here does not match the lookup string in client.c, the
 *  pointer comes back NULL and the label/area silently never updates.
 *
 *  This file pairs with diff_gui_assets.c CHANGE 4 (the C-struct side of the
 *  same rename).  Read both together.
 *
 *  Reproduce the raw diff from inside /home/duc/Anteater-Poker/beta:
 *      git diff 200932f aa84c01 -- anteater_poker.glade
 *  Net change: +64 / -132 lines, ~108 <property> lines edited.
 *
 *  (For the SEPARATE server-GUI Glade file, server_gui.glade, see
 *   diff_server_gui.c — that one is brand new in beta, not a modification.)
 * ============================================================================
 */


/* $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
 * $   PART 1 — THE "YOUR HAND INFO" BOX WAS RESTRUCTURED                    $
 * $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$ */

/*
 * This is the single biggest client-UI change between the branches, and it is a
 * BREAKING one: the widget IDs your code looks up changed names.  Note that
 * alpha and beta both edited this region after the common ancestor (5ca712d) but
 * in DIFFERENT directions — this is a true divergence, not "beta = alpha + X".
 * See the README "Commit Summary": alpha's "Added an overall box to edit your
 * hand info w/ css", "Added big labels for stack and call", and "renamed the
 * stack and call labels" commits are the alpha side; beta simplified instead.
 *
 * ALPHA layout (REMOVED in beta):
 *   <object class="GtkEventBox" id="yourInfo_Box">     <- wrapper added for CSS
 *     <object class="GtkBox">
 *       <object class="GtkBox">                        <- "call" caption + value
 *         <object class="GtkLabel"/>                   <- static "Call:" caption
 *         <object class="GtkLabel" id="label_call_amnt"/>
 *       <object class="GtkBox" id="hand_box">
 *         <object class="GtkLabel" id="hand_lbl"/>
 *         <object class="GtkBox"   id="hand_row">
 *           <object class="GtkDrawingArea" id="my_card_0"/>
 *           <object class="GtkDrawingArea" id="my_card_1"/>
 *       <object class="GtkBox">                         <- "stack" caption + value
 *         <object class="GtkLabel"/>                    <- static "Stack:" caption
 *         <object class="GtkLabel" id="label_your_stack"/>
 *
 * BETA layout (ADDED — flatter, no EventBox, self-describing labels):
 *   <object class="GtkBox">
 *     <object class="GtkLabel" id="ammount_to_call"/>   <- was label_call_amnt
 *     <object class="GtkBox"   id="hand_box">
 *       <object class="GtkLabel" id="hand_lbl"/>
 *       <object class="GtkBox"   id="hand_row">
 *         <object class="GtkDrawingArea" id="my_card_0"/>
 *         <object class="GtkDrawingArea" id="my_card_1"/>
 *     <object class="GtkLabel" id="player_ammount"/>    <- was label_your_stack
 *
 * WIDGET-ID RENAMES (verified in code, alpha -> beta):
 *   label_call_amnt   ->  ammount_to_call   (the "amount to call" value label)
 *   label_your_stack  ->  player_ammount    (the local player's chip-stack label)
 *   yourInfo_Box      ->  (deleted; no EventBox wrapper in beta)
 *
 * KEPT (re-parented but same IDs): hand_box, hand_lbl, hand_row, my_card_0,
 *   my_card_1.  The two card GtkDrawingAreas are unchanged in name and purpose.
 *
 * ALSO GONE IN BETA: the alpha-only "big labels" (CSS ids #big_stack_label and
 *   #big_toCall_label, styled in alpha gui_assets.h) have no counterpart in beta.
 *   Do not migrate code that references them.
 *
 * WHY THE NAMES MATTER (the silent-NULL trap):
 *   client.c calls gtk_builder_get_object(builder, "<id>") for each widget and
 *   stores it in W.  If you copy beta's client.c (which asks for "ammount_to_call"
 *   / "player_ammount") but keep alpha's .glade (which only defines
 *   "label_call_amnt" / "label_your_stack"), every lookup returns NULL.  GTK does
 *   not error — refresh_ui() just guards `if (W.player_ammount) {...}` and skips,
 *   so the stack/call labels never update and you waste an afternoon.  The .glade
 *   IDs and the client.c lookup strings MUST be migrated together.
 */


/* $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
 * $   PART 2 — PROPERTY / STYLING EDITS                                     $
 * $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$ */

/*
 * Roughly 108 <property> lines were added/removed across the file (margins,
 * spacing, halign/valign, label text, packing).  These are cosmetic and do not
 * change widget IDs, so they are not individually load-bearing for a migration.
 * The CSS that styles the renamed labels lives in gui_assets.h (and, in beta,
 * also in anteater_poker.css loaded from disk) — see diff_gui_assets.c CHANGE 6
 * (#player_ammount / #ammount_to_call rules) and diff_client.c (init_gui now
 * prefers to load CSS from anteater_poker.css).
 */


/* $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
 * $   HOW TO IMPLEMENT (migration checklist)                               $
 * $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$ */

/*
 * Easiest path: take beta's anteater_poker.glade wholesale (open it once in the
 * Glade editor to confirm it loads), because hand-merging XML is error-prone.
 * If you must merge by hand, do these four edits in lockstep:
 *
 *   Step 1.  In anteater_poker.glade, delete the <object id="yourInfo_Box">
 *            GtkEventBox wrapper and its two caption sub-boxes.
 *   Step 2.  Rename the value labels:
 *              id="label_call_amnt"  -> id="ammount_to_call"
 *              id="label_your_stack" -> id="player_ammount"
 *   Step 3.  In gui_assets.h (AppWidgets), rename the matching pointers
 *            (see diff_gui_assets.c CHANGE 4).
 *   Step 4.  In client.c init_gui(), rename the gtk_builder_get_object lookup
 *            strings to the new IDs (see diff_client.c), and update the CSS
 *            selectors (#player_ammount / #ammount_to_call).
 *
 * VERIFY:  run `./client`, connect, and confirm the "amount to call" and your
 * chip-stack labels actually change as the hand progresses.  tests/test_gui.c
 * (see diff_tests.c) loads this Glade file headlessly and asserts the labels
 * update — `make test` will fail loudly if an ID is wrong.
 */

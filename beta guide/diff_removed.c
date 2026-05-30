/*
 * ============================================================================
 *  MODULE DIFF:  REMOVED FILES — gui_extensions.* and specialCards.*
 *  ALPHA (gladeUI @200932f)  ->  BETA (test @aa84c01)
 * ============================================================================
 *
 *  TEACHING ARTIFACT — not compilable code.
 *  These files existed in alpha but were deleted or stripped in beta.  Read this
 *  if you are migrating code that USED these modules.
 *
 *  Verify the removals from inside /home/duc/Anteater-Poker/beta:
 *      git diff 200932f aa84c01 -- gui_extensions.c gui_extensions.h specialCards.c specialCards.h
 * ============================================================================
 */


/* $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
 * $   REMOVED FILES                                                        $
 * $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$ */

/*
 * REMOVED FILE: gui_extensions.c  +  gui_extensions.h
 *
 * WHAT THEY CONTAINED:
 *   gui_extensions.c was a 434-line "drop-in extra GUI elements" file containing:
 *   - A duplicate SeatTimer struct and duplicate timer_tick_cb() implementation.
 *   - build_anteater_deck_panel(), build_shop_button(), build_chat_box() — widget
 *     builders intended to be called from build_game_screen() with TODOs.
 *   - ext_start_player_timer(), ext_stop_player_timer(), ext_start_my_timer(),
 *     ext_stop_my_timer() — timer wrappers over a separate GuiExtensions struct.
 *   - ext_append_chat(), ext_set_anteater_count() — chat and deck-count helpers.
 *   - EXT_CSS — a duplicate CSS string.
 *
 * WHY REMOVED:
 *   The file was never actually integrated into client.c or gui.c — every function
 *   body had TODO comments for hook-in points.  In beta, all functionality moved
 *   directly into production code:
 *   - Timer logic   -> gui_helpers.c (start_seat_timer, start_my_timer, etc.)
 *   - Anteater deck panel -> anteater_poker.glade directly.
 *   - Chat box      -> anteater_poker.glade.
 *   - CSS           -> consolidated into gui_assets.h's CSS string.
 *
 *   The GuiExtensions struct is gone; the AppWidgets struct (gui_assets.h) is the
 *   single container for all widget references.
 *
 * MIGRATION NOTES FOR JUNIOR DEV:
 *   - If you used any ext_* function, it no longer exists.  Use the gui_helpers.c
 *     equivalent:
 *       ext_start_my_timer(ext, secs)   -> start_my_timer(secs)
 *       ext_stop_my_timer(ext)          -> stop_my_timer()
 *       ext_start_player_timer(ext,i,s) -> start_player_timer(i, s)
 *       ext_stop_player_timer(ext, i)   -> stop_player_timer(i)
 *       ext_append_chat(ext, s, m)      -> append_chat(s, m)   (in gui.c)
 *       ext_set_anteater_count(ext, n)  -> set_anteater_count(n)
 *   - Include "gui_helpers.h" instead of "gui_extensions.h".
 *   - Remove the GuiExtensions ext; variable from any file that declared one.
 *   - Remove the #include "gui_extensions.h" line from any file that had it.
 */

/*
 * REMOVED FILE: specialCards.c
 * RETAINED BUT STRIPPED: specialCards.h
 *
 * WHAT specialCards.c CONTAINED:
 *   39-line stub implementations of swapCard(), swapCards(), redrawCards(),
 *   revealComCard(), revealOppCard(), swapOppCards().  All implementations were
 *   incorrect (using const-pointer reassignment to "swap" values — a common
 *   beginner mistake that compiles but has no effect, because you reassign the
 *   local pointer, not the value it points to).
 *
 * WHY REMOVED:
 *   The special card mechanics were never hooked into the server logic in alpha.
 *   The stub file contained bugs and was unused.  In beta, specialCards.c is
 *   deleted; specialCards.h is kept (stripped to declarations only) as a
 *   placeholder for future implementation.
 *
 * specialCards.h CHANGES (alpha -> beta):
 *   ALPHA: included Rules.h, game.h, time.h, stdio.h, stdlib.h, string.h.
 *          Had a long comment block of rank/suit enum values at the bottom.
 *   BETA:  includes only uds.h.
 *          Declarations changed from pointer params to value types:
 *            alpha: void swapCards(GameState gs, const Card* ownHand, const Card* oppHand)
 *            beta:  void swapCards(GameState gs, PlayerHand ownHand, PlayerHand oppHand)
 *          swapCard() (one card) removed; swapCards() kept.  Comment block removed.
 *
 * MIGRATION NOTES FOR JUNIOR DEV:
 *   - Do NOT implement the old pointer-reassignment approach.  To swap two Card
 *     structs, use value semantics: Card temp = a; a = b; b = temp;
 *   - specialCards.h declares PlayerHand, but that typedef may not exist in uds.h
 *     yet — you may need to add it.
 *   - The special card feature is planned but not implemented.  If implementing,
 *     start with swapCards() using value parameters and test thoroughly before
 *     integrating into the server's action dispatch loop.
 *   - The client-side special card WIDGETS do exist (see diff_gui_helpers.c PART 2,
 *     "Special card widget family") — only the server-side game LOGIC is absent.
 */

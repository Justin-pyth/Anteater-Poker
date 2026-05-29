/*
 * ============================================================================
 *  MODULE DIFF:  rules.c   —  Turn order & blind posting
 *  ALPHA (gladeUI @200932f)  ->  BETA (test @aa84c01)
 * ============================================================================
 *
 *  TEACHING ARTIFACT — not compilable code.
 *  Prerequisite reading: diff_uds.c (blind index/amount GameState fields).
 *
 *  Reproduce the raw diff from inside /home/duc/Anteater-Poker/beta:
 *      git diff 200932f aa84c01 -- rules.c
 * ============================================================================
 */


/* $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
 * $   PART 1 — MODIFIED FUNCTIONS                                          $
 * $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$ */

/*
 * MODIFIED FUNCTION: nextActive()
 *
 * WHAT IT DOES: Starting from a given seat index, finds the next seat whose
 * player is PLAYING or (optionally) READY.  Used to advance turn order.
 *
 * ALPHA:
 *   int next = (curr + 1) % MAX_PLAYERS;
 *   while(next != curr) {
 *       if(status matches) return next;
 *       next = (next + 1) % MAX_PLAYERS;
 *   }
 *   if(gs->players[curr] matches) return curr;
 *   return -1;
 *
 * BETA:
 *   for(int step = 1; step <= MAX_PLAYERS; step++) {
 *       int next = (((curr + step) % MAX_PLAYERS) + MAX_PLAYERS) % MAX_PLAYERS;
 *       if(status matches) return next;
 *   }
 *   return -1;
 *
 * WHY:
 *   - The alpha version has an infinite-loop bug if `curr` is negative (e.g. -1,
 *     or 255 because -1 was stored in a uint8_t).  The double-modulo trick
 *     `(((curr+step)%N)+N)%N` produces a valid index even for negative inputs.
 *   - The alpha version checked the starting seat last as a special case.  The
 *     beta rewrite subsumes that: when step == MAX_PLAYERS the loop checks curr
 *     itself, so the last-resort check is unnecessary.
 *   - A bounded for-loop (steps 1..MAX_PLAYERS) guarantees termination in at most
 *     MAX_PLAYERS iterations under all inputs.
 *
 * HOW TO IMPLEMENT:
 *   Step 1.  Replace the entire while-loop and the two if-statements after it
 *            with the for-loop shown in the BETA version above.
 *   Step 2.  Keep the return -1; at the end for the no-active-player case.
 *   Step 3.  The double-modulo idiom looks odd but is the standard C way to do
 *            modulo with a possibly-negative dividend.  Leave a comment.
 *
 * PITFALL: Do NOT simplify back to a single %.  C's % can return a negative
 * result when the left operand is negative; the double-modulo is required for
 * correctness with stale uint8_t dealer indices.
 */

/*
 * MODIFIED FUNCTION: initBlinds()
 *
 * WHAT IT DOES: At the start of each hand, forces the small-blind and big-blind
 * players to post their mandatory bets.
 *
 * CHANGES:
 *   1. Record blind seats in GameState:
 *        gs->smallBlindIndex = (uint8_t)small;
 *        gs->bigBlindIndex   = (uint8_t)big;
 *   2. Read blind amounts from GameState instead of constants:
 *      ALPHA: uses SMALL_BLIND and BIG_BLIND directly everywhere.
 *      BETA:
 *        uint32_t sb = gs->smallBlind ? gs->smallBlind : SMALL_BLIND;
 *        uint32_t bb = gs->bigBlind   ? gs->bigBlind   : BIG_BLIND;
 *        // then replaces every SMALL_BLIND with sb, BIG_BLIND with bb
 *
 * WHY:
 *   - smallBlindIndex / bigBlindIndex: the GUI needs them to render "[SB]"/"[BB]"
 *     labels.  initBlinds() is the only place these seats are determined.
 *   - Stored values with fallback: the operator can change blind levels live.
 *     `gs->smallBlind ? gs->smallBlind : SMALL_BLIND` means a zero (uninitialised)
 *     value falls back to the compile-time default so old paths don't break.
 *
 * HOW TO IMPLEMENT:
 *   Step 1.  After the `if(small == -1 || big == -1) return;` guard, add:
 *              gs->smallBlindIndex = (uint8_t)small;
 *              gs->bigBlindIndex   = (uint8_t)big;
 *   Step 2.  Add the sb/bb locals using the ternary-with-fallback form.
 *   Step 3.  Find-and-replace in the function body: SMALL_BLIND -> sb,
 *            BIG_BLIND -> bb.  There are 10 occurrences total
 *            (4 x SMALL_BLIND -> sb, 6 x BIG_BLIND -> bb).
 */

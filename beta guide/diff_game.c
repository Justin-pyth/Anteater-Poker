/*
 * ============================================================================
 *  MODULE DIFF:  game.c / game.h   —  Core game logic
 *  ALPHA (gladeUI @200932f)  ->  BETA (test @aa84c01)
 * ============================================================================
 *
 *  TEACHING ARTIFACT — not compilable code.
 *  Prerequisite reading: diff_uds.c (the GameState/Player field additions).
 *
 *  Reproduce the raw diff from inside /home/duc/Anteater-Poker/beta:
 *      git diff 200932f aa84c01 -- game.c game.h
 * ============================================================================
 */


/* $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
 * $   PART 1 — MODIFIED FUNCTIONS                                          $
 * $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$ */

/*
 * MODIFIED FUNCTION: award()
 *
 * WHAT IT DOES: Distributes the pot among winner(s) after a showdown or
 * fold-around.  Multiple winners get equal shares.
 *
 * CHANGE: Beta adds one line after distributing chips:
 *   ALPHA (nothing):
 *     // pot distributed, function ends
 *   BETA:
 *     gs->winnerID = (winnerCount == 1) ? winnerIDs[0] : (MAX_PLAYERS + 1);
 *
 * WHY: The GUI needs to know who won to show the "wins +$X" floating popup
 * (start_chip_win_anim).  Single winner -> record their seat index.
 * Tie -> set winnerID to MAX_PLAYERS+1 as an invalid / "tie" sentinel.
 *
 * HOW TO IMPLEMENT:
 *   Step 1.  Find the loop that calls gs->players[winnerIDs[i]].chips += split;
 *   Step 2.  After the loop body (and the remainder line), add the single line
 *            shown above.  That is the entire change.
 */

/*
 * MODIFIED FUNCTION: resetHand()
 *
 * WHAT IT DOES: Clears per-hand state (community cards, bets, stage) between hands.
 *
 * ALPHA:
 *   gs->dealerIndex = nextActive(gs, gs->dealerIndex, true);
 *   // ... clears pot, currentBet, minRaise, handPlaying
 *
 * BETA:
 *   int newDealer = nextActive(gs, gs->dealerIndex, true);
 *   if (newDealer != -1) gs->dealerIndex = (uint8_t)newDealer;
 *   // guard: if nextActive returns -1 (no active player), keep the old dealer
 *   // to avoid writing 255 into a uint8_t and breaking the next nextActive call
 *   gs->smallBlindIndex = MAX_PLAYERS;  // "none" — until initBlinds() posts them
 *   gs->bigBlindIndex   = MAX_PLAYERS;
 *   // ... clears pot, currentBet, minRaise, handPlaying
 *   gs->showdown = 0;                   // clear showdown state for next hand
 *
 * WHY:
 *   - The -1 guard: nextActive can return -1 if there are no active players.
 *     In alpha, assigning -1 to a uint8_t wraps to 255, making the next
 *     nextActive() loop spin MAX_PLAYERS iterations before returning -1 again.
 *   - smallBlindIndex / bigBlindIndex init: new fields need clearing every hand.
 *   - showdown = 0: so the next hand is not immediately in showdown state.
 *
 * HOW TO IMPLEMENT:
 *   Step 1.  Replace the direct assignment with the two-step guarded version.
 *   Step 2.  Add the two blind-index init lines immediately after.
 *   Step 3.  Add gs->showdown = 0; near the end with the other single-field resets.
 */

/*
 * MODIFIED FUNCTION: resetGame()
 *
 * WHAT IT DOES: Full reset for a new tournament (chips back to INIT_CHIPS,
 * statuses cleared).
 *
 * CHANGE: One line added inside the player loop:
 *   p->place = 0;   // fresh tournament — clear finish ranks
 *
 * WHY: Without this, a second tournament in the same server session would show
 * stale finishing ranks in the scoreboard.
 *
 * HOW TO IMPLEMENT:
 *   Inside the for-loop that sets p->chips and p->status, add: p->place = 0;
 */

/*
 * MODIFIED FUNCTION: newHand()
 *
 * WHAT IT DOES: Starts a new hand — sets PREFLOP stage, deals cards, posts blinds.
 *
 * CHANGE: One line added:
 *   gs->lastActor = MAX_PLAYERS;   // no one has acted yet this hand
 *
 * WHY: lastActor drives the one-tick reveal deferral in hide_card_info_for_others()
 * (protocol.c).  If not cleared at hand start, a stale value could suppress a card
 * reveal when it should not.
 *
 * HOW TO IMPLEMENT:
 *   After the lines that set gs->stage = PREFLOP and gs->handPlaying = true,
 *   add: gs->lastActor = MAX_PLAYERS;
 */

/*
 * MODIFIED FUNCTION: advance()
 *
 * WHAT IT DOES: Moves the game from one stage to the next (PREFLOP->FLOP->TURN
 * ->RIVER->award/showdown).  Called after all players have acted.
 *
 * CHANGE:
 *   ALPHA: gs->minRaise = BIG_BLIND;
 *   BETA:  gs->minRaise = gs->bigBlind ? gs->bigBlind : BIG_BLIND;
 *
 *   RIVER case:
 *   ALPHA:                       BETA:
 *     case RIVER:                  case RIVER:
 *         award(gs);                   award(gs);
 *         resetHand(gs);               gs->showdown = 1;
 *         return;                      gs->handPlaying = false;
 *                                      return;
 *
 * WHY:
 *   - minRaise: uses the configurable big blind so server-side blind changes
 *     are reflected in the minimum raise amount.
 *   - RIVER case: instead of resetting immediately, beta enters showdown state;
 *     cards stay revealed until all players type .ready.  resetHand() is now
 *     called by start_new_hand() (protocol.c) only after everyone is ready.
 *
 * HOW TO IMPLEMENT:
 *   Step 1.  Change the minRaise assignment to the ternary-with-fallback form.
 *   Step 2.  In the RIVER case, replace resetHand() with:
 *              gs->showdown = 1;  gs->handPlaying = false;
 *   Step 3.  Do NOT call resetHand() here anymore.
 */

/*
 * MODIFIED FUNCTION: resolveNoAct()
 *
 * WHAT IT DOES: Handles the edge case where no players have actions left
 * (everyone all-in, or one contestant left).  Runs out the board and awards.
 *
 * CHANGE:
 *   ALPHA:                            BETA:
 *     award(gs);                        award(gs);
 *     gs->handPlaying = false;          gs->showdown = 1;
 *     gs->currentPlayer = MAX_PLAYERS;  gs->handPlaying = false;
 *     return true;                      return true;
 *
 * WHY: Like advance()'s RIVER case, the hand now enters showdown instead of
 * resetting.  currentPlayer = MAX_PLAYERS is no longer needed because showdown
 * state alone drives what the client displays.
 *
 * HOW TO IMPLEMENT:
 *   Step 1.  Add gs->showdown = 1; before gs->handPlaying = false;
 *   Step 2.  Remove the gs->currentPlayer = MAX_PLAYERS; line.
 *
 * NOTE: In beta, processMove() no longer calls resolveNoAct() directly; it calls
 * the new resolveNoActStep() instead (see PART 2 below).
 */

/*
 * MODIFIED FUNCTION: processMove()
 *
 * WHAT IT DOES: After a player's move is validated and applied, decide what
 * happens next (advance stage? end hand? next player?).
 *
 * CHANGES:
 *   1. Record last actor immediately at entry:
 *        gs->lastActor = playerID;
 *   2. When count==1 (only one active player left):
 *      ALPHA:                                   BETA:
 *        gs->players[activeIDs[0]].chips += pot;  gs->players[activeIDs[0]].chips += pot;
 *        gs->handPlaying = false;                 gs->winnerID = activeIDs[0];
 *        gs->currentPlayer = MAX_PLAYERS;         gs->showdown = 1;
 *                                                 gs->handPlaying = false;
 *   3. Call resolveNoActStep() instead of resolveNoAct():
 *      ALPHA: if(resolveNoAct(gs, deck)) return;
 *      BETA:  if(resolveNoActStep(gs, deck)) return;
 *
 * WHY:
 *   - lastActor: must be recorded so hide_card_info_for_others() knows not to
 *     reveal this player's cards on the same broadcast they went all-in.
 *   - winnerID + showdown: a clear winner must be recorded and showdown entered.
 *   - resolveNoActStep: advances exactly one board stage so the server loop can
 *     interleave GUI updates and 1-second delays.
 *
 * HOW TO IMPLEMENT:
 *   Step 1.  Add gs->lastActor = playerID; as the very first line of the function.
 *   Step 2.  In the if(count==1) block, add the two lines and remove
 *            gs->currentPlayer = MAX_PLAYERS;
 *   Step 3.  Change the resolveNoAct() call to resolveNoActStep().
 */


/* $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
 * $   PART 2 — NEW FUNCTIONS                                               $
 * $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$ */

/*
 * NEW FUNCTION: resolveNoActStep()
 *   bool resolveNoActStep(GameState* gs, Deck* deck)
 *
 * WHAT IT DOES: Like resolveNoAct() but advances EXACTLY ONE stage of the board
 * (one set of community cards) instead of running out all remaining cards in a
 * tight loop.  Returns true if it advanced (all-in situation), false if there
 * are still active players who can bet.
 *
 * DECLARED IN: game.h
 *
 * IMPLEMENTATION GUIDE:
 *   Step 1.  Call findActive(gs, activeIDs, false) — note inclAllIn=false.
 *            Returns players who can still bet (excludes all-in players).
 *            If count > 0, there are still betting players: return false.
 *   Step 2.  Call advance(gs, deck) — deals the next community card(s) and
 *            handles the RIVER case (award + showdown flag).
 *   Step 3.  Return true.
 *
 * WHY THIS EXISTS:
 *   The old resolveNoAct() called advance() in a while-loop until the hand was
 *   over, completing the board instantly with no visual delay.  With
 *   resolveNoActStep(), the server loop calls it once per second (via step_runout()
 *   in protocol.c) so players see each community card dealt one at a time.
 *
 *   Example:
 *     bool resolveNoActStep(GameState* gs, Deck* deck) {
 *         int activeIDs[MAX_PLAYERS];
 *         if (findActive(gs, activeIDs, false) > 0) return false;
 *         advance(gs, deck);
 *         return true;
 *     }
 */

/*
 * NEW FUNCTION: updatePlaces()
 *   void updatePlaces(GameState* gs)
 *
 * WHAT IT DOES: Assigns finishing ranks to players who just busted out
 * (chips==0, place==0).  Called from handle_after_move() (protocol.c) after
 * every hand resolves.
 *
 * DECLARED IN: game.h
 *
 * IMPLEMENTATION GUIDE:
 *   Step 1.  Call remainingPlayers(gs) -> count of players still with chips (`alive`).
 *   Step 2.  Loop over all players.  Skip PLAYER_EMPTY and PLAYER_DISCONNECTED.
 *   Step 3.  If a player has chips == 0 AND place == 0 (newly busted):
 *              p->place = (uint8_t)(alive + 1);
 *            `alive + 1` gives the next-worst rank.  If 3 players remain alive and
 *            2 just busted simultaneously, both get place = 4 (= "4th place").
 *            This correctly handles co-busts in a single hand.
 *   Step 4.  After the loop, if alive == 1, find the player with chips > 0 and
 *            set p->place = 1 (the champion).
 *
 * NOTE: place = 0 while a player is still in the tournament.  show_scoreboard()
 * (gui.c) sorts ascending by place and treats 0 as 255, so unranked players
 * sort last.
 */

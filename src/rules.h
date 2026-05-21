#ifndef RULES_H
#define RULES_H

#include "uds.h"

/*  rules.h
    -------
    Pure poker-rule logic. Nothing in this module deals cards, shuffles, or
    drives the hand-lifecycle state machine; that lives in game.c/h. Functions
    here answer questions like "is this move legal?", "whose turn is it?",
    "who has the best hand?", and "is the betting round over?".

    Most functions are read-only (const GameState*). The exceptions are
    initBlinds() and determineWinners() — initBlinds mutates the state to
    post the mandatory bets (the rule itself prescribes a state change), and
    determineWinners writes the winning player IDs into a caller-supplied
    array.
*/

/* ============================================================
    HAND EVALUATION
   ============================================================ */

/*  Returns a score for the best 5-card hand a player can make from their
    2 hole cards combined with the community cards on the board.
    Higher score = better hand; equal score = tie.

    Called once per non-folded player at showdown (server) and thousands of
    times by the bot's Monte Carlo simulation (client).
*/
int evaluateHand(const GameState* gs, const Card* hand);


/* ============================================================
    MOVE LEGALITY
   ============================================================ */

/*  Returns true iff the given move by playerID is legal in the current state.
    Checks include: it's that player's turn, they have a PLAYING status, the
    hand is in progress, and the chip/bet math works out for the requested
    move type (CHECK only when no chips owed, RAISE >= minRaise, etc.).
*/
bool validate(const GameState* gs, uint8_t playerID, MoveType move, uint32_t amount);


/* ============================================================
    TURN ORDER & ACTIVE PLAYERS
   ============================================================ */

/*  Fills activeIDs[] with the seat indices of every PLAYING player (and also
    ALL_IN players when inclAllIn == true). Returns the count written.
    The caller must provide an array of at least MAX_PLAYERS ints.
*/
int findActive(const GameState* gs, int activeIDs[], bool inclAllIn);

/*  Returns the seat index of the next player clockwise from `curr` who is
    currently able to act. When inclReady is true, READY players are also
    accepted (used while a new hand is being set up). Returns -1 if no such
    player exists.
*/
int nextActive(const GameState* gs, int curr, bool inclReady);


/* ============================================================
    BETTING-ROUND RULES
   ============================================================ */

/*  Returns true once every still-playing player has acted in this round AND
    matched the current bet (or is otherwise resolved via fold/all-in).
    Used by the game loop to decide when to advance to the next stage.
*/
bool allPlayersWent(const GameState* gs);

/*  Posts the small and big blinds for the current hand and sets currentBet,
    minRaise, and currentPlayer accordingly. Mutates the game state because
    posting blinds is, by rule, a state change that happens before any
    voluntary action. If a player can't afford their blind, they go all-in.
*/
void initBlinds(GameState* gs);


/* ============================================================
    SHOWDOWN
   ============================================================ */

/*  Evaluates the hands of every still-in player and writes the seat indices
    of the winner(s) into winnerIDs[]. Multiple IDs are written on a tie.
    Returns the number of winners (0 if there are no contenders at all).
    The caller must provide a winnerIDs array of at least MAX_PLAYERS ints.

    Does NOT touch the pot or any chip counts — chip distribution is the
    game module's job (see award() in game.c).
*/
int determineWinners(const GameState* gs, int winnerIDs[]);

#endif

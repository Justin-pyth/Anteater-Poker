#ifndef GAME_H
#define GAME_H

#include "uds.h"
#include <stdbool.h>

/*  SHARED FUNCTIONS (CLIENT + SERVER)

    SERVER calls this once per non-folded player to determine
    the winner after the final betting round. The player with
    the max score should win. Will leave the server to handle
    other outcomes (ties or only 1 non-folded player).

    CLIENT (for bot) calls this thousands of times to evaluate several outcomes
    and return a probablity for each possible move.

    Will need to handle flush, full house, etc.
*/
int evaluateHand(const GameState* gs, const PlayerHand* hand);

//SERVER ONLY FUNCTIONS
void shuffle(Deck* deck);   //shuffle the deck
void deal(GameState* gs, Deck* deck, PlayerHand hand[MAX_PLAYERS]); //deal cards to each player
bool validate(const GameState* gs, MoveType move, uint32_t amount); //check if a move is valid (FOLD, CHECK, CALL, RAISE)
void apply(GameState* gs, uint8_t playerID, MoveType move, uint32_t amount); //apply the move on the server if valid

int findActive(const GameState * gs, int activeIDS[]); //find id array of all active players
void award(GameState*gs); //award players at showdown
int nextActive(GameState* gs, int curr); //find the next active player
void resetHand(GameState* gs); //reset hand after final betting round
/*^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
    The flow of a move will be:
        -client uses sendMove() to the server
        -server uses validate() to make sure client's move is legal
        -server uses apply() to update the game state
        -server sends updated game state to clients
*/

#endif
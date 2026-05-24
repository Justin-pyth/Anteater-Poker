#ifndef GAME_H
#define GAME_H

#include "uds.h"
#include "rules.h"
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*  SHARED FUNCTIONS (CLIENT + SERVER)

    SERVER calls this once per non-folded player to determine
    the winner after the final betting round. The player with
    the max score should win. Will leave the server to handle
    other outcomes (ties or only 1 non-folded player).

    CLIENT (for bot) calls this thousands of times to evaluate several outcomes
    and return a probablity for each possible move.

    Will need to handle flush, full house, etc.
*/
// evaluateHand() moved to rules.h

//SERVER ONLY FUNCTIONS
void initDeck(Deck *deck);  //init the deck once
void initPlayer(Player *p, uint8_t id, const char* name, uint32_t chips);
void shuffle(Deck* deck);   //shuffle all 52 cards back
void shuffleRemaining(Deck* deck, int count); //shuffle only the remaining cards (for bot)
Card deal(Deck* deck); //deals a single card
void dealHoles(GameState* gs, Deck* deck);//deal private cards to each player

// validate() moved to rules.h
void apply(GameState* gs, uint8_t playerID, MoveType move, uint32_t amount); //apply the move on the server if valid

// findActive() and nextActive() moved to rules.h

void award(GameState*gs); //award players at showdown

void resetHand(GameState* gs); //reset hand after final betting round
void resetGame(GameState* gs); //reset hand and then reapply starting chips
void newHand(GameState* gs, Deck* deck); //start the new hand
// initBlinds() moved to rules.h

// allPlayersWent() moved to rules.h
void advance(GameState* gs, Deck* deck); //advance to next stage after all players go
void processMove(GameState* gs, Deck* deck, uint8_t playerID); //after applying move, decide state
bool tryMove(GameState* gs, Deck* deck, uint8_t playerID, MoveType move, uint32_t amount); //for server
bool resolveNoAct(GameState* gs, Deck* deck); //to fix edge case where no players can go
int remainingPlayers(const GameState* gs); //gets # of remaining players w/ chips
/*^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
    The flow of a move will be:
        -client uses sendMove() to the server
        -server uses validate() to make sure client's move is legal
        -server uses apply() to update the game state
        -server sends updated game state to clients
*/

#endif

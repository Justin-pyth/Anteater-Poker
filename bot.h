#ifndef BOT_H
#define BOT_H

#include "game.h"

/* **SHOULD BE DECLARED IN CLIENT.H**
    Send a signal from client -> server for a player's move.
    Amount should be 0 if move is not RAISE.
*/
void sendMove(int socket, MoveType move, int amount);

double monteCarloSim(const GameState gs, const Deck deck, const PlayerHand* botHand, uint8_t botID, int simCount);
MoveType decide(const GameState* gs, int socket);
int checkRemainingCards(const Deck* deck);
void reconstructDeck(const GameState* gs, const PlayerHand* hand, Deck* deck);

#endif
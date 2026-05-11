#ifndef BOT_H
#define BOT_H

#include "game.h"

/* **SHOULD BE DECLARED IN CLIENT.H**
    Send a signal from client -> server for a player's move.
    Amount should be 0 if move is not RAISE.
*/
void sendMove(int socket, MoveType move, int amount);

double monteCarloSim(const GameState* gs, int simCount);
MoveType decide(const GameState* gs, int socket);
void checkRemainingCards(const GameState* gs);

#endif
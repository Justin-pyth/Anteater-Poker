#ifndef BOT_H
#define BOT_H

#include "game.h"

/* **SHOULD BE DECLARED IN CLIENT.H**
    Send a signal from client -> server for a player's move.
    Amount should be 0 if move is not RAISE.
*/
void sendMove(int socket, MoveType move, int amount);

double monteCarloSim(const GameState* gs, const Deck* deck, const Card* botHand, int simCount);
MoveType decide(const GameState* gs, const Deck* deck);
int reconstructDeck(const GameState* gs, const Card* hand, Deck* deck);

bool isBot(const char* name);
void addBot(GameState* gs);
void botMove(const GameState* gs, const Deck* deck, uint8_t* botID, MoveType* move, uint32_t* amount);

bool doOneBotTurn(GameState* gs, Deck* deck);
void doBotTurn(GameState* gs, Deck* deck);

#endif

#ifndef GAME_H
#define GAME_H

#include "uds.h"

typedef struct {
    Player players[MAX_PLAYERS];
    uint8_t playerCount;

    Card community[5];
    uint8_t communityCount;  // how many community cards are revealed

    uint8_t stage;           // use Stage enum
    uint8_t currentPlayer;
    uint8_t dealerIndex;
    uint8_t yourIndex;

    uint32_t pot;
    uint32_t currentBet;     // bet needed to match
    uint32_t minRaise;
} GameState;

/*  SERVER calls this once per non-folded player to determine
    the winner after the final betting round. The player with
    the max score should win. Will leave the server to handle
    other outcomes (ties or only 1 non-folded player).

    BOT calls this thousands of times to evaluate several outcomes
    and return a probablity for each possible move.

    Will need to handle flush, full house, etc.
*/
int evaluateHand(const GameState* gs, Player player);

#endif
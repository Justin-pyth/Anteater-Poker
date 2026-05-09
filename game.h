#ifndef GAME_H
#define GAME_H

#include "uds.h"

/*  SERVER calls this once per non-folded player to determine
    the winner after the final betting round. The player with
    the max score should win. Will leave the server to handle
    other outcomes (ties or only 1 non-folded player).

    BOT calls this thousands of times to evaluate several outcomes
    and return a probablity for each possible move.

    Will need to handle flush, full house, etc.
*/
int evaluateHand(const GameState* gs, const PlayerHand* hand);

#endif
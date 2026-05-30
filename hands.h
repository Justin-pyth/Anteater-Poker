// Declaration of all the different hands in poker
#ifndef HANDS_H
#define HANDS_H

#include "uds.h"

void determineHands (GameState* gs, PlayerHand hand, CommunityCards comCards);
void highHand (GameState* gs, PlayerHand hand, CommunityCards comCards);

#endif
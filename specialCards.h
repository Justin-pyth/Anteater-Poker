// Special Moves that differ from base Texas Hold-em
#ifndef SPECIALCARDS_H
#define  SPECIALCARDS_H

#include "uds.h"
#include "rules.h"
#include "game.h"
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Anteater shop prices. INSTAWIN is 75% of the pot.
#define PRICE_SWAP1    300
#define PRICE_SWAP2    400
#define PRICE_REVEAL   200
#define PRICE_REDRAW   250
#define PRICE_SWAPOPS  150

//Swap both of your cards with an opponent of your choosing
void swapCard(GameState *gs, int myID, int oppID);

//Swap one of your cards with an opponent of your choosing
void swapCards(GameState *gs, int myID, int myCardIdx, int oppID, int oppCardIdx);

//Redraw one of your cards from the deck
void redrawCards(GameState *gs, Deck *deck, int myID, int myCardIdx);

//Reveal the next community card
void revealComCard(GameState *gs, Deck *deck);

//Reveal one of your opponents cards in exchange for one of yours
void revealOppCard(GameState *gs, int oppID, int oppCardIdx);

//Swap your opponent's cards 
void swapOppCards(GameState *gs, int oppID);

//Win game (pay 75% of total prize pool)
void instaWin(GameState *gs, int myID);

// Anteater shop: charge the buyer for `card` and run the matching action above.
// `target` is the opponent's id (SWAP1/SWAP2/SWAPOPS); `myCardIdx`/`oppCardIdx`
// pick which cards to swap/redraw (SWAP1, REDRAW). Returns false (charging
// nothing) if the purchase is illegal or unaffordable.
bool buyPowerup(GameState *gs, Deck *deck, uint8_t playerID, Anteater_shop card,
                uint8_t target, uint8_t myCardIdx, uint8_t oppCardIdx);

#endif

/*
UNKNOW_R = 0
TWO = 1
THREE = 2
FOUR = 3
FIVE = 4
SIX = 5
SEVEN = 6
EIGHT = 7
NINE = 8
TEN = 9
JACK = 10
QUEEN = 11
KING = 11
ACE = 12

UNKNOW_S = 0
HEARTS = 1
DIAMONDS = 2
CLUBS = 3
SPADES = 4
*/
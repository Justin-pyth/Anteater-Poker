// Special Moves that differ from base Texas Hold-em
#ifndef SPECIALCARDS_H
#define  SPECIALCARDS_H

#include "uds.h"
#include "Rules.h"
#include "game.h"
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//Swap both of your cards with an opponent of your choosing
void swapCard (GameState gs, const Card* ownHand, const Card* oppHand);

//Swap one of your cards with an opponent of your choosing
void swapCards (GameState gs, const Card* ownHand, const Card* oppHand);

//Redraw one of your cards from the deck
void redrawCards (GameState gs, const Card* ownHand, Deck* deck);

//Reveal the next community card
void revealComCard (GameState gs, const Card* ownHand, Deck* community);

//Reveal one of your opponents cards in exchange for one of yours
void revealOppCard (GameState gs, const Card* oppHand);

//Swap your opponent's cards 
void swapOppCards (GameState gs, const Card* oppHand1, const Card* oppHand2);

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

#endif
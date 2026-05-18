// Special Moves that differ from base Texas Hold-em
#ifndef SPECIALCARDS_H
#define  SPECIALCARDS_H

#include "uds.h"
#include "Rules.h"
#include "game.h"

//Swap one or both of your cards with an opponent of your choosing
void swapCards (GameState gs, PlayerHand ownHand, PlayerHand oppHand);

//Redraw one of your cards from the deck
void redrawCards (GameState gs, PlayerHand ownHand, Deck deck);

//Reveal the next community card
void revealComCard (GameState gs, PlayerHand ownHand, CommunityCards community);

//Reveal one of your opponents cards in exchange for one of yours
void revealOppCard (GameState gs, PlayerHand ownHand, PlayerHand oppHand);

//Swap your opponent's cards
void swapOppCards (GameState gs, PlayerHand oppHand1, PlayerHand oppHand2);


#endif
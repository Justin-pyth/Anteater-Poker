#include "specialCards.h"

///Swap both of your cards with an opponent of your choosing
void swapCard(GameState *gs, int myID, int oppID){
    Card temp0 = gs->players[myID].hand[0];
    Card temp1 = gs->players[myID].hand[1];
    gs->players[myID].hand[0] = gs->players[oppID].hand[0];
    gs->players[myID].hand[1] = gs->players[oppID].hand[1];
    gs->players[oppID].hand[0] = temp0;
    gs->players[oppID].hand[1] = temp1;
}

//Swap one of your cards with an opponent of your choosing
void swapCards(GameState *gs, int myID, int myCardIdx, int oppID, int oppCardIdx){
    Card temp = gs->players[myID].hand[myCardIdx];
    gs->players[myID].hand[myCardIdx] = gs->players[oppID].hand[oppCardIdx];
    gs->players[oppID].hand[oppCardIdx] = temp;
}

//Redraw one of your cards from the deck
void redrawCards(GameState *gs, Deck *deck, int myID, int myCardIdx){
    if (deck->top >= DECK_SIZE + SPECIAL_CARDS) return; // deck empty
    gs->players[myID].hand[myCardIdx] = deck->cards[deck->top];
    deck->top++;
}

//Reveal the next community card
void revealComCard(GameState *gs, Deck *deck){
    if (gs->communityCount >= 5) 
        return;
    gs->community[gs->communityCount] = deck->cards[deck->top];
    gs->communityCount++;
    deck->top++;
}

//Reveal one of your opponents cards in exchange for one of yours
void revealOppCard(GameState *gs, int oppID, int oppCardIdx){
    // Server-side: the card is already known; this just flags it visible to caller
    // Nothing to change in GameState — handled by broadcast logic
    (void)gs; (void)oppID; (void)oppCardIdx;
}

//Swap your opponent's cards 
void swapOppCards(GameState *gs, int oppID){
    Card temp = gs->players[oppID].hand[0];
    gs->players[oppID].hand[0] = gs->players[oppID].hand[1];
    gs->players[oppID].hand[1] = temp;
}

// Instant win (server validates payment of 75% of pot)
void instaWin(GameState *gs, int myID) {
    gs->gameOver = 1;
    gs->winnerID = myID;
}
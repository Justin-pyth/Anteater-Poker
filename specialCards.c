#include "specialCards.h"

///Swap both of your cards with an opponent of your choosing
void swapCard (GameState gs, const Card* ownHand1, const Card* ownHand2, const Card* oppHand1,const Card* oppHand2){
    Card* temp1 = ownHand1;
    Card* temp2 = ownHand2;
    ownHand1 = oppHand1;
    ownHand2 = oppHand2;
    oppHand1 = temp1;
    oppHand2 = temp2;
}

//Swap one of your cards with an opponent of your choosing
void swapCards (GameState gs, const Card* ownHand, const Card* oppHand){
    Card* temp = ownHand;
    ownHand = oppHand;
    oppHand = temp;
}

//Redraw one of your cards from the deck
void redrawCards (GameState gs, const Card* ownHand, Deck* deck){
    ownHand = deck->top;
}

//Reveal the next community card
void revealComCard (GameState gs, const Card* ownHand, Deck* community){

}


//Reveal one of your opponents cards in exchange for one of yours
void revealOppCard (GameState gs, const Card* ownHand, const Card* oppHand){

}

//Swap your opponent's cards 
void swapOppCards (GameState gs, const Card* oppHand1, const Card* oppHand2){

}
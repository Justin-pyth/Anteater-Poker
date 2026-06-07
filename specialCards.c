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

//Sabotage a target opponent: replace both their hole cards with fresh deck draws
void swapOppCards(GameState *gs, Deck *deck, int oppID){
    if (deck->top + 2 > DECK_SIZE + SPECIAL_CARDS) return; // not enough cards left
    gs->players[oppID].hand[0] = deck->cards[deck->top++];
    gs->players[oppID].hand[1] = deck->cards[deck->top++];
}

// Instant win (server validates payment of 75% of pot)
void instaWin(GameState *gs, int myID) {
    gs->gameOver = 1;
    gs->winnerID = myID;
}

void init_shop_slots(GameState *gs)
{
    static const Anteater_shop slots[6] = {
        SWAP1, SWAP2, REVEAL, REDRAW, INSTAWIN, SWAPOPS
    };
    for (int i = 0; i < 6; i++)
        gs->cardPrice[i] = slots[i];
}

bool shop_window_open(const GameState *gs)
{
    //usable throughout a live hand (every street); callers additionally gate on
    //it being the buyer's turn so the shop can be used any number of times.
    return gs->handPlaying;
}

bool buyPowerup(GameState *gs, Deck *deck, uint8_t playerID, Anteater_shop card,
                uint8_t target, uint8_t myCardIdx, uint8_t oppCardIdx)
{
    if (!shop_window_open(gs)) {
        fprintf(stderr, "buyPowerup reject: no hand in progress\n");
        return false;
    }

    //TEMP: INSTAWIN disabled — keep the cost/instaWin() code below for re-enabling
    if (card == INSTAWIN) {
        fprintf(stderr, "buyPowerup reject: INSTAWIN disabled\n");
        return false;
    }

    //buyer must be a live player, and only on their own turn
    if (playerID >= MAX_PLAYERS) {
        fprintf(stderr, "buyPowerup reject: bad playerID %u\n", playerID);
        return false;
    }
    if (gs->currentPlayer != playerID) {
        fprintf(stderr, "buyPowerup reject: not your turn (current=%u, you=%u)\n",
                gs->currentPlayer, playerID);
        return false;
    }
    Player *p = &gs->players[playerID];
    if (p->status != PLAYER_PLAYING) {
        fprintf(stderr, "buyPowerup reject: player %u status %u not PLAYING\n",
                playerID, p->status);
        return false;
    }
    if (myCardIdx >= HAND_SIZE || oppCardIdx >= HAND_SIZE) {
        fprintf(stderr, "buyPowerup reject: bad card index (my=%u opp=%u)\n",
                myCardIdx, oppCardIdx);
        return false;
    }

    //target must be a real opponent holding cards (target-based cards)
    bool needsTarget = (card == SWAP1 || card == SWAP2 || card == SWAPOPS);
    if (needsTarget) {
        if (target >= MAX_PLAYERS || target == playerID) {
            fprintf(stderr, "buyPowerup reject: bad target %u (you=%u)\n", target, playerID);
            return false;
        }
        if (!gs->players[target].has_cards) {
            fprintf(stderr, "buyPowerup reject: target %u has no cards\n", target);
            return false;
        }
    }

    //cost, INSTAWIN is 75% of the pot, everything else is a flat price
    uint32_t cost;
    switch (card) {
        case SWAP1:    cost = PRICE_SWAP1;     break;
        case SWAP2:    cost = PRICE_SWAP2;     break;
        case REVEAL:   cost = PRICE_REVEAL;    break;
        case REDRAW:   cost = PRICE_REDRAW;    break;
        case SWAPOPS:  cost = PRICE_SWAPOPS;   break;
        case INSTAWIN: cost = gs->pot * 3 / 4; break;
        default:       return false;
    }
    if (p->chips < cost) {
        fprintf(stderr, "buyPowerup reject: can't afford (chips=%u < cost=%u)\n",
                p->chips, cost);
        return false;
    }

    p->chips -= cost;
    switch (card) {
        case SWAP1:    swapCards(gs, playerID, myCardIdx, target, oppCardIdx); break;
        case SWAP2:    swapCard(gs, playerID, target);                         break;
        case REVEAL:   revealComCard(gs, deck);                                break;
        case REDRAW:   redrawCards(gs, deck, playerID, myCardIdx);             break;
        case INSTAWIN: instaWin(gs, playerID);                                 break;
        case SWAPOPS:  swapOppCards(gs, deck, target);                         break;
    }
    return true;
}
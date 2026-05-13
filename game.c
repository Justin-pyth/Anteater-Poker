#include "game.h"

int evaluateHand(const GameState* gs, const PlayerHand* hand)
{
    (void)gs;
    (void)hand;
    return 0;
}
void shuffle(Deck* deck)
{
    (void)deck;
}
void deal(GameState* gs, Deck* deck, PlayerHand hand[MAX_PLAYERS])
{
    (void)gs;
    (void)deck;
    (void)hand;
}
bool validate(const GameState* gs, MoveType move, uint32_t amount)
{
    (void)gs;
    (void)move;
    (void)amount;
    return true;
}
void apply(GameState* gs, uint8_t playerID, MoveType move, uint32_t amount)
{
    (void)gs;
    (void)playerID;
    (void)move;
    (void)amount;
}

int findActive(const GameState * gs, int activeIDS[])
{   
    int count = 0;
    for(int i = 0 ; i < gs->playerCount ; i++)
        if(gs->players[i].status == ALL_IN || gs->players[i].status == PLAYER_PLAYING)
            activeIDS[count++] = gs->players[i].id;

    return count;
}

void award(GameState* gs)
{
    int activeIDS[MAX_PLAYERS];
    int count = findActive(gs, activeIDS);

    int split = gs->pot/count;
    for(int i = 0 ; i < count ; i++)
        if(gs->players[i].id == activeIDS[i])
            gs->players[i].chips += split;
            
}

void resetHand(GameState* gs)
{
    //reset each player
    for(int i = 0 ; i < MAX_PLAYERS; i++)
    {
        Player* p = &gs->players[i];

        //check if player exists
        if(p->status == PLAYER_EMPTY || p->status == PLAYER_DISCONNECTED) continue;
        //check if player can continue playing
        if(p->chips == 0) p->status = PLAYER_SPECTATING;
        else p->status = PLAYER_READY;
        
        //reset bets and cards
        p->current_bet = 0; p->has_cards = 0;
        memset(&p->hand, 0, sizeof(PlayerHand));

    }

    //reset gamestate values

    memset(gs->community, 0, sizeof(gs->community));
    gs->communityCount = 0;

    gs->dealerIndex = nextActive(gs, gs->dealerIndex); //new dealer

    gs->pot = 0;
    gs->currentBet = 0;
    gs->minRaise = 0;

    gs->handPlaying = false;
    
}

int nextActive(GameState* gs, int curr)
{
    //move to next player in a circle
    int next = (curr + 1) % MAX_PLAYERS;
    while(next != curr)
    {
        //if the player is active, return index
        if(gs->players[next].status == PLAYER_PLAYING || gs->players[next].status == PLAYER_READY)
            return next;

        //otherwise check the next player
        next = (next + 1) % MAX_PLAYERS;
    }
    return curr; //default
}
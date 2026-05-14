#include "game.h"

int evaluateHand(const GameState* gs, const PlayerHand* hand)
{
    (void)gs;
    (void)hand;
    return 0;
}

void initDeck(Deck *deck) 
{

    int i = 0;
    
    for (int suit = HEARTS; suit <= SPADES; suit++) 
    {
        for (int rank = TWO; rank <= ACE; rank++) 
        {
            deck->cards[i].suit = suit;
            deck->cards[i].rank = rank;
            deck->cards[i].value = rank;
            i++;
        }
    }

    deck->top = 0;

}

void shuffle(Deck* deck)
{
    deck->top = 0;
    for(int i = DECK_SIZE - 1; i>0; i--)
    {
        //generate a random index
        int j = rand() % (i+1);

        //swap cards at the random index
        Card temp = deck->cards[j];
        deck->cards[j] = deck->cards[i];
        deck->cards[i] = temp;
    }

}

Card deal(Deck* deck)
{
    return deck->cards[deck->top++];
}

void dealHoles(GameState* gs, Deck* deck)
{
    for(int i = 0; i < MAX_PLAYERS; i++)
    {
        Player *p = &gs->players[i];
        if(p->status != PLAYER_READY) continue; //check if player is ready
        
        //deal both cards
        p->hand.hand[0] = deal(deck); p->hand.hand[1] = deal(deck);
        p->has_cards = 1;
        p->status = PLAYER_PLAYING;
    }
}

bool validate(const GameState* gs, uint8_t playerID, MoveType move, uint32_t amount)
{
    if(playerID >= MAX_PLAYERS) return false;
    if(playerID != gs->currentPlayer) return false;
    if(!gs->handPlaying) return false;
    
    const Player *p = &gs->players[playerID];
    if(p->status != PLAYER_PLAYING) return false;
    switch(move)
    {
        case FOLD:
        {
            return true;
        }
        case CHECK:
        {
            return p->current_bet == gs->currentBet;
        }
        case CALL:
        {
            if(p->current_bet >= gs->currentBet)
                return false;

            return p->chips >= gs->currentBet - p->current_bet;
        }
        case RAISE:
        {
            if(amount < gs->minRaise)
                return false;
            
                return p->chips >= (gs->currentBet+amount) - p->current_bet;
        }
        case ALL_IN:
        {
            return p->chips > 0;
        }
        default:
            return false;
    }

}
void apply(GameState* gs, uint8_t playerID, MoveType move, uint32_t amount)
{
    
    /***MIGHT NEED TO BE CHANGED DEPENDING ON HOW PLAYER ID IS MAPPED 
        ALSO ASSUMES THAT A MOVE IS ALREADY VALID
    ***/
    Player *p = &gs->players[playerID];
    switch(move)
    {
        case FOLD:
        {
            p->status = PLAYER_FOLDED;
            break;
        }
        case CHECK:
        {
            break;
        }
        case CALL:
        {
            uint32_t call = gs->currentBet - p->current_bet;

            p->chips -= call;
            p->current_bet += call;
            gs->pot += call;

            if(p->chips == 0)
                p->status = PLAYER_ALL_IN;

            break;
        }
        case RAISE:
        {
            uint32_t total = gs->currentBet + amount;
            uint32_t chipsPotted = total - p->current_bet;
            p->chips -= chipsPotted;
            p->current_bet = total;
            gs->pot += chipsPotted;

            gs->minRaise = amount;
            gs->currentBet = total;

            //other players need to bet again
            for(int i = 0; i < MAX_PLAYERS ; i++)
            {
                const Player *p = &gs->players[i];
                if(p->status == PLAYER_PLAYING && i != playerID)//might need to change id condition
                    gs->acted[i] = false;
            }

            if(p->chips == 0)
                p->status = PLAYER_ALL_IN;

            break;
        }
        case ALL_IN:
        {
            uint32_t allIn = p->chips;

            p->chips = 0;
            p->current_bet += allIn;
            gs->pot += allIn;
            p->status = PLAYER_ALL_IN;

            if(p->current_bet > gs->currentBet)
            {
                uint32_t raise = p->current_bet-gs->currentBet;

                gs->currentBet = p->current_bet;
                gs->minRaise = raise;

                //other players need to bet again
                for(int i = 0; i < MAX_PLAYERS ; i++)
                {
                    const Player *p = &gs->players[i];
                    if(p->status == PLAYER_PLAYING && i != playerID)//might need to change id condition
                        gs->acted[i] = false;
                }

            }

            break;
        }

    }

    gs->acted[playerID] = true;

}

int nextActive(GameState* gs, int curr, bool inclReady)
{
    //move to next player in a circle
    int next = (curr + 1) % MAX_PLAYERS;
    while(next != curr)
    {
        //if the player is active, return index
        if(gs->players[next].status == PLAYER_PLAYING || (inclReady && gs->players[next].status == PLAYER_READY))
            return next;

        //otherwise check the next player
        next = (next + 1) % MAX_PLAYERS;
    }
    
    if(gs->players[curr].status == PLAYER_PLAYING || (inclReady && gs->players[curr].status == PLAYER_READY))
        return curr;
        
    return -1; //return if no one found
}

int findActive(const GameState * gs, int activeIDs[], bool inclAllIn)
{   
    int count = 0;
    for(int i = 0 ; i < MAX_PLAYERS ; i++)
        if(gs->players[i].status == PLAYER_PLAYING || (inclAllIn && gs->players[i].status == PLAYER_ALL_IN))
            activeIDs[count++] = i; //maybe change to gs->players[i].id

    return count;
}

void award(GameState* gs)
{
    int activeIDs[MAX_PLAYERS];
    int count = findActive(gs, activeIDs, true);
    if(count == 0) return;


    int bestScore = -1, score, winnerCount = 0;
    int winnerIDs[MAX_PLAYERS];
    for(int i = 0 ; i < count; i++)
    {
        int pID = activeIDs[i];
        const Player* p = &gs->players[pID];

        score = evaluateHand(gs, &p->hand);

        if(score > bestScore)
        {
            bestScore = score;
            winnerCount = 0;
            winnerIDs[winnerCount++] = pID;
        }
        else if(score == bestScore)
        {
            winnerIDs[winnerCount++] = pID;
        }
            
    }


    int split = gs->pot / winnerCount;
    int remainder = gs->pot % winnerCount;
    for(int i = 0 ; i < winnerCount ; i++)
        gs->players[winnerIDs[i]].chips += split;
    gs->players[winnerIDs[0]].chips += remainder; //may change, just give to first winner for now
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

        gs->acted[i] = false;

    }

    //reset gamestate values

    memset(gs->community, 0, sizeof(gs->community));
    gs->communityCount = 0;

    gs->dealerIndex = nextActive(gs, gs->dealerIndex, true); //new dealer

    gs->pot = 0;
    gs->currentBet = 0;
    gs->minRaise = 0;

    gs->handPlaying = false;
    
}

void initBlinds(GameState* gs)
{
    int small   = nextActive(gs, gs->dealerIndex, false);
    int big     = nextActive(gs, small, false);

    if(small == -1 || big == -1) return; //no active players found

    if(gs->players[small].chips <= SMALL_BLIND)
    {
        //player goes all in if they can't afford small blind
        gs->pot += gs->players[small].chips;
        gs->players[small].current_bet = gs->players[small].chips;
        gs->players[small].chips = 0;
        gs->players[small].status = PLAYER_ALL_IN;
    }
    else
    {
        //player can afford small blind
        gs->players[small].chips -= SMALL_BLIND;
        gs->players[small].current_bet = SMALL_BLIND;
        gs->pot += SMALL_BLIND;
    }

    if(gs->players[big].chips <= BIG_BLIND)
    {
        //player goes all in if they can't afford big blind
        gs->pot += gs->players[big].chips;
        gs->players[big].current_bet = gs->players[big].chips;
        gs->players[big].chips = 0;
        gs->players[big].status = PLAYER_ALL_IN;
    }
    else
    {
        //player can afford big blind
        gs->players[big].chips -= BIG_BLIND;
        gs->players[big].current_bet = BIG_BLIND;
        gs->pot += BIG_BLIND;
    }

    gs->currentBet = BIG_BLIND;
    gs->minRaise = BIG_BLIND;
    
    //add rest of texas holdem conditions later
    gs->currentPlayer = nextActive(gs, big, false);
}

void newHand(GameState* gs, Deck* deck)
{
    shuffle(deck);
    dealHoles(gs, deck);

    //set to Preflop stage
    gs->stage = PREFLOP; gs->handPlaying = true;

    //set current player after blinds
    initBlinds(gs);
    resolveNoAct(gs, deck);
}

bool allPlayersWent(const GameState* gs)
{
    for(int i = 0; i < MAX_PLAYERS ; i++)
    {
        const Player *p = &gs->players[i];
        if(p->status != PLAYER_PLAYING) continue;

        if((!gs->acted[i]) || (p->current_bet < gs->currentBet)) return false;
    }
    return true;
}

void advance(GameState* gs, Deck* deck)
{
    gs->currentBet = 0;
    gs->minRaise = BIG_BLIND;
    
    for(int i = 0 ; i < MAX_PLAYERS; i++)
    {
        Player *p = &gs->players[i];

        gs->acted[i] = false;
        p->current_bet = 0;
    }

    switch (gs->stage)
    {
        case PREFLOP:
        {
            gs->community[gs->communityCount++] = deal(deck);
            gs->community[gs->communityCount++] = deal(deck);
            gs->community[gs->communityCount++] = deal(deck);
            gs->stage = FLOP;
            break;
        }
        case FLOP:
        {
            gs->community[gs->communityCount++] = deal(deck);
            gs->stage = TURN;
            break;
        }
        case TURN:
        {
            gs->community[gs->communityCount++] = deal(deck);
            gs->stage = RIVER;
            break;
        }
        case RIVER:
        {
            award(gs);
            resetHand(gs);
            return;
        }
    }

    gs->currentPlayer = nextActive(gs, gs->dealerIndex, false);
}

bool resolveNoAct(GameState* gs, Deck* deck)
{
    int activeIDs[MAX_PLAYERS];

    if(findActive(gs, activeIDs, false) > 0)
        return false;

    while(gs->stage != RIVER)
        advance(gs, deck);

    award(gs);
    resetHand(gs);
    return true;
}


void processMove(GameState* gs, Deck* deck, uint8_t playerID)
{
    int activeIDs[MAX_PLAYERS];
    int count = findActive(gs, activeIDs, true);

    //check if everyone except 1 folded (or left)
    if(count == 1)
    {
        gs->players[activeIDs[0]].chips += gs->pot;
        resetHand(gs);
        return;
    }

    if(resolveNoAct(gs, deck))
        return;

    //if everyone went, then advance to next stage
    if(allPlayersWent(gs))
    {
        advance(gs, deck);
        return;
    }

    //next player's turn if normal move and nothing else occurs
    gs->currentPlayer = nextActive(gs, playerID, false);

}

bool tryMove(GameState* gs, Deck* deck, uint8_t playerID, MoveType move, uint32_t amount)
{
    if(!validate(gs, playerID, move, amount))
        return false;

    apply(gs, playerID, move, amount);
    processMove(gs, deck, playerID);

    return true;
}

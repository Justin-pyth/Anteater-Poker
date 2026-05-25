#include "game.h"

// score5() and evaluateHand() moved to rules.c

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

void initPlayer(Player *p, uint8_t id, const char* name, uint32_t chips)
{
    memset(p, 0, sizeof(Player));
    p->id = id;

    p->chips = chips;
    p->has_cards = 0;
    p->status = PLAYER_READY; //for bots by default

    //copy the name into the player struct
    strncpy(p->name, name, MAX_NAME_LENTH - 1);
    p->name[MAX_NAME_LENTH - 1] = '\0'; //manually add termination char

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

void shuffleRemaining(Deck* deck, int count)
{
    for(int i = count - 1; i>deck->top; i--)
    {
        //generate a random index
        int j = deck->top + rand() % (i - deck->top + 1);

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
        p->hand[0] = deal(deck); p->hand[1] = deal(deck);
        p->has_cards = 1;
        p->status = PLAYER_PLAYING;
    }
}

// validate() moved to rules.c

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
        case USE_SPECIAL_CARD:
        {
            //Fill in l8r
        }

    }

    gs->acted[playerID] = true;

}

// nextActive() and findActive() moved to rules.c

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

        score = evaluateHand(gs, p->hand);

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
        memset(&p->hand, 0, sizeof(p->hand));

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

void resetGame(GameState* gs)
{
    resetHand(gs);
    
    //reset gameover
    gs->gameOver = 0;
    gs->winnerID = MAX_PLAYERS+1;

    for (int i = 0; i < MAX_PLAYERS; i++) 
    {
        Player* p = &gs->players[i];

        if (p->status == PLAYER_EMPTY || p->status == PLAYER_DISCONNECTED)
            continue;

        p->chips = INIT_CHIPS;
        p->status = PLAYER_CONNECTED;
    }
}

// initBlinds() moved to rules.c

void newHand(GameState* gs, Deck* deck)
{
    shuffle(deck);
    dealHoles(gs, deck);

    //set to Preflop stage
    gs->stage = PREFLOP; gs->handPlaying = true;

    //reset gameover
    gs->gameOver = 0;
    gs->winnerID = MAX_PLAYERS+1;//aka invalid id (no winner yet)

    //set current player after blinds
    initBlinds(gs);
    resolveNoAct(gs, deck);
}

// allPlayersWent() moved to rules.c

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

int remainingPlayers(const GameState* gs)
{
    int count = 0; //players w/ chips

    for(int i = 0; i < MAX_PLAYERS; i++)
    {
        const Player* p = &gs->players[i];

        //skip inactive players
        if(p->status == PLAYER_DISCONNECTED || p->status == PLAYER_EMPTY || p->status == PLAYER_SPECTATING) continue;

        if(p->chips > 0)
            count ++;
    }

    return count;
}

int countStatus(const GameState* gs, PlayerStatus status)
{
    int count = 0;

    for(int i = 0; i < MAX_PLAYERS; i++)
    {
        const Player* p = &gs->players[i];
        if(p->status == status)
            count++;
    }
    
    return count;
}
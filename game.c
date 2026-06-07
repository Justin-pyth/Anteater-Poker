#include "game.h"
#include "specialCards.h"

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
    //to make it random, generate a seed
    srand(time(NULL));

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
            p->total_bet += call;
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
            p->total_bet += chipsPotted;
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
            p->total_bet += allIn;
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
            break;

    }

    gs->acted[playerID] = true;

}

// nextActive() and findActive() moved to rules.c

void awardSidepots(GameState* gs, int activeIDs[], int activeCount)
{
    //for tracking multiple winners, store chips prior
    uint32_t prevChips[MAX_PLAYERS];
    for(int i = 0; i< MAX_PLAYERS; i++)
        prevChips[i] = gs->players[i].chips;


    //side pot logic
    uint32_t threshold[MAX_PLAYERS]; //create an array of contribution thresholds
    int thresholdCount = 0;

    //find (unique) contribution thresholds first
    for(int i = 0; i < MAX_PLAYERS; i++)
    {
        uint32_t bet = gs->players[i].total_bet;
        if(bet == 0) continue; //skip if total bet is 0

        //track if the contribution threshold was already within the array
        bool dupe = false;
        for(int j = 0; j < thresholdCount; j++)
        {
            if(threshold[j] == bet)
            {
                dupe = true;
                break;
            }
        }

        //only add new thresholds
        if(!dupe)
            threshold[thresholdCount++] = bet;
    }

    //bubble sort
    for(int i = 0; i < thresholdCount-1; i++)
    {
        for(int j = 0; j < thresholdCount-i-1; j++)
        {
            if(threshold[j] > threshold[j+1])
            {
                //swap
                uint32_t tempBet = threshold[j];
                threshold[j] = threshold[j+1];
                threshold[j+1] = tempBet;
            }
        }
    }


    uint32_t previousThreshold = 0;
    for(int i = 0; i < thresholdCount; i ++)
    {
        //store non folded players here
        int eligible[MAX_PLAYERS];
        int eligCount = 0;

        //A put 50, B put 100, C put 200
        //slices would be 50, 50, 100
        //Any player putting more than a certain thresholdBet is (difference between previous and current) gets that slice
        uint32_t thresholdBet = threshold[i];
        uint32_t slice = thresholdBet - previousThreshold;
        uint32_t sidePot = 0;

        //for every player that has enough for the threshold, add their slice to the sidepot
        for(int p = 0; p < MAX_PLAYERS; p++)
        {
            if(gs->players[p].total_bet >= thresholdBet)
                sidePot += slice;
        }

        //check their eligibility(not folded), and add them to the list
        //eligible players are not folded and bet past the threshold
        for(int a = 0 ; a < activeCount; a++)
        {
            int p = activeIDs[a];

            if(gs->players[p].total_bet >= thresholdBet)
                eligible[eligCount++] = p;
        }

        //award the pot to eligible players
        awardPot(gs, sidePot, eligible, eligCount);
        previousThreshold = thresholdBet; //set curr threshold as previous for next iter
    }

    //track if multiple people won some money
    gs->winnerCount = 0; //reset winner count
    gs->winnerID = MAX_PLAYERS+1; //init invalid player 
    uint32_t highestChips = 0;
    for(int i = 0; i < MAX_PLAYERS; i++)
    {
        uint32_t addedChips = gs->players[i].chips - prevChips[i];
        if(addedChips > highestChips)
        {
            highestChips = addedChips;
            gs->winnerID = i;

        }
        if(addedChips > 0)
            gs->winnerCount++;
    }
    
    printf("[awardSidepots] winnerID=%d winnerCount=%d\n", gs->winnerID, gs->winnerCount);

}

void awardPot(GameState* gs, uint32_t pot, int eligible[], int eligCount)
{
    if (pot == 0 || eligCount == 0) return; //if the side pot is empty or no one is eligble, skip
    //for determining the winner
    int bestScore = -1, score, winnerCount = 0;
    int winnerIDs[MAX_PLAYERS];

    //iterate through all eligible players
    for(int i = 0 ; i < eligCount; i++)
    {
        int pID = eligible[i];
        const Player* p = &gs->players[pID];

        //evaluate their hand
        score = evaluateHand(gs, p->hand);

        //if their score is greater than bestScore, reset winner array and make new bestScore
        if(score > bestScore)
        {
            bestScore = score;
            winnerCount = 0;
            winnerIDs[winnerCount++] = pID;
        } //if their score is the same as bestScore, add them to winner array
        else if(score == bestScore)
        {
            winnerIDs[winnerCount++] = pID;
        }
            
    }

    //for multiple winners, split the pot by the winner count
    //2 winners means each player gets half
    uint32_t split = pot / winnerCount;
    uint32_t remainder = pot % winnerCount;

    for(int i = 0 ; i < winnerCount ; i++)
        gs->players[winnerIDs[i]].chips += split;

    gs->players[winnerIDs[0]].chips += remainder; //may change, just give to first winner for now

    

}

void award(GameState* gs)
{
    int activeIDs[MAX_PLAYERS];
    int count = findActive(gs, activeIDs, true);
    if(count == 0) return;

    //will handle sidepots if there is an all-in scenerio, otherwise, it'll just reward normally
    awardSidepots(gs, activeIDs, count);

    gs->pot = 0; //reset pot
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
        //aka check if they have chips left
        if(p->chips == 0)
        {
            if(p->place == 0) // only assign place if not already placed
            {
                p->place = remainingPlayers(gs) + 1;
                printf("[bust] %s busted, place=%d\n", p->name, p->place);
            }
            p->status = PLAYER_SPECTATING;
        }
        else
        {
            //only reset hole cards if they didn't bust, otherwise, use them for leaderboard
            memset(&p->hand, 0, sizeof(p->hand));
            p->status = PLAYER_READY;
        }
        //reset bets and cards
        p->current_bet = 0; p->total_bet = 0; p->has_cards = 0; 

        gs->acted[i] = false;

    }

    //reset gamestate values

    memset(gs->community, 0, sizeof(gs->community));
    gs->communityCount = 0;

    int newDealer = nextActive(gs, gs->dealerIndex, true); //new dealer
    if(newDealer != -1)
        gs->dealerIndex = newDealer;

    gs->pot = 0;
    gs->currentBet = 0;
    gs->minRaise = 0;
    gs->smallBlindIndex = 0;
    gs->bigBlindIndex = 0;

    gs->handPlaying = false;
    gs->phase = HAND_IDLE;

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
        p->place = 0;
    }
}

// initBlinds() moved to rules.c

/* ============================================================
    HAND FSM  (phase transitions)

    A hand moves through: BETTING -> (RUNOUT) -> COMPLETE.
    processMove() is the single transition taken after each move;
    runoutStep() is ticked by the server to pace the all-in reveal.
   ============================================================ */

// players still contesting the pot (not folded): PLAYING + ALL_IN
static int contesting(const GameState* gs)
{
    int ids[MAX_PLAYERS];
    return findActive(gs, ids, true);
}

// players who can still make a betting decision: PLAYING only (all-in can't act)
static int ableToBet(const GameState* gs)
{
    int ids[MAX_PLAYERS];
    return findActive(gs, ids, false);
}

// deal the next community street and open a fresh betting round.
// never deals past the river; does NOT pick the next actor (caller decides).
static void dealStreet(GameState* gs, Deck* deck)
{
    gs->currentBet = 0;
    gs->minRaise = gs->bigBlind ? gs->bigBlind : BIG_BLIND;

    for(int i = 0; i < MAX_PLAYERS; i++)
    {
        gs->acted[i] = false;
        gs->players[i].current_bet = 0;
    }

    //deal up to the street's target count, preserving any cards already revealed
    //early (e.g. by the REVEAL powerup) so they aren't overwritten
    switch(gs->stage)
    {
        case PREFLOP:
            while (gs->communityCount < 3) gs->community[gs->communityCount++] = deal(deck);
            gs->stage = FLOP;
            break;
        case FLOP:
            while (gs->communityCount < 4) gs->community[gs->communityCount++] = deal(deck);
            gs->stage = TURN;
            break;
        case TURN:
            while (gs->communityCount < 5) gs->community[gs->communityCount++] = deal(deck);
            gs->stage = RIVER;
            break;
        case RIVER:
            break; //no street past the river; showdown is handled separately
    }
}

// single hand-teardown path: pot cleared, hand parked in COMPLETE for the server.
static void endHand(GameState* gs)
{
    gs->pot = 0;
    gs->handPlaying = false;
    gs->currentPlayer = MAX_PLAYERS;
    gs->phase = HAND_COMPLETE;
}

// everyone but one player folded -> that player wins the pot uncontested.
static void endHandFold(GameState* gs)
{
    int ids[MAX_PLAYERS];
    findActive(gs, ids, true); //exactly one contestant remains
    uint8_t winner = ids[0];

    gs->players[winner].chips += gs->pot;
    gs->winnerID = winner;
    gs->winnerCount = 1; //single uncontested winner (everyone else folded)
    endHand(gs);
}

// final street reached -> evaluate hands and pay the winner(s).
static void endHandShowdown(GameState* gs)
{
    award(gs); //sets winnerID, pays chips, zeroes pot
    endHand(gs);
}

// enter the paced all-in runout; the server ticks runoutStep() from here.
static void beginRunout(GameState* gs)
{
    gs->phase = HAND_RUNOUT;
    gs->currentPlayer = MAX_PLAYERS; //no one acts during a runout
}

// betting round is settled: go to showdown, deal the next street, or run it out.
static void settleBettingRound(GameState* gs, Deck* deck)
{
    if(gs->stage == RIVER)        //no streets left -> showdown
    {
        endHandShowdown(gs);
        return;
    }
    if(ableToBet(gs) > 1)         //2+ players can still bet -> next street, keep betting
    {
        dealStreet(gs, deck);
        gs->currentPlayer = nextActive(gs, gs->dealerIndex, false);
        return;
    }
    beginRunout(gs);              //<=1 can act but streets remain -> paced runout
}

void newHand(GameState* gs, Deck* deck)
{
    shuffle(deck);
    dealHoles(gs, deck);

    //set to Preflop stage
    gs->stage = PREFLOP; gs->handPlaying = true;
    gs->phase = HAND_BETTING;

    //reset gameover
    gs->gameOver = 0;
    gs->winnerID = MAX_PLAYERS+1;//aka invalid id (no winner yet)

    //set last actor
    gs->lastActor = MAX_PLAYERS;

    //post blinds and set the first player to act
    initBlinds(gs);

    //blinds alone can already decide the hand (only one contestant, or everyone
    //forced all-in by the blinds) -> route through the same settle path as a move
    if(contesting(gs) == 1)
        endHandFold(gs);
    else if(allPlayersWent(gs))
        settleBettingRound(gs, deck);

    init_shop_slots(gs);
}

// allPlayersWent() moved to rules.c

// FSM transition taken after a move has been applied (or a current player left).
void processMove(GameState* gs, Deck* deck, uint8_t playerID)
{
    gs->lastActor = playerID;

    //everyone else folded -> uncontested pot
    if(contesting(gs) == 1)
    {
        endHandFold(gs);
        return;
    }

    //betting round not settled yet -> hand off to the next player who can act.
    //(this is what gives an opponent the chance to call/fold an all-in instead
    // of being run out on automatically.)
    if(!allPlayersWent(gs))
    {
        gs->currentPlayer = nextActive(gs, playerID, false);
        return;
    }

    //betting round settled -> showdown / next street / runout
    settleBettingRound(gs, deck);
}

// Ticked by the server while phase == HAND_RUNOUT to pace the all-in reveal:
// reveals one more street per call, then runs the showdown. Returns true while
// more reveals remain, false once the hand is decided.
bool runoutStep(GameState* gs, Deck* deck)
{
    if(gs->phase != HAND_RUNOUT)
        return false;

    if(gs->stage == RIVER)   //all community cards out -> showdown, runout done
    {
        endHandShowdown(gs);
        return false;
    }

    dealStreet(gs, deck);    //reveal one more street
    return true;
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

        //skip inactive seats
        if(p->status == PLAYER_DISCONNECTED || p->status == PLAYER_EMPTY) continue;

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

void updatePlaces(GameState* gs)
{
    //iterate through all players
    for(int i = 0; i < MAX_PLAYERS; i++)
    {
        Player* p = &gs->players[i];

        //skip empty or disconnected
        if(p->status == PLAYER_EMPTY || p->status == PLAYER_DISCONNECTED) continue;

        //if they went bust and (they are not spectating or the place hasnt been assigned)
        //have to check if they are not spectating for second place winner
        if(p->chips == 0 && (p->status != PLAYER_SPECTATING || p->place == 0))
        {
            p->place = remainingPlayers(gs) + 1;
            printf("[bust] %s busted, place=%d\n", p->name, p->place);
        }
    }
}

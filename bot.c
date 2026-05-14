#include "bot.h"


double monteCarloSim(const GameState* gs, const Deck* deck, const PlayerHand* botHand, int simCount)
{
    int winCount;

    GameState state_copy = *gs;
    Deck deck_copy = *deck;
    
    int activeIDs[MAX_PLAYERS];
    int count = findActive(&state_copy, activeIDs, true);

    reconstructDeck(&state_copy, botHand, &deck_copy);


    for(int i = 0; i < simCount ; i++)
    {
        shuffleRemaining(&deck_copy);

        //fill community
        for(int c = state_copy.communityCount-1; c < 5; c++)
        {
            state_copy.community[state_copy.communityCount++] = deal(&deck_copy);
        }


        for(int p = 0; p < MAX_PLAYERS; p++)
        {
            if((state_copy.players[p].id == activeIDs[p]) && (state_copy.players[p].id != state_copy.currentPlayer))
            {}
                dealHoles(&state_copy, &deck_copy); //deal 2 hole cards each
        }

        //find the best hand and see if its the bot's hand or not(if winner == botID)
        int bestScore = -1, score = 0, winner = 0;
        for(int i = 0 ; i < count; i++)
        {
            int pID = activeIDs[i];
            const Player p = state_copy.players[pID];

            score = evaluateHand(&state_copy, &p.hand);

            if(score > bestScore)
            {
                bestScore = score;
                winner = pID;
            }
            
        }

        //if the bot won, then increment win count
        if(winner == state_copy.currentPlayer)
            winCount++;

    }

    //the probablity of winning is the winCount over the # of sims
    //if you won 565 times in 1000 sims, then prob to win is 56.5%
    return winCount/simCount;
}

void botMove(const GameState* gs, const Deck* deck, uint8_t* playerID, MoveType* move, uint32_t* amount)
{
    //return id, movetype, raise amnt
    uint8_t botID = gs->currentPlayer;
    MoveType move = decide(gs, deck);
    int amount = 0;
    if(move == RAISE)
    {
        //pick a random amount to raise
        amount = gs->minRaise + (rand() % gs->players[botID].chips);
    }
    
}

MoveType decide(const GameState* gs, const Deck* deck)
{
    double prob = monteCarloSim(gs, deck, &gs->players[gs->currentPlayer].hand, 1000);
    int botChips = &gs->players[gs->currentPlayer].chips;
    int call = &gs->currentBet - botChips;
    if(call > 0)
    {
        if(prob > 0.75)     return RAISE;
        else if(prob > 0.5) return CALL;
        else                return FOLD;
    }
    else
    {
        if(prob > 0.65)     return RAISE;
        else                return CHECK;
    }
}

int checkRemainingCards(const Deck* deck)
{
    return deck->top;
}

void reconstructDeck(const GameState* gs, const PlayerHand* hand, Deck* deck)
{
    Card known[52];
    int kCount;
    Deck unknown;

    int comCount = gs->communityCount;

    //create an array of known cards
    for(int c = 0 ; c < comCount; c++)
    {
        known[c] = gs->community[c];
        kCount++;
    }

    known[comCount] = hand->hand[0]; kCount++;
    known[comCount] = hand->hand[1]; kCount++;


    //reconstruct the deck with known cards excluded
    int i = 0;
    for (int suit = HEARTS; suit <= SPADES; suit++) 
    {
        for (int rank = TWO; rank <= ACE; rank++) 
        {
            int isKnown = false;

            for(int k = 0; k<kCount; k++)
            {
                if((suit == known[k].suit) && (rank==known[k].rank))
                {
                    isKnown = true;
                    break;
                }
            }

            if(!isKnown)
            {
                unknown.cards[i].suit = suit;
                unknown.cards[i].rank = rank;
                unknown.cards[i].value = rank;
                i++;
            }
        }

    }

    unknown.top = i;
}
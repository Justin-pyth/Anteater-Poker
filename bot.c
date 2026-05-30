#include "bot.h"

void addBot(GameState* gs, bool shuffle)
{

    char botNames[MAX_PLAYERS][MAX_NAME_LENTH] = {"Alvin", "Randy", "Betty",
                                        "Colleen", "Minnie", "Dumbo"};

    //shuffle bot array
    if(shuffle)
    {
        for(int i = MAX_PLAYERS-1; i>0; i--)
        {
            int j = rand() % (i+1);

            //do random swaps to shuffle name bot name array
            char temp[MAX_NAME_LENTH];
            strcpy(temp, botNames[i]);
            strcpy(botNames[i], botNames[j]);
            strcpy(botNames[j], temp);
        }
    }

    //check for empty seats, if empty occupy with a random bot
    for(int seat = 0; seat < MAX_PLAYERS; seat++)
    {
        if(gs->players[seat].status == PLAYER_EMPTY)
        {
            for(int botNameIndex = 0; botNameIndex < MAX_PLAYERS; botNameIndex++)
            {
                bool nameInUse = false;
                for(int existingSeat = 0; existingSeat < MAX_PLAYERS; existingSeat++)
                {
                    if(existingSeat == seat) continue;
                    if(strcmp(gs->players[existingSeat].name, botNames[botNameIndex]) == 0)
                    {
                        nameInUse = true;
                        break;
                    }
                }

                if(!nameInUse)
                {
                    initPlayer(&gs->players[seat], seat, botNames[botNameIndex], INIT_CHIPS);
                    break;
                }
            }
        }
    }
}

bool isBot(const char* name)
{
    const char botNames[MAX_PLAYERS][MAX_NAME_LENTH] = {"Alvin", "Randy", "Betty",
                                            "Colleen", "Minnie", "Dumbo"};
    //to identify bots, just check their name, bot names will be reserved ideally
    for(int i = 0; i < MAX_PLAYERS; i++)
        if(strcmp(name, botNames[i]) == 0) return true; //strcmp returns 0 if equal
    
    return false;
}

double monteCarloSim(const GameState* gs, const Deck* deck, const Card* botHand, int simCount)
{
    double winCount = 0.0;

    GameState state_copy_og = *gs;
    Deck deck_copy_og = *deck;
    
    int activeIDs[MAX_PLAYERS];
    int count = findActive(&state_copy_og, activeIDs, true);

    int unknownCount = reconstructDeck(&state_copy_og, botHand, &deck_copy_og);


    for(int i = 0; i < simCount ; i++)
    {
        GameState state_copy = state_copy_og;
        Deck deck_copy = deck_copy_og;
        shuffleRemaining(&deck_copy, unknownCount);

        //fill community
        for(int c = state_copy.communityCount; c < 5; c++)
        {
            state_copy.community[state_copy.communityCount++] = deal(&deck_copy);
        }


        for(int p = 0; p < count; p++)
        {
            int pID = activeIDs[p];
            if(pID == state_copy.currentPlayer) continue; //skip if bot

            state_copy.players[pID].hand[0] = deal(&deck_copy);
            state_copy.players[pID].hand[1] = deal(&deck_copy);
            state_copy.players[pID].has_cards = 1;
        }

        //find the best hand and see if the bot is one of the winners
        int bestScore = -1, score, winnerCount = 0;
        int winnerIDs[MAX_PLAYERS];
        for(int i = 0 ; i < count; i++)
        {
            int pID = activeIDs[i];
            const Player p = state_copy.players[pID];

            score = evaluateHand(&state_copy, p.hand);

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

        //if bot tied, split its win count to a fractional number
        for(int i = 0; i < winnerCount; i++)
        {
            if(winnerIDs[i] == state_copy.currentPlayer)
            {
                winCount += 1.0 / winnerCount;
                break;
            }
        }

    }

    //the probablity of winning is the winCount over the # of sims
    //if you won 565 times in 1000 sims, then prob to win is 56.5%
    return winCount/simCount;
}

void botMove(const GameState* gs, const Deck* deck, uint8_t* botID, MoveType* move, uint32_t* amount)
{
    //return id, movetype, raise amnt
    *botID = gs->currentPlayer;
    *move = decide(gs, deck);
    *amount = 0;
    if(*move == RAISE)
    {
        //pick a random amount to raise
        uint32_t maxRaise = gs->players[*botID].chips - (gs->currentBet - gs->players[*botID].current_bet);

        if(maxRaise >= gs->minRaise)
            *amount = gs->minRaise + (rand() % (maxRaise - gs->minRaise + 1));
        else
        {
            //if bot can't raise but owes chips CALL
            if(gs->players[*botID].current_bet < gs->currentBet)
                *move = CALL;
            //if bot can't raise but owes nothing CHECK
            else
                *move = CHECK;
        }
    }
    
}

MoveType decide(const GameState* gs, const Deck* deck)
{
    double prob = monteCarloSim(gs, deck, gs->players[gs->currentPlayer].hand, 1000);
    uint32_t call = gs->currentBet - gs->players[gs->currentPlayer].current_bet;
    uint32_t chips = gs->players[gs->currentPlayer].chips;

    char name[MAX_NAME_LENTH];
    strcpy(name, gs->players[gs->currentPlayer].name);
    double _allIn, _raise, _call, _check, _bluff;

    if(strcmp(name, "Alvin") == 0)
    {
        _allIn = 0.30; _raise = 0.45; _call = 0.30; _check = 0.40, _bluff = 0.15;
    }
    else if(strcmp(name, "Randy") == 0)
    {
        _allIn = 0.50; _raise = 0.50; _call = 0.50; _check = 0.50, _bluff = 0.30;
    }
    else if(strcmp(name, "Betty") == 0)
    {
        _allIn = 0.50; _raise = 0.75; _call = 0.50; _check = 0.65, _bluff = 0.05;
    }
    else if(strcmp(name, "Colleen") == 0)
    {
        _allIn = 0.70; _raise = 0.85; _call = 0.70; _check = 0.80, _bluff = 0.00;
    }
    else if(strcmp(name, "Minnie") == 0)
    {
        _allIn = 0.25; _raise = 0.35; _call = 0.25; _check = 0.30, _bluff = 0.40;
    }
    else if(strcmp(name, "Dumbo") == 0)
    {
        _allIn = 0.10; _raise = 0.20; _call = 0.10; _check = 0.15, _bluff = 0.50;
    }
    else{ //default
        _allIn = 0.50; _raise = 0.75; _call = 0.50; _check = 0.65, _bluff = 0.05;
    }

    //if the bot is able to raise, bluff (based on bot name)
    if(call == 0 || chips >= call)
        if( (rand() / (double)RAND_MAX) < _bluff) return RAISE;
    
    //if bot owes chips
    if(call > 0)
    {
        //if bot doesn't have enough chips
        if(chips < call)
        {
            //you are allowed to go all_in on a call with insufficient chips
            if (prob > _allIn) return ALL_IN;
            else            return FOLD; //fold if bad hand
        }

        if(prob > _raise)     return RAISE;
        else if(prob > _call) return CALL;
        else                return FOLD;
    }
    else
    {
        if(prob > _check)     return RAISE;
        else                return CHECK;
    }
}

int reconstructDeck(const GameState* gs, const Card* hand, Deck* deck)
{
    Card known[DECK_SIZE];
    int kCount = 0;
    Deck unknown;

    int comCount = gs->communityCount;

    //create an array of known cards
    for(int c = 0 ; c < comCount; c++)
    {
        known[c] = gs->community[c];
        kCount++;
    }

    known[comCount] = hand[0]; kCount++;
    known[comCount+1] = hand[1]; kCount++;


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

    unknown.top = 0;

    *deck = unknown;

    return i;
}

bool doOneBotTurn(GameState* gs, Deck* deck)
{
    if(!gs->handPlaying || !isBot(gs->players[gs->currentPlayer].name))
        return false;

    uint8_t botID;
    MoveType move;
    uint32_t amount;

    //get the bot's move
    botMove(gs, deck, &botID, &move, &amount);

    //see if its a valid move and apply it
    return tryMove(gs, deck, botID, move, amount);
}

void doBotTurn(GameState* gs, Deck* deck)
{
    //while the betting round is occuring and the player is a bot
    while(gs->handPlaying && isBot(gs->players[gs->currentPlayer].name))
    {
        //try the move, if its invalid stop
        if(!doOneBotTurn(gs, deck))
            break;
    }
}

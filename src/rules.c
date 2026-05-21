#include "rules.h"

/* ============================================================
    HAND EVALUATION
   ============================================================ */

/*  Scores exactly 5 cards as a packed int: bits 23-20 = hand type (0-9),
    bits 19-0 = up to 5 tie-breaker rank nibbles in descending importance.
    Higher score always beats lower score; equal scores are ties.
*/
static int score5(Card five[5])
{
    // Extract ranks and suits into plain arrays
    int r[5], suits[5];
    for (int i = 0; i < 5; i++) { r[i] = five[i].rank; suits[i] = five[i].suit; }

    // Sort ranks descending so r[0] is highest — needed for straight/flush tie-breaks
    for (int i = 0; i < 4; i++)
        for (int j = i + 1; j < 5; j++)
            if (r[j] > r[i]) {
                int t = r[i]; r[i] = r[j]; r[j] = t; 
            }

    // Flush: all five cards share the same suit
    int isFlush = (suits[0] == suits[1] && suits[1] == suits[2] &&
                   suits[2] == suits[3] && suits[3] == suits[4]);

    // Straight: each rank is exactly one below the previous when sorted
    int isStraight = 0, straightHigh = r[0];
    if (r[0]-r[1]==1 && r[1]-r[2]==1 && r[2]-r[3]==1 && r[3]-r[4]==1)
        isStraight = 1;

    // Special case: Ace can play low in a 5-high straight (A-2-3-4-5).
    // (A-2-3-4-5): ace plays low, so sorted ranks look like [13,4,3,2,1].
    // The effective high card is the 5, not the ace.
    if (!isStraight && r[0]==ACE && r[1]==FIVE && r[2]==FOUR && r[3]==THREE && r[4]==TWO) {
        isStraight = 1; straightHigh = FIVE;
    }

    // Count how many times each rank appears (index = rank enum value, 1-13)
    int count[14] = {0};
    for (int i = 0; i < 5; i++) count[r[i]]++;

    //Finding Pairs, Trips, Quads, and Singles
    int quad = -1, triple = -1, pairs[2] = {-1, -1}, pairCount = 0;
    int singles[5], singleCount = 0;
    for (int rank = ACE; rank >= TWO; rank--) {
        if      (count[rank] == 4) quad = rank;
        else if (count[rank] == 3) triple = rank;
        else if (count[rank] == 2 && pairCount < 2) pairs[pairCount++] = rank;
        else if (count[rank] == 1) singles[singleCount++] = rank;
    }

    // Determine hand type using standard poker priority (highest first).
    // Each condition is checked in descending rank order so the first match wins.
    int type, s = 0;
    if      (isFlush && isStraight && straightHigh == ACE) type = ROYAL_FLUSH;    // 10-J-Q-K-A all same suit — the best possible hand
    else if (isFlush && isStraight)                        type = STRAIGHT_FLUSH; // five consecutive ranks all same suit
    else if (quad != -1)                                   type = FOUR_KIND;      // four cards of the same rank
    else if (triple != -1 && pairCount >= 1)               type = FULL_HOUSE;     // three of a kind plus a pair
    else if (isFlush)                                      type = FLUSH;          // five cards of the same suit (not sequential)
    else if (isStraight)                                   type = STRAIGHT;       // five consecutive ranks (mixed suits)
    else if (triple != -1)                                 type = THREE_KIND;     // three cards of the same rank
    else if (pairCount == 2)                               type = TWO_PAIR;       // two different pairs
    else if (pairCount == 1)                               type = PAIR;           // exactly one pair
    else                                                   type = HIGHT_CARD;    // no matching ranks or suits — highest card plays

    // Pack tie-breaker ranks into the low 20 bits (4 bits per rank, max rank = ACE = 13 = 0xD).
    // For each hand type, the most important rank goes in the highest nibble so that
    // a simple integer comparison resolves ties correctly.
    switch (type) {
        case ROYAL_FLUSH:    s = 0; break;                                                            // all royal flushes tie
        case STRAIGHT_FLUSH: s = (straightHigh << 16); break;                                        // highest card wins
        case FOUR_KIND:      s = (quad << 16) | (singles[0] << 12); break;                           // quad rank, then kicker
        case FULL_HOUSE:     s = (triple << 16) | (pairs[0] << 12); break;                           // three rank, then pair rank
        case FLUSH:          s = (r[0]<<16)|(r[1]<<12)|(r[2]<<8)|(r[3]<<4)|r[4]; break;             // all 5 ranks, high to low
        case STRAIGHT:       s = (straightHigh << 16); break;                                        // highest card wins
        case THREE_KIND:     s = (triple<<16)|(singles[0]<<12)|(singles[1]<<8); break;               // three rank, then 2 kickers
        case TWO_PAIR:       s = (pairs[0]<<16)|(pairs[1]<<12)|(singles[0]<<8); break;               // high pair, low pair, kicker
        case PAIR:           s = (pairs[0]<<16)|(singles[0]<<12)|(singles[1]<<8)|(singles[2]<<4); break; // pair rank, then 3 kickers
        case HIGHT_CARD:     s = (r[0]<<16)|(r[1]<<12)|(r[2]<<8)|(r[3]<<4)|r[4]; break;             // all 5 ranks, high to low
    }
    // Final score: hand type in bits 23-20, tie-breakers in bits 19-0
    return (type << 20) | s;
}

int evaluateHand(const GameState* gs, const Card* hand)
{
    // Gather all available cards: 2 hole cards + however many community cards
    Card pool[7];
    pool[0] = hand[0];
    pool[1] = hand[1];
    int total = 2;
    for (int i = 0; i < gs->communityCount; i++)
        pool[total++] = gs->community[i];

    if (total < 5) return 0; // not enough cards to form a hand yet

    //Try all 21 hand combinations
    int best = 0;
    for (int a = 0; a < total-4; a++)
    for (int b = a+1; b < total-3; b++)
    for (int c = b+1; c < total-2; c++)
    for (int d = c+1; d < total-1; d++)
    for (int e = d+1; e < total; e++) {
        Card five[5] = { pool[a], pool[b], pool[c], pool[d], pool[e] };
        int sc = score5(five);
        if (sc > best) best = sc;
    }
    return best;
}


/* ============================================================
    MOVE LEGALITY
   ============================================================ */

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


/* ============================================================
    TURN ORDER & ACTIVE PLAYERS
   ============================================================ */

int findActive(const GameState* gs, int activeIDs[], bool inclAllIn)
{
    int count = 0;
    for(int i = 0 ; i < MAX_PLAYERS ; i++)
        if(gs->players[i].status == PLAYER_PLAYING || (inclAllIn && gs->players[i].status == PLAYER_ALL_IN))
            activeIDs[count++] = i;

    return count;
}

int nextActive(const GameState* gs, int curr, bool inclReady)
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


/* ============================================================
    BETTING-ROUND RULES
   ============================================================ */

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


/* ============================================================
    SHOWDOWN
   ============================================================ */

int determineWinners(const GameState* gs, int winnerIDs[])
{
    int activeIDs[MAX_PLAYERS];
    int count = findActive(gs, activeIDs, true);
    if(count == 0) return 0;

    int bestScore = -1, score, winnerCount = 0;
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

    return winnerCount;
}

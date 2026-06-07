#include "rules.h"

// Helper function, scores 5 card hand
static int score5(Card five[5])
{
    // Extract ranks and suits into plain arrays
    int r[5], suits[5];
    for (int i = 0; i < 5; i++)
    {
        r[i] = five[i].rank;
        suits[i] = five[i].suit;
    }

    // Sort ranks descending so r[0] is highest, needed for straight/flush tie-breaks
    for (int i = 0; i < 4; i++)
    {
        for (int j = i + 1; j < 5; j++)
        {
            if (r[j] > r[i])
            {
                int t = r[i];
                r[i] = r[j];
                r[j] = t;
            }
        }
    }

    // Flush: all five cards share the same suit
    int isFlush = (suits[0] == suits[1] &&
                   suits[1] == suits[2] &&
                   suits[2] == suits[3] &&
                   suits[3] == suits[4]);

    // Straight: each rank is exactly one below the previous when sorted
    int isStraight = 0, straightHigh = r[0];
    if (r[0] - r[1] == 1 && r[1] - r[2] == 1 && r[2] - r[3] == 1 && r[3] - r[4] == 1)
    {
        isStraight = 1;
    }

    // Special case: Ace can play low in a 5-high straight (A-2-3-4-5).
    // (A-2-3-4-5): ace plays low
    // The effective high card is the 5, not the ace.
    if (!isStraight && r[0] == ACE && r[1] == FIVE && r[2] == FOUR && r[3] == THREE && r[4] == TWO)
    {
        isStraight = 1;
        straightHigh = FIVE;
    }

    // Count how many times each rank appears (index = rank enum value, 1-13)
    int count[14] = {0};
    for (int i = 0; i < 5; i++)
    {
        count[r[i]]++;
    }

    // Finding Pairs, Trips, Quads, and Singles
    int quad = -1;
    int triple = -1;
    int pairs[2] = {-1, -1};
    int pairCount = 0;
    int singles[5], singleCount = 0;

    for (int rank = ACE; rank >= TWO; rank--)
    {
        if (count[rank] == 4)
        {
            quad = rank;
        }
        else if (count[rank] == 3)
        {
            triple = rank;
        }
        else if (count[rank] == 2 && pairCount < 2)
        {
            pairs[pairCount++] = rank;
        }
        else if (count[rank] == 1)
        {
            singles[singleCount++] = rank;
        }
    }

    // Determine hand type using standard poker priority (highest first).
    // Each condition is checked in descending rank order so the first match wins.
    int type, s = 0;
    if (isFlush && isStraight && straightHigh == ACE)
        type = ROYAL_FLUSH; // 10-J-Q-K-A all same suit — the best possible hand
    else if (isFlush && isStraight)
        type = STRAIGHT_FLUSH; // five consecutive ranks all same suit
    else if (quad != -1)
        type = FOUR_KIND; // four cards of the same rank
    else if (triple != -1 && pairCount >= 1)
        type = FULL_HOUSE; // three of a kind plus a pair
    else if (isFlush)
        type = FLUSH; // five cards of the same suit (not sequential)
    else if (isStraight)
        type = STRAIGHT; // five consecutive ranks (mixed suits)
    else if (triple != -1)
        type = THREE_KIND; // three cards of the same rank
    else if (pairCount == 2)
        type = TWO_PAIR; // two different pairs
    else if (pairCount == 1)
        type = PAIR; // exactly one pair
    else
        type = HIGHT_CARD; // no matching ranks or suits — highest card plays

    // Pack tie-breaker ranks into the low 20 bits (4 bits per rank, max rank = ACE = 13 = 0xD).
    // For each hand type, the most important rank goes in the highest nibble so that
    // a simple integer comparison resolves ties correctly.
    switch (type)
    {
    case ROYAL_FLUSH:
        s = 0;
        break; // all royal flushes tie
    case STRAIGHT_FLUSH:
        s = (straightHigh << 16);
        break; // highest card wins
    case FOUR_KIND:
        s = (quad << 16) | (singles[0] << 12);
        break; // quad rank, then kicker
    case FULL_HOUSE:
        s = (triple << 16) | (pairs[0] << 12);
        break; // three rank, then pair rank
    case FLUSH:
        s = (r[0] << 16) | (r[1] << 12) | (r[2] << 8) | (r[3] << 4) | r[4];
        break; // all 5 ranks, high to low
    case STRAIGHT:
        s = (straightHigh << 16);
        break; // highest card wins
    case THREE_KIND:
        s = (triple << 16) | (singles[0] << 12) | (singles[1] << 8);
        break; // three rank, then 2 kickers
    case TWO_PAIR:
        s = (pairs[0] << 16) | (pairs[1] << 12) | (singles[0] << 8);
        break; // high pair, low pair, kicker
    case PAIR:
        s = (pairs[0] << 16) | (singles[0] << 12) | (singles[1] << 8) | (singles[2] << 4);
        break; // pair rank, then 3 kickers
    case HIGHT_CARD:
        s = (r[0] << 16) | (r[1] << 12) | (r[2] << 8) | (r[3] << 4) | r[4];
        break; // all 5 ranks, high to low
    }
    // Final score: hand type in bits 23-20, tie-breakers in bits 19-0
    return (type << 20) | s;
}

int evaluateHand(const GameState *gs, const Card *hand)
{
    // Gather all available cards: 2 hole cards + however many community cards
    Card pool[7];
    pool[0] = hand[0];
    pool[1] = hand[1];
    int total = 2;
    for (int i = 0; i < gs->communityCount; i++)
    {
        pool[total++] = gs->community[i];
    }

    // not enough cards to form a hand yet
    if (total < 5)
    {
        return 0;
    }

    // Try all 21 hand combinations
    int best = 0;
    for (int a = 0; a < total - 4; a++)
    {
        for (int b = a + 1; b < total - 3; b++)
        {
            for (int c = b + 1; c < total - 2; c++)
            {
                for (int d = c + 1; d < total - 1; d++)
                {
                    for (int e = d + 1; e < total; e++)
                    {
                        Card five[5] = {pool[a], pool[b], pool[c], pool[d], pool[e]};
                        int sc = score5(five);
                        if (sc > best)
                        {
                            best = sc;
                        }
                    }
                }
            }
        }
    }
    return best;
}

#ifdef DEBUG
// UNIT TESTS for score5() and evaluateHand()

#include <stdio.h>
#include <string.h>

void printResult(const char *name, int cond)
{
    if (cond)
    {
        printf("  PASS  %s\n", name);
    }
    else
    {
        printf("  FAIL  %s  (line %d)\n", name, __LINE__);
    }
}

static Card buildCard(Suit s, Rank r)
{
    Card c;
    c.suit = s;
    c.rank = r;
    c.value = r;
    return c;
}
static int handRank(int score)
{
    return (score >> 20) & 0xF; // hand type in bits 23-20
}

int main(void)
{
    printf("score5: hand type detection\n");

    Card royal[5] = {buildCard(SPADES, ACE), buildCard(SPADES, KING), buildCard(SPADES, QUEEN), buildCard(SPADES, JACK), buildCard(SPADES, TEN)};
    Card sflush[5] = {buildCard(HEARTS, NINE), buildCard(HEARTS, EIGHT), buildCard(HEARTS, SEVEN), buildCard(HEARTS, SIX), buildCard(HEARTS, FIVE)};
    Card quads[5] = {buildCard(SPADES, KING), buildCard(HEARTS, KING), buildCard(DIAMONDS, KING), buildCard(CLUBS, KING), buildCard(SPADES, TWO)};
    Card boat[5] = {buildCard(SPADES, TEN), buildCard(HEARTS, TEN), buildCard(DIAMONDS, TEN), buildCard(CLUBS, FOUR), buildCard(SPADES, FOUR)};
    Card flush[5] = {buildCard(CLUBS, ACE), buildCard(CLUBS, TEN), buildCard(CLUBS, SEVEN), buildCard(CLUBS, FIVE), buildCard(CLUBS, TWO)};
    Card str6[5] = {buildCard(SPADES, SIX), buildCard(HEARTS, FIVE), buildCard(DIAMONDS, FOUR), buildCard(CLUBS, THREE), buildCard(SPADES, TWO)};
    Card wheel[5] = {buildCard(SPADES, ACE), buildCard(HEARTS, TWO), buildCard(DIAMONDS, THREE), buildCard(CLUBS, FOUR), buildCard(SPADES, FIVE)};
    Card trips[5] = {buildCard(SPADES, SEVEN), buildCard(HEARTS, SEVEN), buildCard(DIAMONDS, SEVEN), buildCard(CLUBS, KING), buildCard(SPADES, TWO)};
    Card twopair[5] = {buildCard(SPADES, JACK), buildCard(HEARTS, JACK), buildCard(DIAMONDS, THREE), buildCard(CLUBS, THREE), buildCard(SPADES, TWO)};
    Card pair[5] = {buildCard(SPADES, NINE), buildCard(HEARTS, NINE), buildCard(DIAMONDS, KING), buildCard(CLUBS, SEVEN), buildCard(SPADES, TWO)};
    Card high1[5] = {buildCard(SPADES, ACE), buildCard(HEARTS, KING), buildCard(DIAMONDS, NINE), buildCard(CLUBS, SEVEN), buildCard(SPADES, TWO)};
    Card high2[5] = {buildCard(SPADES, ACE), buildCard(HEARTS, KING), buildCard(DIAMONDS, NINE), buildCard(CLUBS, SIX), buildCard(SPADES, TWO)};

    printResult("royal flush", handRank(score5(royal)) == ROYAL_FLUSH);
    printResult("straight flush", handRank(score5(sflush)) == STRAIGHT_FLUSH);
    printResult("four of a kind", handRank(score5(quads)) == FOUR_KIND);
    printResult("full house", handRank(score5(boat)) == FULL_HOUSE);
    printResult("flush", handRank(score5(flush)) == FLUSH);
    printResult("straight", handRank(score5(str6)) == STRAIGHT);
    printResult("wheel A-5 straight", handRank(score5(wheel)) == STRAIGHT);
    printResult("three of a kind", handRank(score5(trips)) == THREE_KIND);
    printResult("two pair", handRank(score5(twopair)) == TWO_PAIR);
    printResult("one pair", handRank(score5(pair)) == PAIR);
    printResult("high card", handRank(score5(high1)) == HIGHT_CARD);

    printf("\nscore5: ordering & tie-breaks\n");
    printResult("full house > flush", score5(boat) > score5(flush));
    printResult("flush > straight", score5(flush) > score5(str6));
    printResult("quads > full house", score5(quads) > score5(boat));
    printResult("6-high straight > wheel", score5(str6) > score5(wheel)); 
    printResult("kicker breaks tie (7 > 6)", score5(high1) > score5(high2));

    printf("\nevaluateHand: best 5 of 7\n");
    GameState gs;
    memset(&gs, 0, sizeof gs);
    gs.communityCount = 5;
    gs.community[0] = buildCard(SPADES, ACE);
    gs.community[1] = buildCard(SPADES, KING);
    gs.community[2] = buildCard(SPADES, QUEEN);
    gs.community[3] = buildCard(HEARTS, TWO);
    gs.community[4] = buildCard(DIAMONDS, SEVEN);

    Card holeRoyal[2] = {buildCard(SPADES, JACK), buildCard(SPADES, TEN)}; /* completes the royal flush */
    printResult("finds royal flush in 7", handRank(evaluateHand(&gs, holeRoyal)) == ROYAL_FLUSH);

    Card holeTrips[2] = {buildCard(SPADES, TWO), buildCard(DIAMONDS, TWO)}; /* three 2s with the board 2h */
    printResult("finds trips in 7", handRank(evaluateHand(&gs, holeTrips)) == THREE_KIND);

    Card holePairA[2] = {buildCard(CLUBS, ACE), buildCard(HEARTS, FIVE)}; /* pair of aces (Ah board + Ac) */
    printResult("finds pair of aces", handRank(evaluateHand(&gs, holePairA)) == PAIR);

    GameState pre;
    memset(&pre, 0, sizeof pre);
    pre.communityCount = 0; /* preflop: < 5 cards */
    Card any[2] = {buildCard(SPADES, ACE), buildCard(HEARTS, ACE)};
    printResult("returns 0 with < 5 cards", evaluateHand(&pre, any) == 0);

    return 0;
}
#endif /* DEBUG */

/* ============================================================
    MOVE LEGALITY
   ============================================================ */

bool validate(const GameState *gs, uint8_t playerID, MoveType move, uint32_t amount)
{
    if (playerID >= MAX_PLAYERS)
        return false;
    if (playerID != gs->currentPlayer)
        return false;
    if (!gs->handPlaying)
        return false;

    const Player *p = &gs->players[playerID];
    if (p->status != PLAYER_PLAYING)
        return false;
    switch (move)
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
        if (p->current_bet >= gs->currentBet)
            return false;

        return p->chips >= gs->currentBet - p->current_bet;
    }
    case RAISE:
    {
        if (amount < gs->minRaise)
            return false;

        return p->chips >= (gs->currentBet + amount) - p->current_bet;
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

int findActive(const GameState *gs, int activeIDs[], bool inclAllIn)
{
    int count = 0;
    for (int i = 0; i < MAX_PLAYERS; i++)
        if (gs->players[i].status == PLAYER_PLAYING || (inclAllIn && gs->players[i].status == PLAYER_ALL_IN))
            activeIDs[count++] = i;

    return count;
}

int nextActive(const GameState *gs, int curr, bool inclReady)
{
    for (int step = 1; step <= MAX_PLAYERS; step++)
    {
        int next = (((curr + step) % MAX_PLAYERS) + MAX_PLAYERS) % MAX_PLAYERS;

        // if the player is active, return index
        if (gs->players[next].status == PLAYER_PLAYING || (inclReady && gs->players[next].status == PLAYER_READY))
            return next;
    }

    return -1; // return if no one found
}

/* ============================================================
    BETTING-ROUND RULES
   ============================================================ */

bool allPlayersWent(const GameState *gs)
{
    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        const Player *p = &gs->players[i];
        if (p->status != PLAYER_PLAYING)
            continue;

        if ((!gs->acted[i]) || (p->current_bet < gs->currentBet))
            return false;
    }
    return true;
}

void initBlinds(GameState *gs)
{
    int small = nextActive(gs, gs->dealerIndex, false);
    int big = nextActive(gs, small, false);

    if (small == -1 || big == -1)
        return; // no active players found

    if (gs->players[small].chips <= SMALL_BLIND)
    {
        // player goes all in if they can't afford small blind
        gs->pot += gs->players[small].chips;
        gs->players[small].current_bet = gs->players[small].chips;
        gs->players[small].total_bet += gs->players[small].chips;
        gs->players[small].chips = 0;
        gs->players[small].status = PLAYER_ALL_IN;
    }
    else
    {
        // player can afford small blind
        gs->players[small].chips -= SMALL_BLIND;
        gs->players[small].current_bet = SMALL_BLIND;
        gs->players[small].total_bet += SMALL_BLIND;
        gs->pot += SMALL_BLIND;
    }

    if (gs->players[big].chips <= BIG_BLIND)
    {
        // player goes all in if they can't afford big blind
        gs->pot += gs->players[big].chips;
        gs->players[big].current_bet = gs->players[big].chips;
        gs->players[big].total_bet += gs->players[big].chips;
        gs->players[big].chips = 0;
        gs->players[big].status = PLAYER_ALL_IN;
    }
    else
    {
        // player can afford big blind
        gs->players[big].chips -= BIG_BLIND;
        gs->players[big].current_bet = BIG_BLIND;
        gs->players[big].total_bet += BIG_BLIND;
        gs->pot += BIG_BLIND;
    }

    gs->currentBet = BIG_BLIND;
    gs->minRaise = BIG_BLIND;

    // add rest of texas holdem conditions later
    gs->currentPlayer = nextActive(gs, big, false);
    gs->smallBlindIndex = small;
    gs->bigBlindIndex = big;
    gs->smallBlind = SMALL_BLIND;
    gs->bigBlind = BIG_BLIND;
}

/* ============================================================
    SHOWDOWN
   ============================================================ */

int determineWinners(const GameState *gs, int winnerIDs[])
{
    int activeIDs[MAX_PLAYERS];
    int count = findActive(gs, activeIDs, true);
    if (count == 0)
        return 0;

    int bestScore = -1, score, winnerCount = 0;
    for (int i = 0; i < count; i++)
    {
        int pID = activeIDs[i];
        const Player *p = &gs->players[pID];

        score = evaluateHand(gs, p->hand);

        if (score > bestScore)
        {
            bestScore = score;
            winnerCount = 0;
            winnerIDs[winnerCount++] = pID;
        }
        else if (score == bestScore)
        {
            winnerIDs[winnerCount++] = pID;
        }
    }

    return winnerCount;
}

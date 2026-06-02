/*  tests/test_game.c
    Comprehensive unit tests for game logic: move validation, apply, hand evaluation,
    award, showdown, and full hand progression.
    Every move type (FOLD, CHECK, CALL, RAISE, ALL_IN) is tested with valid and invalid cases.
    Build: make tests/test_game
    Run:   ./tests/test_game
*/
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "game.h"
#include "rules.h"
#include "uds.h"

static int passed = 0, failed = 0;
#define TEST(name, expr) do { \
    if (expr) { printf("  PASS  %s\n", name); passed++; } \
    else       { printf("  FAIL  %s  (line %d)\n", name, __LINE__); failed++; } \
} while(0)

static Card make_card(Rank r, Suit s) { Card c; c.rank = r; c.suit = s; c.value = r; return c; }

static void setup_2player(GameState *gs, Deck *deck)
{
    memset(gs, 0, sizeof(*gs));
    initDeck(deck);
    initPlayer(&gs->players[0], 0, "Alice", INIT_CHIPS);
    initPlayer(&gs->players[1], 1, "Bob",   INIT_CHIPS);
    gs->players[0].status = PLAYER_READY;
    gs->players[1].status = PLAYER_READY;
    gs->playerCount = 2;
}

/* ================================================================
   1. FOLD — valid, wrong player, after-fold state, last-player showdown
   ================================================================ */
static void test_fold(void)
{
    printf("\n[FOLD]\n");
    GameState gs; Deck deck;
    setup_2player(&gs, &deck);
    newHand(&gs, &deck);

    uint8_t cur   = gs.currentPlayer;
    uint8_t other = (cur == 0) ? 1 : 0;

    /* valid fold */
    TEST("fold is valid for current player",    validate(&gs, cur,   FOLD, 0));
    /* wrong player cannot fold */
    TEST("fold is invalid for other player",   !validate(&gs, other, FOLD, 0));

    /* apply fold — player status changes */
    apply(&gs, cur, FOLD, 0);
    TEST("fold sets PLAYER_FOLDED",             gs.players[cur].status == PLAYER_FOLDED);

    /* folded player cannot act again */
    TEST("folded player fails validate",       !validate(&gs, cur, FOLD, 0));

    /* only 1 active player => processMove triggers showdown */
    setup_2player(&gs, &deck);
    newHand(&gs, &deck);
    cur   = gs.currentPlayer;
    other = (cur == 0) ? 1 : 0;
    gs.players[other].status = PLAYER_FOLDED;   /* force last player standing */
    processMove(&gs, &deck, cur);
    TEST("showdown set when last player folds", gs.phase == HAND_COMPLETE);
    TEST("handPlaying false after last fold",   gs.handPlaying == false);
    TEST("winnerID correct after fold-win",     gs.winnerID == cur);
}

/* ================================================================
   2. CHECK — valid when bet matched, invalid when behind
   ================================================================ */
static void test_check(void)
{
    printf("\n[CHECK]\n");
    GameState gs; Deck deck;
    setup_2player(&gs, &deck);
    newHand(&gs, &deck);

    uint8_t cur = gs.currentPlayer;
    Player *p   = &gs.players[cur];

    if (p->current_bet == gs.currentBet) {
        TEST("check valid when bet matched",   validate(&gs, cur, CHECK, 0));
        apply(&gs, cur, CHECK, 0);
        TEST("chips unchanged after check",    p->chips == INIT_CHIPS);
    } else {
        TEST("check invalid when behind on bet", !validate(&gs, cur, CHECK, 0));
        /* force the bet to match, then check should be valid */
        p->current_bet = gs.currentBet;
        TEST("check valid after matching bet",  validate(&gs, cur, CHECK, 0));
    }

    /* wrong player cannot check */
    uint8_t other = (cur == 0) ? 1 : 0;
    TEST("check invalid for other player",    !validate(&gs, other, CHECK, 0));
}

/* ================================================================
   3. CALL — chips deducted, pot grows, status stays PLAYING
   ================================================================ */
static void test_call(void)
{
    printf("\n[CALL]\n");
    GameState gs; Deck deck;
    setup_2player(&gs, &deck);
    newHand(&gs, &deck);

    uint8_t cur = gs.currentPlayer;
    Player *p   = &gs.players[cur];

    /* find a player who is behind on the bet */
    if (p->current_bet < gs.currentBet) {
        uint32_t to_call     = gs.currentBet - p->current_bet;
        uint32_t chips_before = p->chips;
        uint32_t pot_before   = gs.pot;

        TEST("call is valid",               validate(&gs, cur, CALL, 0));
        apply(&gs, cur, CALL, 0);
        TEST("call deducts exact chips",    p->chips == chips_before - to_call);
        TEST("call increases pot correctly",gs.pot   == pot_before   + to_call);
        TEST("status stays PLAYING",        p->status == PLAYER_PLAYING);
    } else {
        /* rotate to the player who posted the blind */
        uint8_t other = (cur == 0) ? 1 : 0;
        Player *q     = &gs.players[other];
        if (q->current_bet < gs.currentBet) {
            uint32_t to_call     = gs.currentBet - q->current_bet;
            uint32_t chips_before = q->chips;
            uint32_t pot_before   = gs.pot;
            gs.currentPlayer = other;
            TEST("call is valid for BB",        validate(&gs, other, CALL, 0));
            apply(&gs, other, CALL, 0);
            TEST("call deducts chips (BB)",     q->chips == chips_before - to_call);
            TEST("pot grows by call amount",    gs.pot   == pot_before   + to_call);
        } else {
            TEST("call skipped (already all matched)", 1);
        }
    }

    /* wrong player cannot call */
    setup_2player(&gs, &deck);
    newHand(&gs, &deck);
    cur = gs.currentPlayer;
    uint8_t other = (cur == 0) ? 1 : 0;
    TEST("call invalid for other player",   !validate(&gs, other, CALL, 0));
}

/* ================================================================
   4. RAISE — amount must meet minRaise, currentBet updated, chips deducted
   ================================================================ */
static void test_raise(void)
{
    printf("\n[RAISE]\n");
    GameState gs; Deck deck;
    setup_2player(&gs, &deck);
    newHand(&gs, &deck);

    uint8_t cur = gs.currentPlayer;
    Player *p   = &gs.players[cur];

    /* below minRaise rejected */
    TEST("raise below minRaise invalid",    !validate(&gs, cur, RAISE, gs.minRaise - 1));

    /* raise of 0 rejected */
    TEST("raise of 0 invalid",              !validate(&gs, cur, RAISE, 0));

    /* exact minRaise accepted */
    TEST("raise at minRaise valid",          validate(&gs, cur, RAISE, gs.minRaise));

    /* raise above minRaise accepted */
    TEST("raise above minRaise valid",       validate(&gs, cur, RAISE, gs.minRaise + 50));

    /* apply raise */
    uint32_t chips_before = p->chips;
    uint32_t raise_to     = gs.minRaise;
    uint32_t old_bet      = gs.currentBet;
    apply(&gs, cur, RAISE, raise_to);
    TEST("currentBet increases after raise", gs.currentBet > old_bet);
    TEST("chips decrease after raise",       p->chips < chips_before);

    /* after a raise, other player's current_bet is behind the new currentBet */
    uint8_t other = (cur == 0) ? 1 : 0;
    TEST("other player is behind currentBet after raise",
         gs.players[other].current_bet < gs.currentBet);

    /* wrong player cannot raise */
    setup_2player(&gs, &deck);
    newHand(&gs, &deck);
    cur   = gs.currentPlayer;
    other = (cur == 0) ? 1 : 0;
    TEST("raise invalid for other player",  !validate(&gs, other, RAISE, gs.minRaise));
}

/* ================================================================
   5. ALL_IN — zeroes chips, sets status, pot grows
   ================================================================ */
static void test_allin(void)
{
    printf("\n[ALL_IN]\n");
    GameState gs; Deck deck;
    setup_2player(&gs, &deck);
    newHand(&gs, &deck);

    uint8_t cur = gs.currentPlayer;
    Player *p   = &gs.players[cur];
    uint32_t pot_before = gs.pot;

    TEST("all-in valid with chips",          validate(&gs, cur, ALL_IN, 0));

    apply(&gs, cur, ALL_IN, 0);
    TEST("all-in zeroes chips",              p->chips  == 0);
    TEST("all-in sets PLAYER_ALL_IN",        p->status == PLAYER_ALL_IN);
    TEST("all-in increases pot",             gs.pot > pot_before);

    /* player with 0 chips cannot go all-in again */
    TEST("all-in invalid with 0 chips",     !validate(&gs, cur, ALL_IN, 0));

    /* wrong player cannot go all-in */
    setup_2player(&gs, &deck);
    newHand(&gs, &deck);
    cur = gs.currentPlayer;
    uint8_t other = (cur == 0) ? 1 : 0;
    TEST("all-in invalid for other player", !validate(&gs, other, ALL_IN, 0));
}

/* ================================================================
   6. HAND EVALUATION
   ================================================================ */
static void test_evaluate(void)
{
    printf("\n[evaluateHand]\n");
    GameState gs;
    memset(&gs, 0, sizeof(gs));

    /* Royal flush vs straight flush */
    gs.community[0] = make_card(TEN,   HEARTS);
    gs.community[1] = make_card(JACK,  HEARTS);
    gs.community[2] = make_card(QUEEN, HEARTS);
    gs.community[3] = make_card(KING,  HEARTS);
    gs.community[4] = make_card(NINE,  HEARTS);
    gs.communityCount = 5;
    Card royal[2] = { make_card(ACE,   HEARTS), make_card(TWO,   CLUBS) };
    Card sf[2]    = { make_card(EIGHT, HEARTS), make_card(TWO,   CLUBS) };
    TEST("royal flush > straight flush",     evaluateHand(&gs, royal) > evaluateHand(&gs, sf));

    /* Four of a kind vs full house */
    gs.community[0] = make_card(ACE,  HEARTS);
    gs.community[1] = make_card(ACE,  DIAMONDS);
    gs.community[2] = make_card(ACE,  CLUBS);
    gs.community[3] = make_card(KING, HEARTS);
    gs.community[4] = make_card(KING, DIAMONDS);
    gs.communityCount = 5;
    Card quads[2]    = { make_card(ACE,  SPADES), make_card(TWO, CLUBS) };
    Card fullhouse[2]= { make_card(KING, CLUBS),  make_card(TWO, CLUBS) };
    TEST("quads > full house",               evaluateHand(&gs, quads) > evaluateHand(&gs, fullhouse));

    /* Flush vs straight */
    gs.community[0] = make_card(TWO,   HEARTS);
    gs.community[1] = make_card(FIVE,  HEARTS);
    gs.community[2] = make_card(SEVEN, HEARTS);
    gs.community[3] = make_card(NINE,  HEARTS);
    gs.community[4] = make_card(SIX,   CLUBS);
    gs.communityCount = 5;
    Card flush[2]    = { make_card(THREE, HEARTS), make_card(JACK, HEARTS) };
    Card straight[2] = { make_card(THREE, CLUBS),  make_card(FOUR, DIAMONDS) };
    TEST("flush > straight",                 evaluateHand(&gs, flush) > evaluateHand(&gs, straight));

    /* Pair vs high card */
    gs.community[0] = make_card(TWO,  CLUBS);
    gs.community[1] = make_card(FIVE, DIAMONDS);
    gs.community[2] = make_card(SEVEN,CLUBS);
    gs.community[3] = make_card(NINE, CLUBS);
    gs.community[4] = make_card(JACK, DIAMONDS);
    gs.communityCount = 5;
    Card pair_hand[2] = { make_card(TWO,  HEARTS), make_card(THREE, CLUBS) };
    Card high_card[2] = { make_card(KING, HEARTS), make_card(QUEEN, CLUBS) };
    TEST("pair > high card",                 evaluateHand(&gs, pair_hand) > evaluateHand(&gs, high_card));

    /* Identical hands tie */
    Card hand_a[2] = { make_card(ACE, HEARTS),   make_card(KING, HEARTS) };
    Card hand_b[2] = { make_card(ACE, DIAMONDS), make_card(KING, CLUBS)  };
    TEST("identical hands tie",              evaluateHand(&gs, hand_a) == evaluateHand(&gs, hand_b));
}

/* ================================================================
   7. AWARD — winner receives pot, loser unchanged
   ================================================================ */
static void test_award(void)
{
    printf("\n[award]\n");
    GameState gs; Deck deck;
    setup_2player(&gs, &deck);
    newHand(&gs, &deck);

    /* set up a known royal flush for player 0 */
    gs.community[0] = make_card(TEN,   SPADES);
    gs.community[1] = make_card(JACK,  SPADES);
    gs.community[2] = make_card(QUEEN, SPADES);
    gs.community[3] = make_card(KING,  SPADES);
    gs.community[4] = make_card(TWO,   HEARTS);
    gs.communityCount = 5;

    gs.players[0].hand[0]  = make_card(ACE,  SPADES);
    gs.players[0].hand[1]  = make_card(THREE, CLUBS);
    gs.players[0].has_cards = 1;
    gs.players[0].status   = PLAYER_PLAYING;

    gs.players[1].hand[0]  = make_card(TWO,  CLUBS);
    gs.players[1].hand[1]  = make_card(FOUR, CLUBS);
    gs.players[1].has_cards = 1;
    gs.players[1].status   = PLAYER_PLAYING;

    /* award() builds (side)pots from each player's total_bet, not gs.pot:
       fund a 200 pot with 100 from each contestant. */
    gs.players[0].total_bet = 100;
    gs.players[1].total_bet = 100;
    gs.pot = 200;
    uint32_t winner_before = gs.players[0].chips;
    uint32_t loser_before  = gs.players[1].chips;

    award(&gs);
    TEST("winner receives pot",       gs.players[0].chips == winner_before + 200);
    TEST("loser chips unchanged",     gs.players[1].chips == loser_before);
}

/* ================================================================
   8. SHOWDOWN FLAG
   ================================================================ */
static void test_showdown(void)
{
    printf("\n[showdown]\n");
    GameState gs; Deck deck;
    setup_2player(&gs, &deck);
    newHand(&gs, &deck);

    uint8_t cur   = gs.currentPlayer;
    uint8_t other = (cur == 0) ? 1 : 0;
    gs.players[other].status = PLAYER_FOLDED;

    processMove(&gs, &deck, cur);

    TEST("showdown set after last fold",     gs.phase == HAND_COMPLETE);
    TEST("handPlaying false after showdown", gs.handPlaying == false);
    TEST("winnerID correct",                 gs.winnerID == cur);

    resetHand(&gs);
    TEST("showdown cleared by resetHand",   gs.phase == HAND_IDLE);
    TEST("pot zeroed by resetHand",         gs.pot == 0);
}

/* ================================================================
   9. RESET / NEW HAND
   ================================================================ */
static void test_reset(void)
{
    printf("\n[resetHand / newHand]\n");
    GameState gs; Deck deck;
    setup_2player(&gs, &deck);
    newHand(&gs, &deck);

    TEST("player 0 has cards after newHand", gs.players[0].has_cards == 1);
    TEST("player 1 has cards after newHand", gs.players[1].has_cards == 1);
    TEST("handPlaying true",                 gs.handPlaying == true);
    TEST("stage is PREFLOP",                 gs.stage == PREFLOP);
    TEST("pot includes blinds",              gs.pot > 0);

    resetHand(&gs);
    TEST("handPlaying false after reset",    gs.handPlaying == false);
    TEST("pot zeroed after reset",           gs.pot == 0);
    TEST("no cards after reset (p0)",        gs.players[0].has_cards == 0);
    TEST("no cards after reset (p1)",        gs.players[1].has_cards == 0);
}

/* ================================================================
   10. FULL HAND PROGRESSION
   ================================================================ */
static void test_full_hand(void)
{
    printf("\n[full hand progression]\n");
    GameState gs; Deck deck;
    setup_2player(&gs, &deck);
    newHand(&gs, &deck);

    int moves = 0;
    while (gs.handPlaying && moves < 40) {
        uint8_t cur = gs.currentPlayer;
        MoveType move;
        if      (validate(&gs, cur, CHECK, 0)) move = CHECK;
        else if (validate(&gs, cur, CALL,  0)) move = CALL;
        else                                    move = FOLD;
        tryMove(&gs, &deck, cur, move, 0);
        moves++;
    }

    TEST("hand ends within 40 moves",  !gs.handPlaying);
    TEST("showdown set at end",         gs.phase == HAND_COMPLETE);
}

/* ================================================================
   MAIN
   ================================================================ */
int main(void)
{
    printf("=== Anteater Poker — Unit Tests (game logic) ===\n");
    test_fold();
    test_check();
    test_call();
    test_raise();
    test_allin();
    test_evaluate();
    test_award();
    test_showdown();
    test_reset();
    test_full_hand();

    printf("\n================================================\n");
    printf("Score: %d / %d  (%d failed)\n", passed, passed + failed, failed);
    return failed > 0 ? 1 : 0;
}

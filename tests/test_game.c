#include <assert.h>
#include <stdio.h>
#include "../game.h"

/* 
    UNIT TESTING FOR game.c
*/

// HELPER FUNCTIONS

/*
    initializeThreePlayers()

    This helper function initializes three poker players to simulate the poker table play. 
    All three players' statuses will be initially set to 'PLAYER_PLAYING' and will each have 1000 credits. 
    The first player becomes small blind, the second player becomes big blind and must match the small blind. 
*/
void initializeThreePlayers(GameState *gs, Deck *deck, uint32_t initialPot)
{
    // GameState gs;
    // Deck deck;

    memset(gs, 0, sizeof(GameState));
    memset(deck, 0, sizeof(Deck));

    // Initialize two players
    initPlayer(&gs->players[0], 0, "Apple", 1000);
    initPlayer(&gs->players[1], 1, "Banana", 1000);
    initPlayer(&gs->players[2], 2, "Carrot", 1000);

    // Set both players as active
    gs->players[0].status = PLAYER_PLAYING;
    gs->players[1].status = PLAYER_PLAYING;
    gs->players[2].status = PLAYER_PLAYING;

    // Set game state
    gs->playerCount = 2;
    gs->stage = PREFLOP;

    // Assign dealer and blinds
    gs->currentPlayer = 0;
    gs->dealerIndex = 1;

    gs->currentBet = 100;
    gs->minRaise = BIG_BLIND;

    gs->pot = initialPot;
    gs->handPlaying = true;

    gs->players[0].current_bet = 50;
}


/*
    test_tryMove_fold()

    Testing: bool tryMove(GameState* gs, Deck* deck, uint8_t playerID, MoveType move, uint32_t amount)
*/
void test_tryMove_fold()
{
    GameState gs;
    Deck deck;

    uint32_t initialPotAmt = 200;
    initializeThreePlayers(&gs, &deck, initialPotAmt);

    // Try FOLD move
    bool result = tryMove(&gs, &deck, 0, FOLD, 0);

    // // Verify (assert) all resulting consequences of a CALL move are correct
    // assert(result == true); // move succeeded
    // assert(gs.players[0].chips == 950); // player has 50 less chips
    // assert(gs.players[0].current_bet == 100); // player matched the bet
    // assert(gs.pot == 50); // pot increased to 50
    // assert(gs.acted[0] == true); // player made and completed a move

    // Verify (assert) all resulting consequences of a FOLD move are correct
    assert(result == true);                         // move succeeded
    assert(gs.players[0].status == PLAYER_FOLDED);  // player status should be updated from "PLAYER_PLAYING" to "PLAYER_FOLDED"
                                                        // must be 3+ players for status to be "PLAYER_FOLDED"... otherwise, game auto resets
    assert(gs.players[0].chips == 1000);            // player still has the same amount of chips (bc fold)
    assert(gs.pot == initialPotAmt);                // pot before and after did not change
    assert(gs.acted[0] == true);                    // player made and completed a move

    printf("test_tryMove_fold PASSED\n");
}


/*
    test_tryMove_check()
*/
void test_tryMove_check()
{
    GameState gs;
    Deck deck;

    uint32_t initialPotAmt = 200;
    initializeThreePlayers(&gs, &deck, initialPotAmt);

    // Player must already match the current bet to check
    gs.players[0].current_bet = gs.currentBet;

    // Store initial values for comparison
    uint32_t initialChips = gs.players[0].chips;

    // Try CHECK move
    bool result = tryMove(&gs, &deck, 0, CHECK, 0);

    // Verify (assert) all resulting consequences of a CHECK move are correct
    assert(result == true);                             // move succeeded
    assert(gs.players[0].status == PLAYER_PLAYING);     // player status should not change from "PLAYER_PLAYING" 
    assert(gs.players[0].chips == initialChips);        // player still has the same amount of chips (bc check)
    assert(gs.pot == initialPotAmt);                    // pot before and after did not change
    assert(gs.players[0].current_bet == gs.currentBet); // current bet before and after did not change
    assert(gs.acted[0] == true);                        // player made and completed a move

    printf("test_tryMove_check PASSED\n");
}


/*
    test_tryMove_call()
*/
void test_tryMove_call()
{
    GameState gs;
    Deck deck;

    uint32_t initialPotAmt = 200;
    initializeThreePlayers(&gs, &deck, initialPotAmt);

    // Player should not match the current bet so they can call

    // Store initial values for comparison
    uint32_t initialChips = gs.players[0].chips;
    uint32_t initialBet = gs.players[0].current_bet;

    // Store call amount for comparison
    uint32_t callAmt = gs.currentBet - initialBet;

    // Try CALL move
    bool result = tryMove(&gs, &deck, 0, CALL, 0);

    // Verify (assert) all resulting consequences of a CALL move are correct
    assert(result == true);                                 // move succeeded
    assert(gs.players[0].status == PLAYER_PLAYING);         // player status should not change from "PLAYER_PLAYING" 
    assert(gs.players[0].chips == initialChips - callAmt);  // player should have callAmt less chips after calling
    assert(gs.pot == initialPotAmt + callAmt);              // pot should increase by callAmt
    assert(gs.players[0].current_bet == gs.currentBet);     // current bet before and after did not change
    assert(gs.acted[0] == true);                            // player made and completed a move

    printf("test_tryMove_call PASSED\n");
}


/*
    test_tryMove_raise()
*/
void test_tryMove_raise()
{
    GameState gs;
    Deck deck;

    uint32_t initialPotAmt = 200;
    initializeThreePlayers(&gs, &deck, initialPotAmt);

    // Set the raise amount to 200 (bet increases by 200)
    uint32_t raiseAmt = 200;

    // Store initial values for comparison
    uint32_t initialChips = gs.players[0].chips;
    uint32_t initialBet = gs.players[0].current_bet;

    // Determine contribution amount for player in order to raise the table bet by 200
    uint32_t contributionAmt = (gs.currentBet - initialBet) + raiseAmt;

    // Try RAISE move
    bool result = tryMove(&gs, &deck, 0, RAISE, raiseAmt);

    // Verify (assert) all resulting consequences of a RAISE move are correct
    assert(result == true);                                             // move succeeded
    assert(gs.players[0].status == PLAYER_PLAYING);                     // player status should not change from "PLAYER_PLAYING" 
    assert(gs.players[0].chips == initialChips - contributionAmt);      // player should have contributionAmt less chips after raising
    assert(gs.pot == initialPotAmt + contributionAmt);                  // pot should increase by contributionAmt
    assert(gs.players[0].current_bet == initialBet + contributionAmt);  // current bet should increase by contributionAmt
    assert(gs.acted[0] == true);                                        // player made and completed a move

    printf("test_tryMove_raise PASSED\n");
}


/*
    test_tryMove_allIn()
*/
void test_tryMove_allIn()
{
    GameState gs;
    Deck deck;

    uint32_t initialPotAmt = 200;
    initializeThreePlayers(&gs, &deck, initialPotAmt);

    // Store initial values for comparison
    uint32_t initialChips = gs.players[0].chips;
    uint32_t initialBet = gs.players[0].current_bet;

    // Try ALL_IN move
    bool result = tryMove(&gs, &deck, 0, ALL_IN, 0);

    // Verify (assert) all resulting consequences of a ALL_IN move are correct
    assert(result == true);                                         // move succeeded
    assert(gs.players[0].status == PLAYER_ALL_IN);                  // player status should change from "PLAYER_PLAYING" to "PLAYER_ALL_IN"
    assert(gs.players[0].chips == 0);                               // player should not have any chips left if going all in
    assert(gs.pot == initialPotAmt + initialChips);                 // pot should increase by initialChips, since player puts all their chips in
    assert(gs.players[0].current_bet == initialBet + initialChips); // current bet before and after did not change
    assert(gs.acted[0] == true);                                    // player made and completed a move

    printf("test_tryMove_allIn PASSED\n");
}

// ********** SPECIAL CARDS **********

/*
    test_tryMove_useSpecialCard()
*/
// void test_tryMove_useSpecialCard_swapCard()
// {
//     GameState gs;
//     Deck deck;

//     uint32_t initialPotAmt = 200;
//     initializeThreePlayers(&gs, &deck, initialPotAmt);

//     // Player should not match the current bet so they can call

//     // Store initial values for comparison
//     uint32_t initialChips = gs.players[0].chips;
//     uint32_t initialBet = gs.players[0].current_bet;

//     // Store call amount for comparison
//     uint32_t callAmt = gs.currentBet - initialBet;

//     // Try CALL move
//     bool result = tryMove(&gs, &deck, 0, USE_SPECIAL_CARD, 0);

//     // Verify (assert) all resulting consequences of a USE_SPECIAL_CARD move are correct
//     assert(result == true);                                 // move succeeded
//     assert(gs.acted[0] == true);                            // player made and completed a move

//     printf("test_tryMove_useSpecialCard PASSED\n");
// }


int main()
{
    test_tryMove_fold();
    test_tryMove_check();
    test_tryMove_call();
    test_tryMove_raise();
    test_tryMove_allIn();

    return 0;
}
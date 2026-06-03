#include <assert.h>
#include <stdio.h>
#include "../game.h"
#include "../specialCards.h"

/* 
    UNIT TESTING
*/

#define GREEN "\033[32m"
#define RESET "\033[0m"

// ========================================
// *********** HELPER FUNCTIONS ***********
// ========================================

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


// ========================================
// ************* TEST CASES ***************
// ========================================

// ********** TRY_MOVE **********

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

    printf("test_tryMove_allIn " GREEN "PASSED\n" RESET);
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

    printf("test_tryMove_allIn " GREEN "PASSED\n" RESET);
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

    printf("test_tryMove_call " GREEN "PASSED\n" RESET);
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

    printf("test_tryMove_raise " GREEN "PASSED\n" RESET);
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

    printf("test_tryMove_allIn " GREEN "PASSED\n" RESET);
}


// ********** SPECIAL CARDS **********

/*
    test_tryMove_useSpecialCard()
*/
// void test_tryMove_useSpecialCard_swapCard()
// {
//    // ** NOT IMPLEMENTED YET IN tryMove() IN game.c
// }

/*
    test_swapCard()

    Testing: void swapCard(GameState *gs, int myID, int oppID)
    directly from specialCards
    Both cards get swapped
*/
void test_swapCard()
{
    GameState gs;

    memset(&gs, 0, sizeof(GameState));

    // Initialize two players
    initPlayer(&gs.players[0], 0, "Apple", 1000);
    initPlayer(&gs.players[1], 1, "Banana", 1000);

    // Assign first player two cards
    gs.players[0].hand[0].rank = ACE;
    gs.players[0].hand[0].suit = SPADES;

    gs.players[0].hand[1].rank = KING;
    gs.players[0].hand[1].suit = HEARTS;

    // Assign second player two cards
    gs.players[1].hand[0].rank = TWO;
    gs.players[1].hand[0].suit = CLUBS;

    gs.players[1].hand[1].rank = THREE;
    gs.players[1].hand[1].suit = DIAMONDS;

    // Save original hands of each player
    Card playerOneCardOne = gs.players[0].hand[0];
    Card playerOneCardTwo = gs.players[0].hand[1];

    Card playerTwoCardOne = gs.players[1].hand[0];
    Card playerTwoCardTwo = gs.players[1].hand[1];

    // try SWAP CARD (BOTH cards get swapped)
    swapCard(&gs, 0, 1);

    // Verify that the first cards are swapped
    assert(gs.players[0].hand[0].rank == playerTwoCardOne.rank);
    assert(gs.players[0].hand[0].suit == playerTwoCardOne.suit);

    assert(gs.players[1].hand[0].rank == playerOneCardOne.rank);
    assert(gs.players[1].hand[0].suit == playerOneCardOne.suit);

    // Verify that the second cards are also swapped
    assert(gs.players[0].hand[1].rank == playerTwoCardTwo.rank);
    assert(gs.players[0].hand[1].suit == playerTwoCardTwo.suit);

    assert(gs.players[1].hand[1].rank == playerOneCardTwo.rank);
    assert(gs.players[1].hand[1].suit == playerOneCardTwo.suit);

    printf("test_swapCard " GREEN "PASSED\n" RESET);
}


/*
    test_swapCards()

    Testing: void swapCards(GameState *gs, int myID, int myCardIdx, int oppID, int oppCardIdx)
    directly from specialCards
    Only one card get swapped
*/
void test_swapCards()
{
    GameState gs;

    memset(&gs, 0, sizeof(GameState));

    // Initialize two players
    initPlayer(&gs.players[0], 0, "Apple", 1000);
    initPlayer(&gs.players[1], 1, "Banana", 1000);

    // Assign first player two cards
    gs.players[0].hand[0].rank = ACE;
    gs.players[0].hand[0].suit = SPADES;

    gs.players[0].hand[1].rank = KING;
    gs.players[0].hand[1].suit = HEARTS;

    // Assign second player two cards
    gs.players[1].hand[0].rank = TWO;
    gs.players[1].hand[0].suit = CLUBS;

    gs.players[1].hand[1].rank = THREE;
    gs.players[1].hand[1].suit = DIAMONDS;

    // Save original hands of each player
    Card playerOneCardOne = gs.players[0].hand[0];
    Card playerOneCardTwo = gs.players[0].hand[1];

    Card playerTwoCardOne = gs.players[1].hand[0];
    Card playerTwoCardTwo = gs.players[1].hand[1];

    // try SWAP CARDS (ONE card gets swapped)
    swapCards(&gs, 0, 0, 1, 1); // swap p1's first card with p2's second card

    // Verify that the correct cards are swapped
    assert(gs.players[0].hand[0].rank == playerTwoCardTwo.rank);
    assert(gs.players[0].hand[0].suit == playerTwoCardTwo.suit);

    assert(gs.players[1].hand[1].rank == playerOneCardOne.rank);
    assert(gs.players[1].hand[1].suit == playerOneCardOne.suit);

    // Verify that the other cards were not swapped
    assert(gs.players[0].hand[1].rank == playerOneCardTwo.rank);
    assert(gs.players[0].hand[1].suit == playerOneCardTwo.suit);

    assert(gs.players[1].hand[0].rank == playerTwoCardOne.rank);
    assert(gs.players[1].hand[0].suit == playerTwoCardOne.suit);

    printf("test_swapCards " GREEN "PASSED\n" RESET);
}


/*
    test_redrawCards()

    Testing: redrawCards(GameState *gs, Deck *deck, int myID, int myCardIdx)
    directly from specialCards
    Only one card gets redrawn from the deck
*/
void test_redrawCards()
{
    GameState gs;
    Deck deck;

    memset(&gs, 0, sizeof(GameState));
    memset(&deck, 0, sizeof(Deck));

    // Initialize one player
    initPlayer(&gs.players[0], 0, "Apple", 1000);

    // Assign player two cards
    gs.players[0].hand[0].rank = ACE;
    gs.players[0].hand[0].suit = SPADES;

    gs.players[0].hand[1].rank = KING;
    gs.players[0].hand[1].suit = HEARTS;

    // Assign a known card to the top of the deck
    deck.cards[0].rank = TWO;
    deck.cards[0].suit = CLUBS;

    deck.top = 0;

    // Save original cards
    Card cardOne = gs.players[0].hand[0];
    Card cardTwo = gs.players[0].hand[1];

    // Redraw first card in hand
    redrawCards(&gs, &deck, 0, 0);

    // Verify first card was replaced by the top deck card
    assert(gs.players[0].hand[0].rank == TWO);
    assert(gs.players[0].hand[0].suit == CLUBS);

    // Verify second card was unchanged
    assert(gs.players[0].hand[1].rank == cardTwo.rank);
    assert(gs.players[0].hand[1].suit == cardTwo.suit);

    // Verify the first card actually changed
    assert(gs.players[0].hand[0].rank != cardOne.rank || gs.players[0].hand[0].suit != cardOne.suit);

    // Verify the deck advanced by one card
    assert(deck.top == 1);

    printf("test_redrawCards " GREEN "PASSED\n" RESET);
}


/*
    test_revealComCard()

    Testing: revealComCard(GameState *gs, Deck *deck)
    directly from specialCards
    Reveals only the next community card
*/
void test_revealComCard()
{
    GameState gs;
    Deck deck;

    memset(&gs, 0, sizeof(GameState));
    memset(&deck, 0, sizeof(Deck));

    // Start with no community cards revealed
    gs.communityCount = 0;

    // Put a known card on top of the deck
    deck.cards[0].rank = ACE;
    deck.cards[0].suit = SPADES;
    deck.top = 0;

    // Use special card
    revealComCard(&gs, &deck);

    // Verify the card was added to the community cards
    assert(gs.community[0].rank == ACE);
    assert(gs.community[0].suit == SPADES);

    // Verify community card count increased
    assert(gs.communityCount == 1);

    // Verify deck advanced by one card
    assert(deck.top == 1);

    printf("test_revealComCard " GREEN "PASSED\n" RESET);
}


/*
    test_revealOppCard()

    Testing: revealOppCard(GameState *gs, int oppID, int oppCardIdx)
    directly from specialCards
    Essentially, revealOppCard() does nothing, because the opponent's card is already known from the server's side
*/
void test_revealOppCard()
{
    GameState gs;

    memset(&gs, 0, sizeof(GameState));

    // Initialize two players
    initPlayer(&gs.players[0], 0, "Apple", 1000);
    initPlayer(&gs.players[1], 1, "Banana", 1000);

    // Give opponent known cards
    gs.players[1].hand[0].rank = ACE;
    gs.players[1].hand[0].suit = SPADES;

    gs.players[1].hand[1].rank = KING;
    gs.players[1].hand[1].suit = HEARTS;

    // Save state before function call
    Card cardOne = gs.players[1].hand[0];
    Card cardTwo = gs.players[1].hand[1];

    // Reveal opponent's first card
    revealOppCard(&gs, 1, 0);

    // Verify nothing changed in gamestate
    assert(gs.players[1].hand[0].rank == cardOne.rank);
    assert(gs.players[1].hand[0].suit == cardOne.suit);

    assert(gs.players[1].hand[1].rank == cardTwo.rank);
    assert(gs.players[1].hand[1].suit == cardTwo.suit);

    printf("test_revealOppCard " GREEN "PASSED\n" RESET);
}


/*
    test_swapOppCards()

    Testing: swapOppCards(GameState *gs, int oppID)
    directly from specialCards
    Swap your opponent's cards (not your own)
*/
void test_swapOppCards()
{
    GameState gs;

    memset(&gs, 0, sizeof(GameState));

    // Initialize two players
    initPlayer(&gs.players[0], 0, "Apple", 1000);
    initPlayer(&gs.players[1], 1, "Banana", 1000);

    // Give opponent (player 1) two known cards
    gs.players[1].hand[0].rank = ACE;
    gs.players[1].hand[0].suit = SPADES;

    gs.players[1].hand[1].rank = KING;
    gs.players[1].hand[1].suit = HEARTS;

    // Give player cards so we can verify they remain the same before and after swap
    gs.players[0].hand[0].rank = TWO;
    gs.players[0].hand[0].suit = CLUBS;

    gs.players[0].hand[1].rank = THREE;
    gs.players[0].hand[1].suit = DIAMONDS;

    // Save original hands
    Card oppCardOne = gs.players[1].hand[0];
    Card oppCardTwo = gs.players[1].hand[1];

    Card playerCardOne = gs.players[0].hand[0];
    Card playerCardTwo = gs.players[0].hand[1];

    // Use special card
    swapOppCards(&gs, 1);

    // Verify opponent's cards were swapped
    assert(gs.players[1].hand[0].rank == oppCardTwo.rank);
    assert(gs.players[1].hand[0].suit == oppCardTwo.suit);

    assert(gs.players[1].hand[1].rank == oppCardOne.rank);
    assert(gs.players[1].hand[1].suit == oppCardOne.suit);

    // Verify player's cards were not changed
    assert(gs.players[0].hand[0].rank == playerCardOne.rank);
    assert(gs.players[0].hand[0].suit == playerCardOne.suit);

    assert(gs.players[0].hand[1].rank == playerCardTwo.rank);
    assert(gs.players[0].hand[1].suit == playerCardTwo.suit);
    
    printf("test_swapOppCards " GREEN "PASSED\n" RESET);
}


/*
    test_instaWin()

    Testing: instaWin(GameState *gs, int myID)
    directly from specialCards
    Instawin card! (so powerful)
*/
void test_instaWin()
{
    GameState gs;

    memset(&gs, 0, sizeof(GameState));

    // Initialize game state
    gs.gameOver = 0;
    gs.winnerID = -1;

    int winnerID = 0;

    // Use special card
    instaWin(&gs, winnerID);

    // Verify game immediately ends - gameOver
    assert(gs.gameOver == 1);

    // Verify correct winner is recorded
    assert(gs.winnerID == winnerID);

    printf("test_instaWin " GREEN "PASSED\n" RESET);
}


int main()
{
    // Test cases for tryMove()
    test_tryMove_fold();
    test_tryMove_check();
    test_tryMove_call();
    test_tryMove_raise();
    test_tryMove_allIn();
    // test_tryMove_useSpecialCard(); // ** NOT YET IMPLEMENTED

    // Test cases for special cards
    test_swapCard();
    test_swapCards();
    test_redrawCards();
    test_revealComCard();
    test_revealOppCard();
    test_swapOppCards();
    test_instaWin();

    return 0;
}
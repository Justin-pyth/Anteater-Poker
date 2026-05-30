//universal definitions header for poker game
#ifndef UDS_H
#define UDS_H

#include<stdint.h>
#include <stdbool.h>

#define PROTOCOL_HEADER_SIZE 6
#define MAX_PAYLOAD_SIZE 4096
#define MAX_PLAYERS 6
#define MAX_NAME_LENTH 32
#define DECK_SIZE 52
#define SPECIAL_CARDS 4
#define HAND_SIZE 2
#define SMALL_BLIND 5
#define BIG_BLIND 10
#define INIT_CHIPS 1000

//enum of move type.
typedef enum {
    FOLD,
    CHECK,
    CALL,
    RAISE,
    ALL_IN,
    USE_SPECIAL_CARD
} MoveType;

//enum of stage.
typedef enum {
    PREFLOP,
    FLOP,
    TURN,
    RIVER
} Stage;

//server-side hand FSM state (NOT sent on the wire; clients still use stage/handPlaying/winnerID)
typedef enum {
    HAND_IDLE,      // no hand in progress (between hands / before start)
    HAND_BETTING,   // waiting on currentPlayer to act
    HAND_RUNOUT,    // all-in: remaining community cards revealed one tick at a time
    HAND_COMPLETE   // hand decided; winner known, waiting out the inter-hand delay
} HandPhase;

//enum of suit.
typedef enum {
    UNKNOW_S,// for hiding card information when sending data to other players.
    HEARTS,
    DIAMONDS,
    CLUBS,
    SPADES
} Suit;

//enum of rank.
typedef enum{
    UNKNOW_R,// for hiding card information when sending data to other players.
    TWO,
    THREE,
    FOUR,
    FIVE,
    SIX,
    SEVEN,
    EIGHT,
    NINE,
    TEN,
    JACK,
    QUEEN,
    KING,
    ACE
} Rank;

//enum of player status.
typedef enum {
    PLAYER_EMPTY,
    PLAYER_CONNECTED,
    PLAYER_READY,
    PLAYER_PLAYING,
    PLAYER_SPECTATING,
    PLAYER_FOLDED,
    PLAYER_ALL_IN,
    PLAYER_DISCONNECTED
} PlayerStatus;

//struct of card, cotains 8 bits suit and 8 bit rank. 
//we want a fixed size of card struct for easy econding and decoding when sending data over network.
typedef struct {
    uint8_t suit;
    uint8_t rank;
    uint8_t value;
} Card;

//struct of deck, cotains 52 cards and N special cards, 8 bits top to indicate the top card index in the deck.
typedef struct{
    Card *cards[5];
} CommunityCards;

//struct of deck, cotains 52 cards and N special cards, 8 bits top to indicate the top card index in the deck.
typedef struct{
    Card cards[DECK_SIZE + SPECIAL_CARDS];
    uint8_t top;
} Deck;
//struct of player, contains 8 bits id, 32 bytes name, 32 bits chips, 32 bits current bet, 8 bits status and 8 bits has_cards.
typedef struct {
    uint8_t id;
    char name[MAX_NAME_LENTH];
    Card hand[HAND_SIZE];
    uint32_t chips;
    uint32_t current_bet;

    uint8_t status;
    uint8_t has_cards;
    uint8_t place;
}   Player;

//struct for rankings of poker hands
typedef enum {
    HIGHT_CARD,
    PAIR,
    TWO_PAIR,
    THREE_KIND,
    STRAIGHT,
    FLUSH,
    FULL_HOUSE,
    FOUR_KIND,
    STRAIGHT_FLUSH,
    ROYAL_FLUSH
} Rankings;

//Struct for player's current highest hand
typedef struct {
    Card *cards[5];
    int handType; // 0-9 (High Card to Royal Flush)
    int score;    // For tie-breaking
} HighHand;
 typedef enum {
    SWAP1, //swap 1 card with opponent
	SWAP2, //swap 2 cards with opponent
	REVEAL, //reveal next community card
	REDRAW, //redraw one of your cards from the main deck
	INSTAWIN, // win the game
    SWAPOPS // swap your opponents cards
} Anteater_shop;

//struct that current game state (broadcast to all players)
// excludes the deck and player cards which are server-side
typedef struct {
    Player players[MAX_PLAYERS];
    uint8_t playerCount;
    //uint8_t turn;
    Card community[5];
    uint8_t communityCount;  // how many community cards are revealed
    Anteater_shop cardPrice[6]; // price of each card in the anteater shop, 6 cards in total, including 4 special cards and 2 random community cards.
    uint8_t stage;           // use Stage enum
    uint8_t currentPlayer;
    uint8_t dealerIndex;
    uint8_t yourPlayerID;
    uint8_t smallBlindIndex;
    uint8_t bigBlindIndex;

    uint32_t smallBlind;
    uint32_t bigBlind;
    uint32_t pot;
    uint32_t currentBet;     // bet needed to match
    uint32_t minRaise;
    uint8_t gameOver;
    uint8_t winnerID;
    //non encoded
    uint8_t lastActor;
    bool handPlaying; //for starting new hands after reset
    bool acted[MAX_PLAYERS];
    HandPhase phase;  //hand FSM state; drives the server loop (runout pacing, inter-hand delay)
  
} GameState;
typedef struct {
    uint8_t playerID;
    MoveType move;
    uint32_t amount;// same as chips. It is duplicate right now. But will comebine back with chips in the future.
    uint8_t target;
    Anteater_shop useSpecialCard;
} PlayerAction;

#endif

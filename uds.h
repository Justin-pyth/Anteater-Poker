//universal definitions header for poker game
#ifndef UDS_H
#define UDS_H

#include<stdint.h>
#include <stdbool.h>

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
    ALL_IN
} MoveType;

//enum of stage.
typedef enum {
    PREFLOP,
    FLOP,
    TURN,
    RIVER
} Stage;

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


//struct of player hand, cotains 2 cards. This is for the client to see their own hand
// the server will not send the hand to other players.
typedef struct {
    Card hand[HAND_SIZE];
}   PlayerHand;

//struct of deck, cotains 52 cards and N special cards, 8 bits top to indicate the top card index in the deck.
typedef struct{
    Card cards[DECK_SIZE + SPECIAL_CARDS];
    uint8_t top;
} Deck;
//struct of player, contains 8 bits id, 32 bytes name, 32 bits chips, 32 bits current bet, 8 bits status and 8 bits has_cards.
typedef struct {
    uint8_t id;
    char name[MAX_NAME_LENTH];
    PlayerHand hand;
    uint32_t chips;
    uint32_t current_bet;

    uint8_t status;
    uint8_t has_cards;

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

//struct that current game state (broadcast to all players)
// excludes the deck and player cards which are server-side
typedef struct {
    Player players[MAX_PLAYERS];
    uint8_t playerCount;
    //uint8_t turn;
    Card community[5];
    uint8_t communityCount;  // how many community cards are revealed

    uint8_t stage;           // use Stage enum
    uint8_t currentPlayer;
    uint8_t dealerIndex;
    //uint8_t yourIndex;

    uint32_t pot;
    uint32_t currentBet;     // bet needed to match
    uint32_t minRaise;

    bool handPlaying; //for starting new hands after reset
    bool acted[MAX_PLAYERS];
} GameState;
typedef struct {
    uint8_t playerID;
    MoveType move;
    uint32_t amount;// same as chips. It is duplicate right now. But will comebine back with chips in the future.
} PlayerAction;

#endif
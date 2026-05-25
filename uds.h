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

static const int INSTA_WIN_PRICE = 800;
static const int SWAP1_PRICE = 100;
static const int SWAP2P_RICE = 200;
static const int REVEAL_PRICE = 150;
static const int REDRAW_PRICE = 200;
static const int SWAP_OPPS_PRICE = 300;
static const int PRICES[] = {SWAP1_PRICE, SWAP2P_RICE, REVEAL_PRICE, 
                                        REDRAW_PRICE, INSTA_WIN_PRICE, SWAP_OPPS_PRICE};

//enum of move type.
typedef enum {
    FOLD,
    CHECK,
    CALL,
    RAISE,
    ALL_IN,
    USE_SPECIAL_CARD,
} MoveType;

//enum of anteater cards
typedef enum {
	SWAP1, //swap 1 card with opponent
	SWAP2, //swap 2 cards with opponent
	REVEAL, //reveal next community card
	REDRAW, //redraw one of your cards from the main deck
	INSTAWIN, // win the game
    SWAPOPS // swap your opponents cards
} anteater_shop;


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

//struct of deck, cotains 52 cards and N special cards, 8 bits top to indicate the top card index in the deck.
typedef struct{
    Card cards[DECK_SIZE];
    uint8_t top;
} Deck;

//struct of player, contains 8 bits id, 32 bytes name, 32 bits chips, 32 bits current bet, 8 bits status and 8 bits has_cards.
typedef struct {
    uint8_t id;
    char name[MAX_NAME_LENTH];
    Card hand[HAND_SIZE];
    uint32_t chips;
    uint32_t current_bet;

    anteater_shop antCards[3];
    uint8_t status;
    uint8_t has_cards;

}   Player;

typedef struct {
    uint8_t playerID;
    MoveType move;
    uint32_t amount; // same as chips. It is a duplicate right now. But will combine back with chips in the future.
    uint8_t target;
    anteater_shop useSpecialCard;
} PlayerAction;


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
    anteater_shop special_card_prize[6];
    uint8_t stage;           // use Stage enum
    uint8_t currentPlayer;
    uint8_t dealerIndex;
    uint8_t yourPlayerID;    // client-specific seat id, filled before broadcast

    uint32_t pot;
    uint32_t currentBet;     // bet needed to match
    uint32_t minRaise;
    uint8_t gameOver;
    uint8_t winnerID;
    //non-encode
    bool handPlaying; //for starting new hands after reset
    bool acted[MAX_PLAYERS];
  
} GameState;

#endif

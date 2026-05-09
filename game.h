#ifndef GAME_H
#define GAME_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>

#define MAX_PLAYERS 10  //change # of max players
#define MAX_NAMELEN 32  //change char lim for name

typedef enum {
    FOLD,
    CHECK,
    CALL,
    RAISE
} MoveType;

typedef enum {
    PREFLOP,
    FLOP,
    TURN,
    RIVER
} Stage;

typedef enum {
    SPADES,
    HEARTS,
    DIAMONDS,
    CLUBS
} Suit;

typedef struct {
    int rank;
    Suit suit;
} Card;

typedef struct {
    char userName[MAX_NAMELEN];

    int chips;
    int currentBet; //bet placed by specific player
    bool isFolded;

    Card holeCards[2];
} Player;

typedef struct {
    int playerCount;
    Player players[MAX_PLAYERS];

    Card deck[52];
    int deckCount;
    Card community[5];
    Stage stage;

    int currentPlayer;

    int pot;
    int currentBet; //bet needed to match
    int minRaise;


    int yourIndex;
    int dealerIndex;
} GameState;

/*  SERVER calls this once per non-folded player to determine
    the winner after the final betting round. The player with
    the max score should win. Will leave the server to handle
    other outcomes (ties or only 1 non-folded player).

    BOT calls this thousands of times to evaluate several outcomes
    and return a probablity for each possible move.

    Will need to handle flush, full house, etc.
*/
int evaluateHand(const GameState* gs, Player player);

#endif
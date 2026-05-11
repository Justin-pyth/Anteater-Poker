#include "game.h"

int evaluateHand(const GameState* gs, const PlayerHand* hand)
{
    (void)gs;
    (void)hand;
    return 0;
}
void shuffle(Deck* deck)
{
    (void)deck;
}
void deal(GameState* gs, Deck* deck, PlayerHand hand[MAX_PLAYERS])
{
    (void)gs;
    (void)deck;
    (void)hand;
}
bool validate(const GameState* gs, MoveType move, uint32_t amount)
{
    (void)gs;
    (void)move;
    (void)amount;
    return true;
}
void apply(GameState* gs, uint8_t playerID, MoveType move, uint32_t amount)
{
    (void)gs;
    (void)playerID;
    (void)move;
    (void)amount;
}

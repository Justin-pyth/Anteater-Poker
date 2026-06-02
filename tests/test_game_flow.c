/*  tests/flow_demo.c
    Simulate complete poker hands using bot AI — no network required.
    Drives game.c + rules.c + bot.c directly.
    Build: make tests/flow_demo
    Run:   ./tests/flow_demo
*/
#include <stdio.h>
#include <string.h>
#include "game.h"
#include "rules.h"
#include "bot.h"
#include "uds.h"

static const char *stage_name(Stage s)
{
    switch (s) {
        case PREFLOP: return "PREFLOP";
        case FLOP:    return "FLOP";
        case TURN:    return "TURN";
        case RIVER:   return "RIVER";
        default:      return "?";
    }
}

static const char *move_name(MoveType m)
{
    switch (m) {
        case FOLD:  return "FOLD";
        case CHECK: return "CHECK";
        case CALL:  return "CALL";
        case RAISE: return "RAISE";
        case ALL_IN:return "ALL_IN";
        default:    return "?";
    }
}

static void print_state(const GameState *gs)
{
    printf("  Stage: %-8s  Pot: %u  Bet: %u  Current: player %d\n",
           stage_name(gs->stage), gs->pot, gs->currentBet, gs->currentPlayer);
    for (int i = 0; i < gs->playerCount; i++) {
        const Player *p = &gs->players[i];
        printf("    [%d] %-10s  chips=%4u  bet=%3u  status=%d\n",
               i, p->name, p->chips, p->current_bet, p->status);
    }
}

/* Returns 1 if hand ended in showdown, 0 otherwise */
static int play_one_hand(GameState *gs, Deck *deck, int hand_num)
{
    printf("\n=== Hand #%d ===\n", hand_num);
    newHand(gs, deck);
    print_state(gs);

    int moves = 0;
    while (gs->handPlaying && moves < 80) {
        /* All-in: no one acts during the runout. The server normally paces
           this with runoutStep(); drive it to completion here instead. */
        if (gs->phase == HAND_RUNOUT) {
            runoutStep(gs, deck);
            continue;
        }
        uint8_t cur = gs->currentPlayer;
        MoveType move;
        uint32_t amount = 0;
        botMove(gs, deck, &cur, &move, &amount);
        printf("  Player %d (%s) -> %s%s%u\n",
               cur, gs->players[cur].name, move_name(move),
               move == RAISE ? " " : "", move == RAISE ? amount : 0);
        tryMove(gs, deck, cur, move, amount);
        moves++;
    }

    int reached_showdown = (gs->phase == HAND_COMPLETE);
    if (reached_showdown) {
        const char *winner = (gs->winnerID < MAX_PLAYERS)
            ? gs->players[gs->winnerID].name : "(tie/river)";
        printf("  -> Showdown: %s wins  (moves=%d)\n", winner, moves);
    } else {
        printf("  -> Hand ended (moves=%d, handPlaying=%d)\n",
               moves, gs->handPlaying);
    }
    print_state(gs);
    resetHand(gs);
    return reached_showdown;
}

int main(void)
{
    printf("=== Anteater Poker — Flow Demo (bot simulation) ===\n");

    GameState gs;
    Deck deck;
    memset(&gs, 0, sizeof(gs));
    initDeck(&deck);

    /* 4-player game: 1 human seat, 3 bots */
    initPlayer(&gs.players[0], 0, "Alice", INIT_CHIPS);
    initPlayer(&gs.players[1], 1, "Bot_1", INIT_CHIPS);
    initPlayer(&gs.players[2], 2, "Bot_2", INIT_CHIPS);
    initPlayer(&gs.players[3], 3, "Bot_3", INIT_CHIPS);
    for (int i = 0; i < 4; i++)
        gs.players[i].status = PLAYER_READY;
    gs.playerCount = 4;
    gs.dealerIndex  = 0;

    int hands_played = 0;
    int hands_with_showdown = 0;

    for (int h = 1; h <= 10; h++) {
        /* skip players who are bust */
        int active = 0;
        for (int i = 0; i < gs.playerCount; i++)
            if (gs.players[i].chips > 0) active++;
        if (active < 2) { printf("\nNot enough players with chips.\n"); break; }

        int sd = play_one_hand(&gs, &deck, h);
        hands_played++;
        if (sd) hands_with_showdown++;
    }

    printf("\n=== Summary: %d hands played, %d reached showdown ===\n",
           hands_played, hands_with_showdown);
    printf("Final chip counts:\n");
    for (int i = 0; i < gs.playerCount; i++)
        printf("  %-10s  %u chips\n", gs.players[i].name, gs.players[i].chips);

    return 0;
}

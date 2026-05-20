#include <stdio.h>
#include <string.h>
#include "protocol.h"
#include "com.h"

int main(void)
{
    ServerState state;
    init_server(&state);
    state.listen_fd = create_socket(NULL);
    printf("Test server listening on port %d...\n", PORT);

    Client client;
    add_connection(&state, &client);

    // dummy game state
    state.game.playerCount = 2;
    state.game.pot = 100;
    state.game.currentBet = 20;
    state.game.minRaise = 20;
    state.game.stage = FLOP;
    state.game.currentPlayer = 0;
    state.game.dealerIndex = 0;
    state.game.handPlaying = true;
    state.game.communityCount = 2;
    state.game.community[0].rank = ACE;
    state.game.community[0].suit = HEARTS;
    state.game.community[1].rank = KING;
    state.game.community[1].suit = SPADES;

    state.game.players[0].id = 1;
    state.game.players[0].chips = 980;
    state.game.players[0].current_bet = 20;
    state.game.players[0].status = PLAYER_PLAYING;
    state.game.players[0].has_cards = 1;
    state.game.players[0].hand[0].rank = ACE;
    state.game.players[0].hand[0].suit = DIAMONDS;
    state.game.players[0].hand[1].rank = ACE;
    state.game.players[0].hand[1].suit = CLUBS;
    strncpy(state.game.players[0].name, "Player1", MAX_NAME_LENTH);

    state.game.players[1].id = 2;
    state.game.players[1].chips = 980;
    state.game.players[1].current_bet = 20;
    state.game.players[1].status = PLAYER_PLAYING;
    state.game.players[1].has_cards = 1;
    state.game.players[1].hand[0].rank = KING;
    state.game.players[1].hand[0].suit = DIAMONDS;
    state.game.players[1].hand[1].rank = QUEEN;
    state.game.players[1].hand[1].suit = HEARTS;
    strncpy(state.game.players[1].name, "Player2", MAX_NAME_LENTH);

    printf("Sending game state to client...\n");
    broadcast_game_state(&state);
    printf("Game state sent. Waiting for client action...\n");

    // receive action from client
    uint8_t buffer[BUFFER_SIZE];
    ssize_t n = read(client.client_fd, buffer, sizeof(buffer));
    if (n <= 0) {
        fprintf(stderr, "ERROR reading from client\n");
        cleanup_server(&state);
        return 1;
    }

    MessageType type;
    PlayerAction action;
    if (receive_payload(buffer, (uint32_t)n, &type, &action) == 0) {
        printf("Received action:\n");
        printf("  playerID : %d\n", action.playerID);
        printf("  move     : %d\n", action.move);
        printf("  amount   : %u\n", action.amount);
    } else {
        fprintf(stderr, "ERROR decoding client action\n");
    }

    cleanup_server(&state);
    return 0;
}

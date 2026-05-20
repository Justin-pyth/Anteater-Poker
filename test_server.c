#include <stdio.h>
#include <string.h>
#include "protocol.h"
#include "com.h"

#define TEST_CLIENTS 2

int main(void)
{
    ServerState state;
    init_server(&state);
    state.listen_fd = create_socket(NULL);
    printf("Test server listening on port %d, waiting for %d clients...\n", PORT, TEST_CLIENTS);

    // accept all clients before starting
    for (int i = 0; i < TEST_CLIENTS; i++) {
        add_connection(&state, NULL);
        printf("Client %d/%d connected.\n", i + 1, TEST_CLIENTS);
    }

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

    // main loop: broadcast then collect actions from each client
    while (1) {
        printf("\n--- Broadcasting game state ---\n");
        broadcast_game_state(&state);

        // receive one action from each connected client
        int any_connected = 0;
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (!state.clients[i].connected) continue;
            any_connected = 1;

            uint8_t buffer[BUFFER_SIZE];
            ssize_t n = read(state.clients[i].client_fd, buffer, sizeof(buffer));
            if (n <= 0) {
                printf("Client %d disconnected.\n", state.clients[i].id);
                remove_client(&state, &state.clients[i]);
                continue;
            }

            MessageType type;
            PlayerAction action;
            if (receive_payload(buffer, (uint32_t)n, &type, &action) == 0) {
                printf("Action from client %d: move=%d amount=%u\n",
                       state.clients[i].id, action.move, action.amount);
            } else {
                fprintf(stderr, "ERROR decoding action from client %d\n", state.clients[i].id);
            }
        }

        if (!any_connected) {
            printf("All clients disconnected. Shutting down.\n");
            break;
        }
    }

    cleanup_server(&state);
    return 0;
}

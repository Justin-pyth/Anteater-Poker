/*  test_server.c
    Spins up a minimal server to test the full encode/decode round trip.
    Flow:
      1. Wait for TEST_CLIENTS to connect
      2. Broadcast a hardcoded GameState to all clients
      3. Use select() to receive a PlayerAction from each client in any order
      4. Repeat until all clients disconnect
*/
#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include "protocol.h"
#include "com.h"

#define TEST_CLIENTS 2

int main(void)
{
    ServerState state;
    init_server(&state);
    state.listen_fd = create_socket(NULL);
    printf("Test server listening on port %d, waiting for %d clients...\n", PORT, TEST_CLIENTS);

    // block until all test clients have connected
    for (int i = 0; i < TEST_CLIENTS; i++) {
        add_connection(&state, NULL);
        printf("Client %d/%d connected.\n", i + 1, TEST_CLIENTS);
    }

    // hardcoded game state: flop, 2 players, pot=100, bet=20
    // Player1 has AA, Player2 has KQ — card hiding should mask opponent hands on client side
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

    state.game.players[0].id = 0;
    state.game.players[0].chips = 980;
    state.game.players[0].current_bet = 20;
    state.game.players[0].status = PLAYER_PLAYING;
    state.game.players[0].has_cards = 1;
    state.game.players[0].hand[0].rank = ACE;
    state.game.players[0].hand[0].suit = DIAMONDS;
    state.game.players[0].hand[1].rank = ACE;
    state.game.players[0].hand[1].suit = CLUBS;
    strncpy(state.game.players[0].name, "Player1", MAX_NAME_LENTH);

    state.game.players[1].id = 1;
    state.game.players[1].chips = 980;
    state.game.players[1].current_bet = 20;
    state.game.players[1].status = PLAYER_PLAYING;
    state.game.players[1].has_cards = 1;
    state.game.players[1].hand[0].rank = KING;
    state.game.players[1].hand[0].suit = DIAMONDS;
    state.game.players[1].hand[1].rank = QUEEN;
    state.game.players[1].hand[1].suit = HEARTS;
    strncpy(state.game.players[1].name, "Player2", MAX_NAME_LENTH);

    printf("All clients connected. Starting test loop.\n");

    while (1) {
        // send current game state to every connected client
        printf("\n--- Broadcasting game state ---\n");
        broadcast_game_state(&state);

        // count how many actions we expect this round
        int actions_received = 0;
        int expected = 0;
        for (int i = 0; i < MAX_CLIENTS; i++)
            if (state.clients[i].connected) expected++;

        // keep receiving until every connected client has sent one action
        while (actions_received < expected) {
            fd_set read_fds;
            FD_ZERO(&read_fds);
            int max_fd = 0;

            // add every connected client fd to the watch set
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (!state.clients[i].connected) continue;
                FD_SET(state.clients[i].client_fd, &read_fds);
                if (state.clients[i].client_fd > max_fd)
                    max_fd = state.clients[i].client_fd;
            }

            // block until at least one client sends data
            int activity = select(max_fd + 1, &read_fds, NULL, NULL, NULL);
            if (activity < 0) {
                perror("select error");
                break;
            }

            // check which clients are ready and read their action
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (!state.clients[i].connected) continue;
                if (!FD_ISSET(state.clients[i].client_fd, &read_fds)) continue;

                uint8_t buffer[BUFFER_SIZE];
                ssize_t n = read(state.clients[i].client_fd, buffer, sizeof(buffer));
                if (n <= 0) {
                    printf("Client %d disconnected.\n", state.clients[i].id);
                    remove_client(&state, &state.clients[i]);
                    expected--;
                    continue;
                }

                // decode the payload and print what was received
                MessageType type;
                PlayerAction action;
                if (receive_payload(buffer, (uint32_t)n, &type, &action) == 0) {
                    printf("Action from client %d: move=%d amount=%u\n",
                           state.clients[i].id, action.move, action.amount);
                } else {
                    fprintf(stderr, "ERROR decoding action from client %d\n", state.clients[i].id);
                }
                actions_received++;
            }
        }

        // stop the loop if no clients remain
        if (expected == 0) {
            printf("All clients disconnected. Shutting down.\n");
            break;
        }
    }

    cleanup_server(&state);
    return 0;
}

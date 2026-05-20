#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "protocol.h"
#include "com.h"

int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "usage: %s hostname\n", argv[0]);
        return 1;
    }

    ClientState client;
    init_client_state(&client);

    int sockfd = connect_to_server(argv[1], PORT);
    if (sockfd < 0) {
        fprintf(stderr, "Failed to connect to server\n");
        return 1;
    }
    client.socket_fd = sockfd;
    client.connected = 1;
    client.my_player_id = 1;
    printf("Connected to server.\n");

    // main loop: receive game state then send action
    while (client.connected) {
        // receive game state
        handle_server_communication(&client);
        if (!client.connected) break;

        // print received game state
        printf("\n--- Game State ---\n");
        printf("  pot=%u  currentBet=%u  stage=%d  currentPlayer=%d\n",
               client.game.pot, client.game.currentBet,
               client.game.stage, client.game.currentPlayer);
        for (int i = 0; i < client.game.playerCount; i++) {
            printf("  [%d] %s chips=%u hand=(%d/%d),(%d/%d)\n",
                   client.game.players[i].id,
                   client.game.players[i].name,
                   client.game.players[i].chips,
                   client.game.players[i].hand[0].rank,
                   client.game.players[i].hand[0].suit,
                   client.game.players[i].hand[1].rank,
                   client.game.players[i].hand[1].suit);
        }

        // send a fixed action back
        PlayerAction action;
        action.playerID = client.my_player_id;
        action.move = CALL;
        action.amount = 20;

        uint8_t buffer[BUFFER_SIZE];
        uint32_t len = prepare_payload(buffer, MSG_TYPE_PLAYER_ACTION, &action);
        send_to_server(&client, buffer, len);
        printf("Sent: CALL amount=20\n");
    }

    close(client.socket_fd);
    printf("Disconnected.\n");
    return 0;
}

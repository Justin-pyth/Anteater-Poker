#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "protocol.h"
#include "com.h"

int main(void)
{
    ClientState client;
    init_client_state(&client);

    int sockfd = connect_to_server("localhost", PORT);
    if (sockfd < 0) {
        fprintf(stderr, "Failed to connect to server\n");
        return 1;
    }
    client.socket_fd = sockfd;
    client.connected = 1;
    client.my_player_id = 1;

    // receive and decode game state
    printf("Waiting for game state from server...\n");
    handle_server_communication(&client);

    printf("Received game state:\n");
    printf("  playerCount  : %d\n", client.game.playerCount);
    printf("  pot          : %u\n", client.game.pot);
    printf("  currentBet   : %u\n", client.game.currentBet);
    printf("  stage        : %d\n", client.game.stage);
    printf("  communityCount: %d\n", client.game.communityCount);
    for (int i = 0; i < client.game.communityCount; i++) {
        printf("  community[%d] : rank=%d suit=%d\n", i,
               client.game.community[i].rank,
               client.game.community[i].suit);
    }
    printf("  players:\n");
    for (int i = 0; i < client.game.playerCount; i++) {
        printf("    [%d] %s chips=%u hand=(%d,%d)/(%d,%d)\n",
               client.game.players[i].id,
               client.game.players[i].name,
               client.game.players[i].chips,
               client.game.players[i].hand[0].rank,
               client.game.players[i].hand[0].suit,
               client.game.players[i].hand[1].rank,
               client.game.players[i].hand[1].suit);
    }

    // send a player action back
    PlayerAction action;
    action.playerID = client.my_player_id;
    action.move = CALL;
    action.amount = 20;

    uint8_t buffer[BUFFER_SIZE];
    uint32_t len = prepare_payload(buffer, MSG_TYPE_PLAYER_ACTION, &action);
    send_to_server(&client, buffer, len);
    printf("Sent action: CALL amount=20\n");

    close(client.socket_fd);
    return 0;
}

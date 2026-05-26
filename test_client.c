/*  test_client.c
    Test client for verifying the encode/decode communication layer.
    Connects to test_server and offers three modes:
      1. Auto   — cycles through all move types automatically, one per server broadcast
      2. Manual — user types a move after each server broadcast
      3. Exit   — closes the connection and quits

    Both modes use select() to watch the socket and stdin simultaneously,
    so server data and user input never block each other.
*/
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>
#include "protocol.h"
#include "com.h"

// print the relevant fields of the received game state
static void print_game_state(const ClientState *client)
{
    printf("\n--- Game State ---\n");
    printf("  pot=%u  currentBet=%u  stage=%d  currentPlayer=%d\n",
           client->game.pot, client->game.currentBet,
           client->game.stage, client->game.currentPlayer);
    for (int i = 0; i < client->game.playerCount; i++) {
        // opponent hands will show UNKNOW_R/UNKNOW_S due to hide_card_info_for_others()
        printf("  [%d] %s chips=%u hand=(%d/%d),(%d/%d)\n",
               client->game.players[i].id,
               client->game.players[i].name,
               client->game.players[i].chips,
               client->game.players[i].hand[0].rank,
               client->game.players[i].hand[0].suit,
               client->game.players[i].hand[1].rank,
               client->game.players[i].hand[1].suit);
    }
}

// encode and send a PlayerAction to the server
static void send_action(ClientState *client, MoveType move, uint32_t amount)
{
    PlayerAction action;
    action.playerID = client->my_player_id;
    action.move = move;
    action.amount = amount;

    uint8_t buffer[BUFFER_SIZE];
    uint32_t len = prepare_payload(buffer, MSG_TYPE_PLAYER_ACTION, &action);
    send_to_server(client, buffer, len);
    printf("Sent: move=%d amount=%u\n", move, amount);
}

/*  Auto mode: cycles FOLD->CHECK->CALL->RAISE->ALL_IN repeatedly.
    select() watches both the socket (server broadcast) and stdin (exit signal).
    On each server broadcast: print state, send next move in cycle.
    Type 3 + Enter at any time to return to the main menu.
*/
static void run_auto_mode(ClientState *client)
{
    MoveType cycle[] = { FOLD, CHECK, CALL, RAISE, ALL_IN };
    int cycle_len = 5;
    int idx = 0;

    printf("Auto mode started. Type 3 + Enter to exit.\n");

    while (client->connected) {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(client->socket_fd, &read_fds);
        FD_SET(STDIN_FILENO, &read_fds);
        int max_fd = client->socket_fd > STDIN_FILENO ? client->socket_fd : STDIN_FILENO;

        int activity = select(max_fd + 1, &read_fds, NULL, NULL, NULL);
        if (activity < 0) { perror("select error"); break; }

        // user typed something — check if they want to exit
        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            int choice;
            if (scanf("%d", &choice) != 1 || choice == 3) {
                printf("Exiting auto mode.\n");
                return;
            }
        }

        // server sent a new game state — print it and send next move
        if (FD_ISSET(client->socket_fd, &read_fds)) {
            handle_server_communication(client);
            if (!client->connected) break;
            print_game_state(client);

            MoveType move = cycle[idx % cycle_len];
            uint32_t amount = (move == RAISE) ? client->game.minRaise : 0;
            send_action(client, move, amount);
            idx++;
            printf("(Type 3 + Enter to exit auto mode)\n");
        }
    }
}

/*  Manual mode: user picks a move after each server broadcast.
    select() watches both the socket and stdin so neither blocks the other.
    Move options shown after each broadcast; type 5 to return to main menu.
*/
static void run_manual_mode(ClientState *client)
{
    printf("Manual mode started.\n");

    while (client->connected) {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(client->socket_fd, &read_fds);
        FD_SET(STDIN_FILENO, &read_fds);
        int max_fd = client->socket_fd > STDIN_FILENO ? client->socket_fd : STDIN_FILENO;

        int activity = select(max_fd + 1, &read_fds, NULL, NULL, NULL);
        if (activity < 0) { perror("select error"); break; }

        // new game state arrived — print it and prompt for a move
        if (FD_ISSET(client->socket_fd, &read_fds)) {
            handle_server_communication(client);
            if (!client->connected) break;
            print_game_state(client);
            printf("Move (0=FOLD 1=CHECK 2=CALL 3=RAISE 4=ALL_IN 5=Exit): ");
            fflush(stdout);
        }

        // user entered a move choice
        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            int choice;
            if (scanf("%d", &choice) != 1 || choice == 5) {
                printf("Exiting manual mode.\n");
                return;
            }
            if (choice < 0 || choice > 4) {
                printf("Invalid choice.\n");
                continue;
            }
            uint32_t amount = 0;
            if (choice == RAISE) {
                printf("Enter raise amount: ");
                fflush(stdout);
                if (scanf("%u", &amount) != 1) amount = 0;
            }
            send_action(client, (MoveType)choice, amount);
        }
    }
}

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
    client.my_player_id = 0;
    printf("Connected to server.\n");

    // main menu — stays up between mode sessions
    while (client.connected) {
        printf("\n1. Auto mode\n2. Manual mode\n3. Exit\nChoice: ");
        fflush(stdout);

        int choice;
        if (scanf("%d", &choice) != 1) break;

        if (choice == 1)      run_auto_mode(&client);
        else if (choice == 2) run_manual_mode(&client);
        else if (choice == 3) break;
        else printf("Invalid choice.\n");
    }

    close(client.socket_fd);
    printf("Disconnected.\n");
    return 0;
}

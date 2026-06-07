/*  tests/test_protocol.c
    Unit tests for the binary encode/decode protocol (com.c).
    Verifies every message type round-trips correctly.
    Build: make tests/test_protocol
    Run:   ./tests/test_protocol
*/
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "com.h"
#include "uds.h"

#define BUFFER_SIZE (MAX_PAYLOAD_SIZE + PROTOCOL_HEADER_SIZE)

static int passed = 0, failed = 0;
#define TEST(name, expr) do { \
    if (expr) { printf("  PASS  %s\n", name); passed++; } \
    else       { printf("  FAIL  %s  (line %d)\n", name, __LINE__); failed++; } \
} while(0)

static Card make_card(Rank r, Suit s) { Card c; c.rank = r; c.suit = s; c.value = r; return c; }

/* ================================================================
   1. GAME STATE ROUND-TRIP
   ================================================================ */
static void test_gamestate_roundtrip(void)
{
    printf("\n[MSG_TYPE_GAME_STATE round-trip]\n");

    /* build a non-trivial game state */
    GameState orig;
    memset(&orig, 0, sizeof(orig));
    orig.pot         = 42;
    orig.currentBet  = 20;
    orig.minRaise    = 10;
    orig.stage       = FLOP;
    orig.currentPlayer = 2;
    orig.dealerIndex   = 1;
    orig.yourPlayerID  = 3;
    orig.handPlaying   = true;
    orig.gameOver      = 0;
    orig.winnerID      = 255;
    orig.communityCount = 3;
    orig.community[0] = make_card(ACE,   HEARTS);
    orig.community[1] = make_card(KING,  CLUBS);
    orig.community[2] = make_card(QUEEN, DIAMONDS);

    for (int i = 0; i < MAX_PLAYERS; i++) {
        orig.players[i].id     = i;
        orig.players[i].chips  = 100 + i * 50;
        orig.players[i].status = (i == 0) ? PLAYER_FOLDED : PLAYER_PLAYING;
        orig.players[i].has_cards = 1;
        orig.players[i].hand[0] = make_card(TWO + i, HEARTS);
        orig.players[i].hand[1] = make_card(THREE + i, CLUBS);
        snprintf(orig.players[i].name, MAX_NAME_LENTH, "P%d", i);
    }

    /* encode */
    Message msg_out;
    msg_out.type      = MSG_TYPE_GAME_STATE;
    msg_out.gameState = orig;

    uint8_t buf[BUFFER_SIZE];
    uint32_t len = prepare_payload(buf, MSG_TYPE_GAME_STATE, &msg_out);
    TEST("prepare_payload returns nonzero length", len > 0);

    /* decode */
    Message msg_in;
    int rc = receive_payload(buf, len, &msg_in);
    TEST("receive_payload returns 0",        rc == 0);
    TEST("type decoded correctly",           msg_in.type == MSG_TYPE_GAME_STATE);

    GameState *dec = &msg_in.gameState;
    TEST("pot preserved",                    dec->pot          == orig.pot);
    TEST("currentBet preserved",             dec->currentBet   == orig.currentBet);
    TEST("stage preserved",                  dec->stage        == orig.stage);
    TEST("currentPlayer preserved",          dec->currentPlayer == orig.currentPlayer);
    TEST("yourPlayerID preserved",           dec->yourPlayerID == orig.yourPlayerID);
    TEST("handPlaying preserved",            dec->handPlaying  == orig.handPlaying);
    TEST("communityCount preserved",         dec->communityCount == orig.communityCount);
    TEST("community[0] rank preserved",      dec->community[0].rank == ACE);
    TEST("community[0] suit preserved",      dec->community[0].suit == HEARTS);

    for (int i = 0; i < MAX_PLAYERS; i++) {
        TEST("player chips preserved",       dec->players[i].chips  == orig.players[i].chips);
        TEST("player status preserved",      dec->players[i].status == orig.players[i].status);
        TEST("player name preserved",        strcmp(dec->players[i].name, orig.players[i].name) == 0);
    }
}

/* ================================================================
   2. PLAYER ACTION ROUND-TRIP
   ================================================================ */
static void test_action_roundtrip(void)
{
    printf("\n[MSG_TYPE_PLAYER_ACTION round-trip]\n");

    MoveType moves[] = { FOLD, CHECK, CALL, RAISE, ALL_IN };
    const char *names[] = { "FOLD", "CHECK", "CALL", "RAISE", "ALL_IN" };

    for (int i = 0; i < 5; i++) {
        Message out;
        out.type             = MSG_TYPE_PLAYER_ACTION;
        out.action.playerID  = i;
        out.action.move      = moves[i];
        out.action.amount    = (moves[i] == RAISE) ? 150 : 0;
        out.action.target    = 0;
        out.action.useSpecialCard = 0;

        uint8_t buf[BUFFER_SIZE];
        uint32_t len = prepare_payload(buf, MSG_TYPE_PLAYER_ACTION, &out);

        Message in;
        int rc = receive_payload(buf, len, &in);
        char label[64];
        snprintf(label, sizeof(label), "action %s round-trip", names[i]);
        TEST(label, rc == 0 &&
             in.action.playerID == out.action.playerID &&
             in.action.move     == out.action.move &&
             in.action.amount   == out.action.amount);
    }
}

/* ================================================================
   3. CHAT MESSAGE ROUND-TRIP
   ================================================================ */
static void test_chat_roundtrip(void)
{
    printf("\n[MSG_TYPE_CHAT_MESSAGE round-trip]\n");

    const char *texts[] = { "hello", "raise the stakes!", "", "x" };
    for (int i = 0; i < 4; i++) {
        Message out;
        out.type      = MSG_TYPE_CHAT_MESSAGE;
        out.sender_id = i;
        strncpy(out.chat, texts[i], MAX_PAYLOAD_SIZE - 1);

        uint8_t buf[BUFFER_SIZE];
        uint32_t len = prepare_payload(buf, MSG_TYPE_CHAT_MESSAGE, &out);

        Message in;
        int rc = receive_payload(buf, len, &in);
        char label[64];
        snprintf(label, sizeof(label), "chat \"%s\" round-trip", texts[i]);
        TEST(label, rc == 0 &&
             in.sender_id == out.sender_id &&
             strcmp(in.chat, out.chat) == 0);
    }
}

/* ================================================================
   4. ERROR MESSAGE ROUND-TRIP
   ================================================================ */
static void test_error_roundtrip(void)
{
    printf("\n[MSG_TYPE_ERROR_MESSAGE round-trip]\n");

    Message out;
    out.type = MSG_TYPE_ERROR_MESSAGE;
    strncpy(out.error, "Invalid move", MAX_PAYLOAD_SIZE - 1);

    uint8_t buf[BUFFER_SIZE];
    uint32_t len = prepare_payload(buf, MSG_TYPE_ERROR_MESSAGE, &out);

    Message in;
    int rc = receive_payload(buf, len, &in);
    TEST("error message decoded", rc == 0 && strcmp(in.error, "Invalid move") == 0);
}

/* ================================================================
   5. READY MESSAGE ROUND-TRIP
   ================================================================ */
static void test_ready_roundtrip(void)
{
    printf("\n[MSG_TYPE_READY round-trip]\n");

    Message out;
    out.type = MSG_TYPE_READY;

    uint8_t buf[BUFFER_SIZE];
    uint32_t len = prepare_payload(buf, MSG_TYPE_READY, &out);
    TEST("ready payload has header-only length", len == PROTOCOL_HEADER_SIZE);

    Message in;
    int rc = receive_payload(buf, len, &in);
    TEST("ready decoded ok", rc == 0 && in.type == MSG_TYPE_READY);
}

/* ================================================================
   6. CD SIGNAL ROUND-TRIP
   ================================================================ */
static void test_cd_signal_roundtrip(void)
{
    printf("\n[MSG_CD_SIGNAL round-trip]\n");

    for (uint8_t target = 0; target < MAX_PLAYERS; target++) {
        Message out;
        out.type      = MSG_CD_SIGNAL;
        out.sender_id = target;

        uint8_t buf[BUFFER_SIZE];
        uint32_t len = prepare_payload(buf, MSG_CD_SIGNAL, &out);

        Message in;
        int rc = receive_payload(buf, len, &in);
        char label[32];
        snprintf(label, sizeof(label), "CD signal target=%d", target);
        TEST(label, rc == 0 && in.sender_id == target);
    }
}

/* ================================================================
   7. JOIN MESSAGE ROUND-TRIP
   ================================================================ */
static void test_join_roundtrip(void)
{
    printf("\n[MSG_TYPE_JOIN round-trip]\n");

    const char *names[] = { "Alice", "Bob", "Duc", "" };
    for (int i = 0; i < 4; i++) {
        Message out;
        out.type      = MSG_TYPE_JOIN;
        out.sender_id = i;
        strncpy(out.chat, names[i], MAX_PAYLOAD_SIZE - 1);

        uint8_t buf[BUFFER_SIZE];
        uint32_t len = prepare_payload(buf, MSG_TYPE_JOIN, &out);

        Message in;
        int rc = receive_payload(buf, len, &in);
        char label[64];
        snprintf(label, sizeof(label), "join name \"%s\" round-trip", names[i]);
        TEST(label, rc == 0 &&
             in.sender_id == out.sender_id &&
             strcmp(in.chat, out.chat) == 0);
    }
}

/* ================================================================
   8. TRUNCATED BUFFER REJECTION
   ================================================================ */
static void test_bad_input(void)
{
    printf("\n[malformed input rejection]\n");

    /* empty buffer */
    Message in;
    TEST("empty buffer rejected",       receive_payload(NULL, 0,   &in) != 0);

    /* too-short header */
    uint8_t short_buf[3] = {0, 1, 0};
    TEST("short header rejected",       receive_payload(short_buf, 3, &in) != 0);

    /* correct header but truncated payload */
    Message out;
    out.type = MSG_TYPE_CHAT_MESSAGE;
    out.sender_id = 0;
    strncpy(out.chat, "hello", MAX_PAYLOAD_SIZE - 1);
    uint8_t buf[BUFFER_SIZE];
    uint32_t full_len = prepare_payload(buf, MSG_TYPE_CHAT_MESSAGE, &out);
    TEST("truncated payload rejected",  receive_payload(buf, full_len - 1, &in) != 0);
}

/* ================================================================
   MAIN
   ================================================================ */
int main(void)
{
    printf("=== Anteater Poker — Unit Tests (protocol) ===\n");
    test_gamestate_roundtrip();
    test_action_roundtrip();
    test_chat_roundtrip();
    test_error_roundtrip();
    test_ready_roundtrip();
    test_cd_signal_roundtrip();
    test_join_roundtrip();
    test_bad_input();

    printf("\n===============================================\n");
    printf("Results: %d passed, %d failed\n", passed, failed);
    return failed > 0 ? 1 : 0;
}

/*Encoding/decoding protocol for communication between server and clients.
Message format:
[ type ][ length ][ payload ]
uint16_t type      // 2 bytes
uint32_t length    // 4 bytes
payload            // N bytes

*/
#ifndef COM_H
#define COM_H
#include "uds.h"
#include<stdint.h>

typedef enum {
    MSG_TYPE_GAME_STATE = 1,
    MSG_TYPE_PLAYER_ACTION = 2,
    MSG_TYPE_CHAT_MESSAGE = 3,
    MSG_TYPE_ERROR_MESSAGE = 4,
    MSG_TYPE_SPECIAL_MESSAGE = 5,
    MSG_TYPE_READY = 6,
    MSG_CD_SIGNAL= 7,// for sending count down signal
    MSG_TYPE_JOIN = 8
} MessageType;
typedef struct {
    MessageType type;
    uint8_t sender_id;
    union {
        GameState gameState;
        PlayerAction action;
        char chat[MAX_PAYLOAD_SIZE];
        char special[MAX_PAYLOAD_SIZE];
        char error[MAX_PAYLOAD_SIZE];
    };
} Message; // for handling multi datatypes incomming payload.
void write_u8(uint8_t *buffer, uint32_t *offset,uint8_t val);
void write_u32(uint8_t *buffer, uint32_t *offset,uint32_t val);
uint8_t read_u8(const uint8_t *buffer, uint32_t *offset,uint8_t *val);
uint32_t read_u32(const uint8_t *buffer, uint32_t *offset,uint32_t *val);
uint32_t encode_client_data(uint8_t *buffer,uint32_t *offset ,const PlayerAction *action);
int decode_client_data(const uint8_t *buffer, uint32_t *offset, PlayerAction *action);
uint32_t encode_player_data(uint8_t *buffer,uint32_t *offset ,const Player *player);
int decode_player_data(const uint8_t *buffer, uint32_t *offset, Player *player);
uint32_t encode_server_data(uint8_t *buffer, uint32_t *offset, const GameState *gameState);
int decode_server_data(const uint8_t *buffer, uint32_t *offset, GameState *gameState);
uint32_t encode_card(uint8_t *buffer, uint32_t *offset, const Card *card);
int decode_card(const uint8_t *buffer, uint32_t *offset, Card *card);
uint32_t encode_chat_message(uint8_t *buffer, uint32_t *offset, uint8_t sender_id, const char *message);
int decode_chat_message(const uint8_t *buffer, uint32_t *offset, uint8_t *sender_id, char *message);
uint32_t encode_special_message(uint8_t *buffer, uint32_t *offset, const char *message);
int decode_special_message(const uint8_t *buffer, uint32_t *offset, char *message);
uint32_t encode_error_message(uint8_t *buffer, uint32_t *offset, const char *message);
int decode_error_message(const uint8_t *buffer, uint32_t *offset, char *message);

// builds [type(2)][length(4)][payload] into buffer, returns total bytes written
uint32_t prepare_payload(uint8_t *buffer, MessageType type, const Message*data);

// parses header, dispatches decode into out_data, returns 0 on success
int receive_payload(const uint8_t *buffer, uint32_t buf_len, Message *out_data);


#endif


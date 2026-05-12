/*Encoding/decoding protocol for communication between server and clients.
Message format:
[ type ][ length ][ payload ]
uint16_t type      // 2 bytes
uint32_t length    // 4 bytes
payload            // N bytes

*/
#ifndef COM_H
#define COM_H
#include "protocol.h"
#include "uds.h"
#include<stdint.h>
#define PROTOCOL_HEADER SIZE 6// 2 bytes for type and 4 bytes for length
#define MAX_PAYLOAD_SIZE 4096 //4096 bytes
typedef enum {
    MSG_TYPE_GAME_STATE = 1,
    MSG_TYPE_PLAYER_ACTION = 2,
    MSG_TYPE_CHAT_MESSAGE = 3,
    MSG_TYPE_ERROR_MESSAGE = 4
} MessageType;
void write_u8(uint8_t *buffer, uint32_t *offset,uint8_t val);
void write_u32(uint8_t *buffer, uint32_t *offset,uint32_t val);
uint8_t read_u8(const uint8_t *buffer, uint32_t *offset,uint8_t *val);
uint32_t read_u32(const uint8_t *buffer, uint32_t *offset,uint32_t *val);
uint32_t encode_client_data(uint8_t *buffer,uint32_t *offset ,const PlayerAction *action);
int decode_client_data(const uint8_t *buffer, uint32_t *offset, PlayerAction *action);
uint32_t encode_player_data(uint8_t *buffer,uint32_t *offset ,const Player *player);
int decode_player_data(const uint8_t *buffer, uint32_t *offset, Player *player);
uint32_t encode_server_data(uint8_t *buffer, const GameState *gameState);
int decode_server_data(const uint8_t *buffer, uint32_t *offset, GameState *gameState);

#endif
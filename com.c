#include "com.h"
#include <string.h>

void write_u8(uint8_t *buffer, uint32_t *offset,uint8_t val)
{
    buffer[*offset] = val;
    /*
    the way buffer[*offset] work is that
    suppose the offset is 0, and buffer is 8 bytes.
    then in the next 8 bytes starting from buffer[0],
    it will write each byte of value to the buffer[i]
    */
    
    (*offset)++;
    //the offset will be increased by 1 so that the next time it being called.
    //the "cursor" will move to the next byte in the buffer.

}
void write_u32(uint8_t *buffer, uint32_t *offset, uint32_t val)
{
    buffer[*offset + 0] = (val >> 24) & 0xFF;
    buffer[*offset + 1] = (val >> 16) & 0xFF;
    buffer[*offset + 2] = (val >> 8)  & 0xFF;
    buffer[*offset + 3] = val & 0xFF;
    //(val >> 24) & 0xFF; will write the byte of val in big Edian to buffer
    //suppose 0x12345678
    //then buffer[0] = 0x12
    //buffer[1] = 0x34
    //buffer[2] = 0x56
    //buffer[3] = 0x78
    
    *offset += 4;
}
uint8_t read_u8(const uint8_t *buffer, uint32_t *offset, uint8_t *val)
{
    *val = buffer[*offset];
    (*offset)++;
    return 0;
}
//now, both read (decode)function only need to do the opposite of write (encode) function, 
//which is to read the data from buffer and move the offset accordingly.
uint32_t read_u32(const uint8_t *buffer, uint32_t *offset, uint32_t *val)
{
    uint32_t temp1 = buffer[*offset + 0]<<24;
    uint32_t temp2 = buffer[*offset + 1]<<16;
    uint32_t temp3 = buffer[*offset + 2]<<8;
    uint32_t temp4 = buffer[*offset + 3];
    *val = temp1 | temp2 | temp3 | temp4;

    *offset += 4;
    return 0;
}
uint32_t encode_client_data(uint8_t *buffer, uint32_t *offset, const PlayerAction *action)
{
    write_u8(buffer, offset, action->playerID);
    write_u8(buffer, offset, action->move);
    write_u32(buffer, offset, action->amount);
    write_u8(buffer, offset, action->target);
    write_u8(buffer, offset, action->useSpecialCard);

    return *offset;
}
int decode_client_data(const uint8_t *buffer, uint32_t *offset, PlayerAction *action)
{
   
    read_u8(buffer, offset, &action->playerID);
    uint8_t move_byte;
    read_u8(buffer, offset, &move_byte);
    action->move = move_byte;
    read_u32(buffer, offset, &action->amount);
    read_u8(buffer, offset, &action->target);
    //useSpecialCard is a 4-byte enum but only 1 byte is on the wire; read into a
    //byte and assign so the upper 3 bytes are zeroed (a raw (uint8_t*) cast would
    //leave them as stack garbage, corrupting the enum -> buyPowerup default reject)
    uint8_t special_byte;
    read_u8(buffer, offset, &special_byte);
    action->useSpecialCard = special_byte;
    /*if (action->playerID<0 ||action->move <0 || action->amount <0) {
        return -1; // Invalid data
    }*/
    return 0;
}
uint32_t encode_player_data(uint8_t *buffer, uint32_t *offset, const Player *player)
{
    write_u8(buffer, offset, player->id);//write id
    
    for (int i = 0; i < MAX_NAME_LENTH; i++) 
        write_u8(buffer, offset, player->name[i]);//write name
    for (int i = 0; i<2;i++)
        encode_card(buffer, offset, &player->hand[i]);//write hand cards
    
    
    write_u8(buffer, offset, player->status);//write status
    write_u8(buffer, offset, player->has_cards);//write has_cards
    write_u32(buffer, offset, player->chips);//write chips
    write_u32(buffer, offset, player->current_bet);//write current_bet
    write_u32(buffer, offset, player->total_bet);
    write_u8(buffer, offset, player->place);//write place
    return *offset;
}
int decode_player_data(const uint8_t *buffer, uint32_t *offset, Player *player)
{
    read_u8(buffer, offset, &player->id);
    for (int i = 0; i < MAX_NAME_LENTH; i++) 
        read_u8(buffer, offset, (uint8_t *)&player->name[i]);
    for (int i = 0; i<2;i++)
        decode_card(buffer, offset, &player->hand[i]);
    
    read_u8(buffer, offset, &player->status);
    read_u8(buffer, offset, &player->has_cards);
    read_u32(buffer, offset, &player->chips);
    read_u32(buffer, offset, &player->current_bet);
    read_u32(buffer, offset, &player->total_bet);
    read_u8(buffer, offset, &player->place);
 
    
    /*if (player->id < 0 || player->status < 0 || player->has_cards < 0 || player->chips < 0 || player->current_bet < 0) {
        return -1; // Invalid data
    }*/
    return 0;
}
uint32_t encode_card(uint8_t *buffer, uint32_t *offset, const Card *card)
{
    write_u8(buffer, offset, card->rank);
    write_u8(buffer, offset, card->suit);
    return *offset;
}
int decode_card(const uint8_t *buffer, uint32_t *offset, Card *card)
{
    read_u8(buffer, offset, &card->rank);
    read_u8(buffer, offset, &card->suit);
    
    return 0;
}

uint32_t encode_server_data(uint8_t *buffer, uint32_t *offset, const GameState *gameState)
{
    for (int i = 0; i < MAX_PLAYERS; i++) {
        encode_player_data(buffer, offset, &gameState->players[i]);
    }
    write_u8(buffer, offset, gameState->playerCount);
    write_u8(buffer, offset, gameState->communityCount);
    for(int i=0; i<gameState->communityCount;i++)
    {
   //     if(&gameState->community[i] != NULL)
            encode_card(buffer, offset, &gameState->community[i]);
    }
    for (int i=0;i<6;i++)
    {
        write_u8(buffer, offset, gameState->cardPrice[i]);
    }
    write_u8(buffer, offset, gameState->stage);
    write_u8(buffer, offset, gameState->currentPlayer);
    write_u8(buffer, offset, gameState->dealerIndex);
    write_u8(buffer, offset, gameState->yourPlayerID);
    write_u32(buffer, offset, gameState->pot);
    write_u32(buffer, offset, gameState->currentBet);
    write_u32(buffer, offset, gameState->minRaise);
    write_u8(buffer, offset, gameState->handPlaying);
    write_u8(buffer, offset, gameState->gameOver);
    write_u8(buffer, offset, gameState->winnerID);
    write_u8(buffer, offset, gameState->lastActor);
    write_u8(buffer, offset, gameState->smallBlindIndex);
    write_u8(buffer, offset, gameState->bigBlindIndex);
    write_u32(buffer, offset, gameState->smallBlind);
    write_u32(buffer, offset, gameState->bigBlind);


    
    return *offset;
}
int decode_server_data(const uint8_t *buffer, uint32_t *offset, GameState *gameState)
{
    for (int i = 0; i < MAX_PLAYERS; i++) {
        decode_player_data(buffer, offset, &gameState->players[i]);
    }
    read_u8(buffer, offset, &gameState->playerCount);
     read_u8(buffer, offset, &gameState->communityCount);
    for(int i=0; i<gameState->communityCount;i++)
    {
        decode_card(buffer, offset, &gameState->community[i]);
    }
    for (int i=0;i<6;i++)
    {
        //cardPrice is a 4-byte enum; read 1 wire byte and assign (zeroes upper bytes)
        uint8_t price_byte;
        read_u8(buffer, offset, &price_byte);
        gameState->cardPrice[i] = price_byte;
    }
     read_u8(buffer, offset, &gameState->stage);
     read_u8(buffer, offset, &gameState->currentPlayer);
     read_u8(buffer, offset, &gameState->dealerIndex);
     read_u8(buffer, offset, &gameState->yourPlayerID);
    read_u32(buffer, offset, &gameState->pot);
    read_u32(buffer, offset, &gameState->currentBet);
    read_u32(buffer, offset, &gameState->minRaise);
    read_u8(buffer, offset, (uint8_t *)&gameState->handPlaying);
    read_u8(buffer, offset, &gameState->gameOver);
    read_u8(buffer, offset, &gameState->winnerID);
    read_u8(buffer, offset, &gameState->lastActor);
    read_u8(buffer, offset, &gameState->smallBlindIndex);
    read_u8(buffer, offset, &gameState->bigBlindIndex);
    read_u32(buffer, offset, &gameState->smallBlind);
    read_u32(buffer, offset, &gameState->bigBlind);
    /*if (gameState->playerCount < 0 || gameState->communityCount < 0 || gameState->stage < 0 || gameState->currentPlayer < 0 || gameState->dealerIndex < 0 || gameState->pot < 0 || gameState->currentBet < 0 || gameState->minRaise < 0 || gameState->handPlaying < 0) {
        return -1; // Invalid data
    }*/
    return 0;
}
uint32_t encode_chat_message(uint8_t *buffer, uint32_t *offset, uint8_t sender_id,const char *message)
{
    write_u8(buffer,offset,sender_id);
    uint32_t length = strlen(message);
    write_u32(buffer, offset, length);
    for (uint32_t i = 0; i < length; i++) {
        write_u8(buffer, offset, message[i]);
    }
    return *offset;
}
int decode_chat_message(const uint8_t *buffer, uint32_t *offset, uint8_t *sender_id, char *message)
{
    read_u8(buffer, offset, sender_id);
    uint32_t length;
    read_u32(buffer, offset, &length);
    for (uint32_t i = 0; i < length; i++) {
        read_u8(buffer, offset, (uint8_t *)&message[i]);
    }
    message[length] = '\0'; // Null-terminate the string
    return 0;
}
uint32_t encode_error_message(uint8_t *buffer, uint32_t *offset, const char *message)
{
    uint32_t length = strlen(message);
    write_u32(buffer, offset, length);
    for (uint32_t i = 0; i < length; i++) {
        write_u8(buffer, offset, message[i]);
    }
    return *offset;
}
int decode_error_message(const uint8_t *buffer, uint32_t *offset, char *message)
{    uint32_t length;
    read_u32(buffer, offset, &length);
    for (uint32_t i = 0; i < length; i++) {
        read_u8(buffer, offset, (uint8_t *)&message[i]);
    }
    message[length] = '\0'; // Null-terminate the string
    return 0;
}

uint32_t prepare_payload(uint8_t *buffer, MessageType type, const Message*data)
{
    uint32_t offset=0;
    buffer[offset++] = (type >> 8) & 0xFF; // Write the high byte of the message type
    buffer[offset++] = type & 0xFF; // Write the low byte of the message type
    uint32_t length_offset = offset; // Store the offset for the length field
    offset += 4; // Reserve space for the length field, will fill it later

    uint32_t payload_start = offset;
    switch(type)
    {
        case MSG_TYPE_GAME_STATE:
            encode_server_data(buffer, &offset, &data->gameState);
                break;
        case MSG_TYPE_PLAYER_ACTION:
            encode_client_data(buffer, &offset, &data->action);
            break;
        case MSG_TYPE_READY:
            break;
        case MSG_TYPE_CHAT_MESSAGE:
             encode_chat_message(buffer, &offset, data->sender_id, data->chat);
            break;
        case MSG_TYPE_JOIN:
            encode_chat_message(buffer, &offset, data->sender_id, data->chat);
            break;
        case MSG_TYPE_ERROR_MESSAGE:
            encode_error_message(buffer, &offset, data->error);
            break;
        case MSG_CD_SIGNAL:
           {
            write_u8(buffer, &offset, data->sender_id); // sender_id = target.
            break;
           }
        case MSG_TYPE_SELECT_SEAT:
            write_u8(buffer, &offset, data->sender_id); // sender_id = chosen seat index
            break;
        default:
            return 0;
    }
    uint32_t payload_length = offset - payload_start; // Calculate the length of the payload
    buffer[length_offset + 0] = (payload_length >> 24) & 0xFF;
    buffer[length_offset + 1] = (payload_length >> 16) & 0xFF;
    buffer[length_offset + 2] = (payload_length >> 8) & 0xFF;
    buffer[length_offset + 3] = payload_length & 0xFF; // Write the payload length in big-endian format
    return offset; // Return the total bytes written (header + payload)
}
int receive_payload(const uint8_t *buffer, uint32_t buf_len,  Message *out_data)
{
    uint32_t offset=0;
    if(buf_len < PROTOCOL_HEADER_SIZE)
        return -1; // Not enough data for header
    uint8_t temp1, temp2;
    read_u8(buffer, &offset, &temp1);
    read_u8(buffer, &offset, &temp2);
    out_data->type = (temp1 << 8) | temp2; // Combine the two bytes to get the message type
    uint32_t length;
    read_u32(buffer, &offset, &length);
    if(buf_len < PROTOCOL_HEADER_SIZE + length)
        return -1; // Not enough data for payload
    switch(out_data->type){
    case MSG_TYPE_GAME_STATE:
        return decode_server_data(buffer, &offset, &out_data->gameState);
    case MSG_TYPE_PLAYER_ACTION:
        return decode_client_data(buffer, &offset, &out_data->action);
    case MSG_TYPE_READY:
        return 0; 
    case MSG_TYPE_CHAT_MESSAGE:
        return decode_chat_message(buffer, &offset, &out_data->sender_id, out_data->chat);
    case MSG_TYPE_JOIN:
        return decode_chat_message(buffer, &offset, &out_data->sender_id, out_data->chat);//future implementation
    case MSG_TYPE_ERROR_MESSAGE:
        return decode_error_message(buffer, &offset, out_data->error);//future implementation
    case MSG_CD_SIGNAL:
        {
            return read_u8(buffer, &offset, &out_data->sender_id); // sender_id = target.
        }
    case MSG_TYPE_SELECT_SEAT:
        return read_u8(buffer, &offset, &out_data->sender_id); // sender_id = chosen seat index
    default:
        return -1; // Unknown message type

    }
}

#include "com.h"

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
static void write_u32(uint8_t *buffer, uint32_t *offset, uint32_t val)
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
int read_u8(const uint8_t *buffer, uint32_t *offset)
{
    uint8_t val = buffer[*offset];
    (*offset)++;
    return val;
}
//now, both read (decode)function only need to do the opposite of write (encode) function, 
//which is to read the data from buffer and move the offset accordingly.
int read_u32(const uint8_t *buffer, uint32_t *offset)
{
    uint32_t temp1 = buffer[*offset + 0]<<24;
    uint32_t temp2 = buffer[*offset + 1]<<16;
    uint32_t temp3 = buffer[*offset + 2]<<8;
    uint32_t temp4 = buffer[*offset + 3];
    uint32_t val = temp1 | temp2 | temp3 | temp4;

    *offset += 4;
    return val;
}
uint32_t encode_client_data(uint8_t *buffer, const PlayerAction *action)
{
    uint32_t offset = 0;
    write_u8(buffer, &offset, action->playerID);
    write_u8(buffer, &offset, action->move);
    write_u32(buffer, &offset, action->amount);
    return offset;
}
int decode_client_data(const uint8_t *buffer, uint32_t length, PlayerAction *action)
{
    if (length < 6) {
        return -1; // Not enough data
    }
    uint32_t offset = 0;
    action->playerID = read_u8(buffer, &offset);
    action->move = read_u8(buffer, &offset);
    action->amount = read_u32(buffer, &offset);
    if (action->playerID<0 ||action->move <0 || action->amount <0) {
        return -1; // Invalid data
    }
    return 0;
}
uint32_t encode_player_data(uint8_t *buffer, const Player *player)
{
    
}
int decode_player_data(const uint8_t *buffer, uint32_t length, Player *player)
{
}
uint32_t encode_server_data(uint8_t *buffer, const GameState *gameState)
{

}
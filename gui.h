// Declaration of all the different hands in poker
#ifndef GUI_H
#define GUI_H

#include <string.h>
#include <sys/select.h>
#include "protocol.h"
#include "gui_assets.h"

//connect the send chat button and decide what to do
void on_send_chat_button_clicked(GtkEntry *entry, gpointer data);

//call within on_send
//prepare message payload and send to server
void sendChatToServer(uint8_t sender_id, const char *chatMessage);

//in the chat_log, append the chat message
//to be called after server broadcasts new message to all clients
void appendChat(const char *sender, const char *chatMessage);

//call if user types /ready in chat
void sendReadyToServer();


#endif
// Declaration of all the different hands in poker
#ifndef GUI_H
#define GUI_H

#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include "protocol.h"


//connect the send chat button and decide what to do
void on_send(GtkEntry *entry, gpointer data);

//call within on_send
//create a message payload and send to server
void sendChatToServer(uint8_t sender_id, const char *chatMessage);

//in the chat_log, append the chat message
//to be called after server broadcasts new message to all clients
void appendChat(const char *sender, const char *chatMessage);


#endif
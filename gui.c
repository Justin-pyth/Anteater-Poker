#include "gui.h"
#include "gui_helpers.h"
#include <stdio.h>
#include <string.h>

/* -- Globals --------------------------------------------------------------- */
AppWidgets W;
ClientState C;

/* -- Chat display ---------------------------------------------------------- */
void append_chat(const char *sender, const char *msg)
{
    if (!W.chat_log) return;
    GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(W.chat_log));
    GtkTextIter end;
    gtk_text_buffer_get_end_iter(buf, &end);
    char line[512];
    snprintf(line, sizeof(line), "%s: %s\n", sender ? sender : "?", msg ? msg : "");
    gtk_text_buffer_insert(buf, &end, line, -1);
    gtk_text_buffer_get_end_iter(buf, &end);
    GtkTextMark *mark = gtk_text_buffer_get_mark(buf, "insert");
    gtk_text_buffer_move_mark(buf, mark, &end);
    gtk_text_view_scroll_mark_onscreen(GTK_TEXT_VIEW(W.chat_log), mark);
}

/* -- Anteater deck --------------------------------------------------------- */
void set_anteater_count(int count)
{
    if (!W.anteater_count_label) return;
    char buf[64];
    snprintf(buf, sizeof(buf), "Anteater:\n%d card%s", count, count == 1 ? "" : "s");
    gtk_label_set_text(GTK_LABEL(W.anteater_count_label), buf);
}


/* -- Dispatch one parsed message ------------------------------------------- */
static void dispatch_message(Message *msg)
{
    if (msg->type == MSG_TYPE_GAME_STATE) {
        C.game = msg->gameState;
        refresh_ui();
    } else if (msg->type == MSG_TYPE_CHAT_MESSAGE) {
        if (msg->sender_id >= MAX_PLAYERS)
            append_chat("SERVER", msg->chat);
        else
            append_chat(C.game.players[msg->sender_id].name, msg->chat);
    } else if (msg->type == MSG_TYPE_ERROR_MESSAGE) {
        gtk_label_set_text(GTK_LABEL(W.log_label), msg->error);
    } else if (msg->type == MSG_CD_SIGNAL) {
        uint8_t target = msg->sender_id;
        stop_my_timer();
        for (int i = 0; i < GUI_OPPONENT_SLOTS; i++) stop_player_timer(i);
        if (target == C.my_player_id) {
            start_my_timer(TURN_SECONDS);
        } else {
            int slot = 0;
            for (int i = 0; i < MAX_PLAYERS && slot < GUI_OPPONENT_SLOTS; i++) {
                if (i == C.my_player_id) continue;
                if (C.game.players[i].status == PLAYER_EMPTY) continue;
                if (i == target) { start_player_timer(slot, TURN_SECONDS); break; }
                slot++;
            }
        }
    }
}

/* -- Server IO callback ---------------------------------------------------- */
gboolean on_server_data(GIOChannel *ch, GIOCondition cond, gpointer data)
{
    (void)ch; (void)data;
    if (cond & (G_IO_HUP | G_IO_ERR)) { C.connected = 0; return FALSE; }

    //read the available bytes and perform the message's actions one by one
    uint8_t buf[BUFFER_SIZE * 4];
    ssize_t n = recv(C.socket_fd, buf, sizeof(buf), 0);
    if (n <= 0) { C.connected = 0; return FALSE; }

    uint32_t pos = 0;
    while (pos + PROTOCOL_HEADER_SIZE <= (uint32_t)n) {
        //look at the payload length from the header (bytes 2–5)
        uint32_t plen = ((uint32_t)buf[pos+2] << 24) | ((uint32_t)buf[pos+3] << 16) |
                        ((uint32_t)buf[pos+4] << 8)  |  (uint32_t)buf[pos+5];
        uint32_t total = PROTOCOL_HEADER_SIZE + plen;
        if (pos + total > (uint32_t)n) break; //incomplete message

        Message msg;
        if (receive_payload(buf + pos, total, &msg) == 0)
            dispatch_message(&msg);
        pos += total;
    }
    return TRUE;
}

/* -- Player ID inference --------------------------------------------------- */
void infer_my_player_id(void)
{
    if (C.game.yourPlayerID < MAX_PLAYERS) {
        C.my_player_id = C.game.yourPlayerID;
        return;
    }

    for (int i = 0; i < MAX_PLAYERS; i++) {
        Player *p = &C.game.players[i];
        if (p->has_cards && card_is_known(p->hand[0])) { C.my_player_id = i; return; }
    }
}

/* -- UI refresh ------------------------------------------------------------ */
void refresh_ui(void)
{
    GameState *game = &C.game;
    char buf[128];
    infer_my_player_id();

    snprintf(buf, sizeof(buf), "Pot: $%u", game->pot);
    gtk_label_set_text(GTK_LABEL(W.pot_label), buf);

    static const char *stage_names[] = {"PREFLOP","FLOP","TURN","RIVER"};
    gtk_label_set_text(GTK_LABEL(W.stage_label),
        game->stage <= RIVER ? stage_names[game->stage] : "WAITING");

    snprintf(buf, sizeof(buf), "Players: %u", game->playerCount);
    gtk_label_set_text(GTK_LABEL(W.deck_count_label), buf);

    for (int i = 0; i < 5; i++) {
        if (i < game->communityCount) set_card_face(W.community_cards[i], game->community[i], 1);
        else                          set_card_back(W.community_cards[i]);
    }

    Player *me = &game->players[C.my_player_id];

    snprintf(buf, sizeof(buf), "$%u", game->currentBet - me->current_bet);
    gtk_label_set_text(GTK_LABEL(W.label_call_amnt), buf);

    snprintf(buf, sizeof(buf), "$%u", me->chips);
    gtk_label_set_text(GTK_LABEL(W.label_your_stack), buf);

    if (me->has_cards) {
        set_card_face(W.my_cards[0], me->hand[0], card_is_known(me->hand[0]));
        set_card_face(W.my_cards[1], me->hand[1], card_is_known(me->hand[1]));
    } else {
        set_card_back(W.my_cards[0]);
        set_card_back(W.my_cards[1]);
    }

    int opp_slot = 0;
    for (int pi = 0; pi < MAX_PLAYERS && opp_slot < GUI_OPPONENT_SLOTS; pi++) {
        if (pi == C.my_player_id) continue;
        Player *p = &game->players[pi];
        if (p->status == PLAYER_EMPTY) continue;

        gtk_label_set_text(GTK_LABEL(W.opp_name[opp_slot]),
            p->name[0] ? p->name : "Player");
        snprintf(buf, sizeof(buf), "$%u  |  bet $%u", p->chips, p->current_bet);
        gtk_label_set_text(GTK_LABEL(W.opp_chips[opp_slot]), buf);

        const char *st = "Waiting";
        if      (p->status == PLAYER_FOLDED)       st = "Folded";
        else if (p->status == PLAYER_ALL_IN)        st = "All in";
        else if (p->status == PLAYER_DISCONNECTED)  st = "Disconnected";
        else if (p->status == PLAYER_SPECTATING)    st = "Spectating";
        else if (p->status == PLAYER_PLAYING && pi == game->currentPlayer && game->handPlaying) st = "Acting";
        gtk_label_set_text(GTK_LABEL(W.opp_status[opp_slot]), st);

        if (p->has_cards) {
            if (card_is_known(p->hand[0]))
                set_card_face(W.opp_cards[opp_slot][0], p->hand[0], 1);
            else
                set_card_back(W.opp_cards[opp_slot][0]);
            if (card_is_known(p->hand[1]))
                set_card_face(W.opp_cards[opp_slot][1], p->hand[1], 1);
            else
                set_card_back(W.opp_cards[opp_slot][1]);
        } else {
            set_card_back(W.opp_cards[opp_slot][0]);
            set_card_back(W.opp_cards[opp_slot][1]);
        }

        GtkStyleContext *ctx = gtk_widget_get_style_context(W.opp_frame[opp_slot]);
        if (p->status == PLAYER_PLAYING && pi == game->currentPlayer && game->handPlaying)
            gtk_style_context_add_class(ctx, "active-seat");
        else
            gtk_style_context_remove_class(ctx, "active-seat");
        opp_slot++;
    }
    for (int i = opp_slot; i < GUI_OPPONENT_SLOTS; i++) {
        gtk_label_set_text(GTK_LABEL(W.opp_name[i]),   "Empty");
        gtk_label_set_text(GTK_LABEL(W.opp_chips[i]),  "$0  |  bet $0");
        gtk_label_set_text(GTK_LABEL(W.opp_status[i]), "Waiting");
        gtk_style_context_remove_class(
            gtk_widget_get_style_context(W.opp_frame[i]), "active-seat");
    }

    int my_turn  = C.connected && game->handPlaying && game->currentPlayer == C.my_player_id;
    int can_check = me->current_bet >= game->currentBet;

    if (C.connected && game->handPlaying)
        snprintf(buf, sizeof(buf), my_turn ? "Your turn" : "Waiting for player %u", game->currentPlayer);
    else if (C.connected)
        snprintf(buf, sizeof(buf), "Connected. Waiting for hand.");
    else
        snprintf(buf, sizeof(buf), "Not connected.");
    gtk_label_set_text(GTK_LABEL(W.log_label), buf);

    gtk_widget_set_sensitive(W.btn_fold,  my_turn);
    gtk_widget_set_sensitive(W.btn_check, my_turn && can_check);
    gtk_widget_set_sensitive(W.btn_call,  my_turn && !can_check);
    gtk_widget_set_sensitive(W.btn_raise, my_turn);
    gtk_widget_set_sensitive(W.raise_spin, my_turn);
    gtk_widget_set_sensitive(W.btn_allin, my_turn);
}

/* -- Action button callbacks ----------------------------------------------- */
void on_fold (GtkButton *b, gpointer d) { (void)b;(void)d; send_gui_move(FOLD,  0); }
void on_check(GtkButton *b, gpointer d) { (void)b;(void)d; send_gui_move(CHECK, 0); }
void on_call (GtkButton *b, gpointer d) { (void)b;(void)d; send_gui_move(CALL,  0); }
void on_allin(GtkButton *b, gpointer d) { (void)b;(void)d; send_gui_move(ALL_IN, 0); }
void on_raise(GtkButton *b, gpointer d)
{
    (void)b; (void)d;
    uint32_t amount = (uint32_t)gtk_spin_button_get_value(GTK_SPIN_BUTTON(W.raise_spin));
    send_gui_move(RAISE, amount);
}

/* -- Chat callbacks -------------------------------------------------------- */
void on_send_chat(GtkButton *b, gpointer d)
{
    (void)b; (void)d;
    const char *text = gtk_entry_get_text(GTK_ENTRY(W.chat_entry));
    if (!text || text[0] == '\0') return;
    if (strcmp(text, "/ready") == 0) {
        sendReadyToServer();
    } else {
        sendChatToServer(text);
    }
    gtk_entry_set_text(GTK_ENTRY(W.chat_entry), "");
}

void on_chat_activate(GtkEntry *e, gpointer d)
{
    (void)e; (void)d;
    gtk_button_clicked(GTK_BUTTON(W.btn_send_chat));
}

/* -- Screen transitions ---------------------------------------------------- */
void show_game_screen(void)
{
    gtk_widget_show_all(W.game_screen);
    gtk_stack_set_visible_child_name(GTK_STACK(W.stack), "game");
    gtk_window_resize(GTK_WINDOW(W.window), 900, 660);
    refresh_ui();
}
void show_login_screen(void)
{
    gtk_stack_set_visible_child_name(GTK_STACK(W.stack), "login");
}

/* -- Connect callback ------------------------------------------------------ */
void on_play_clicked(GtkButton *b, gpointer d)
{
    (void)b; (void)d;
    gtk_label_set_text(GTK_LABEL(W.login_status), "Offline mode disabled for server testing.");
}

void on_connect_clicked(GtkButton *b, gpointer d)
{
    (void)b; (void)d;
    const char *name = gtk_entry_get_text(GTK_ENTRY(W.name_entry));
    const char *host = gtk_entry_get_text(GTK_ENTRY(W.host_entry));
    const char *port = gtk_entry_get_text(GTK_ENTRY(W.port_entry));

    if (C.connected) { show_game_screen(); return; }
    if (!name[0]) { gtk_label_set_text(GTK_LABEL(W.login_status), "Enter your name."); return; }
    if (!host[0]) { gtk_label_set_text(GTK_LABEL(W.login_status), "Enter a host.");    return; }
    if (!port[0]) { gtk_label_set_text(GTK_LABEL(W.login_status), "Enter a port.");    return; }

    init_client_state(&C);
    int sockfd = connect_to_server(host, atoi(port));
    if (sockfd < 0) {
        gtk_label_set_text(GTK_LABEL(W.login_status), "Could not connect to server.");
        return;
    }
    C.socket_fd    = sockfd;
    C.connected    = 1;
    C.my_player_id = 0;
    sendNameToServer(name);

    if (W.net_source) { g_source_remove(W.net_source); W.net_source = 0; }
    GIOChannel *ch = g_io_channel_unix_new(sockfd);
    g_io_channel_set_encoding(ch, NULL, NULL);
    W.net_source = g_io_add_watch(ch, G_IO_IN | G_IO_HUP | G_IO_ERR, on_server_data, NULL);
    g_io_channel_unref(ch);

    gtk_label_set_text(GTK_LABEL(W.login_status), "");
    show_game_screen();
    append_chat("SERVER", "Type /ready to ready up.");
}

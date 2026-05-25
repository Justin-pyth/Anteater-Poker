#include "gui.h"
#include "gui_helpers.h"
#include <stdio.h>
#include <string.h>
#include "gui_extensions.h"
GuiExtensions EXT;

int PLAYER_ACTIVE = 1;

void on_send_chat_button_clicked(GtkEntry *entry, gpointer data)
{
    (void)data;
    //get the text from chat box
    const char *text = gtk_entry_get_text(entry);
    if(text == NULL || strlen(text) == 0) return;   //return if nothing written
    if(strcmp(text, "/ready") == 0) //if command = /ready, then ready up instead sending a chat message
    {
        sendReadyToServer();
        appendChat("ADMIN", "READY");
    }
    else
    {   //send a chat message if not a command
        sendChatToServer(C.my_player_id, text);
    }

    //empty the entry box
    gtk_entry_set_text(entry, "");
}

void sendChatToServer(uint8_t sender_id, const char *chatMessage)
{
    if(!C.connected) return; //check if the client is connected

    //encode into a payload (chat message type)
    Message msg;
    msg.type = MSG_TYPE_CHAT_MESSAGE;
    msg.sender_id = sender_id;

    //copy the chat message into the msg.chat
    strncpy(msg.chat, chatMessage, MAX_PAYLOAD_SIZE-1);
    msg.chat[MAX_PAYLOAD_SIZE-1] = '\0'; //terminate char

    uint8_t buffer[BUFFER_SIZE];
    uint32_t len = prepare_payload(buffer, MSG_TYPE_CHAT_MESSAGE, &msg);

    send_to_server(&C, buffer, len);

}


void appendChat(const char *sender, const char *chatMessage)
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
    GtkTextIter last;

    gtk_text_buffer_get_end_iter(buf, &last);

    // "Name : "
    gtk_text_buffer_insert(buf, &last, sender, -1);
    gtk_text_buffer_insert(buf, &last, " : ", -1);

    //add message after "Name : "
    gtk_text_buffer_insert(buf, &last, chatMessage, -1);
    gtk_text_buffer_insert(buf, &last, "\n", -1);
}

void sendReadyToServer()
{
    if(!C.connected) return; //check if the client is connected

    //encode the message into [type][length][payload]
    Message msg;
    msg.type = MSG_TYPE_READY;
    uint8_t buffer[BUFFER_SIZE];
    uint32_t len = prepare_payload(buffer, MSG_TYPE_READY, &msg);

    //send the payload to the server
    send_to_server(&C, buffer, len);
}

int card_is_known(Card card)
{
    return card.rank != UNKNOW_R && card.suit != UNKNOW_S;
}

int rank_index(uint8_t rank)
{
    if (rank < TWO || rank > ACE) return 0;
    return rank - TWO;
}

const char *suit_label(uint8_t suit)
{
    if (suit < HEARTS || suit > SPADES) return "?";
    return SUIT_STR[suit - HEARTS];
}

int suit_is_red(uint8_t suit)
{
    if (suit < HEARTS || suit > SPADES) return 0;
    return SUIT_RED[suit - HEARTS];
}

/* -- Anteater deck --------------------------------------------------------- */
void set_anteater_count(int count)
{
    if (!W.anteater_count_label) return;
    char buf[64];
    snprintf(buf, sizeof(buf), "Anteater:\n%d card%s", count, count == 1 ? "" : "s");
    gtk_label_set_text(GTK_LABEL(W.anteater_count_label), buf);
}

/* -- Server IO callback (replaces poll_server_cb) -------------------------- */
gboolean on_server_data(GIOChannel *ch, GIOCondition cond, gpointer data)
{
    (void)ch; (void)data;
    if (cond & (G_IO_HUP | G_IO_ERR)) { C.connected = 0; return FALSE; }

    Message msg;
    if (handle_server_communication(&C, &msg) != 0) {
        C.connected = 0;
        return FALSE;
    }

    if (msg.type == MSG_TYPE_GAME_STATE) {
        C.game = msg.gameState;
        refresh_ui();
    } else if (msg.type == MSG_TYPE_CHAT_MESSAGE) {
        append_chat(C.game.players[msg.sender_id].name, msg.chat);
    } else if (msg.type == MSG_CD_SIGNAL) {
        uint8_t target = msg.sender_id;
        /* stop all running timers first */
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

    const char *stage_names[] = {"PREFLOP", "FLOP", "TURN", "RIVER"};
    gtk_label_set_text(GTK_LABEL(W.stage_label),
        game->stage <= RIVER ? stage_names[game->stage] : "WAITING");

    snprintf(buf, sizeof(buf), "Players: %u", game->playerCount);
    gtk_label_set_text(GTK_LABEL(W.deck_count_label), buf);

    for (int i = 0; i < 5; i++) {
        if (i < game->communityCount) set_card_face(W.community_cards[i], game->community[i], 1);
        else                          set_card_back(W.community_cards[i]);
    }

    Player *me = &game->players[C.my_player_id];
    if (me->has_cards) {
        set_card_face(W.my_cards[0], me->hand[0], card_is_known(me->hand[0]));
        set_card_face(W.my_cards[1], me->hand[1], card_is_known(me->hand[1]));
    } else {
        set_card_back(W.my_cards[0]);
        set_card_back(W.my_cards[1]);
    }

    /* opponent seats (now 5 via EXT) */
    int opp_slot = 0;
    for (int pi = 0; pi < MAX_PLAYERS && opp_slot < GUI_OPPONENT_SLOTS; pi++) {
        if (pi == C.my_player_id) continue;
        Player *p = &game->players[pi];
        if (p->status == PLAYER_EMPTY) continue;

        gtk_label_set_text(GTK_LABEL(W.opp_name[opp_slot]),
            p->name[0] ? p->name : "Player");
        snprintf(buf, sizeof(buf), "$%u  |  bet $%u", p->chips, p->current_bet);
        gtk_label_set_text(GTK_LABEL(EXT.opp_chips[opp_slot]), buf);

        const char *st = "Waiting";
        if      (p->status == PLAYER_FOLDED)       st = "Folded";
        else if (p->status == PLAYER_ALL_IN)        st = "All in";
        else if (p->status == PLAYER_DISCONNECTED)  st = "Disconnected";
        else if (p->status == PLAYER_SPECTATING)    st = "Spectating";
        else if (pi == game->currentPlayer && game->handPlaying) st = "Acting";
        gtk_label_set_text(GTK_LABEL(EXT.opp_status[opp_slot]), st);

        GtkStyleContext *ctx = gtk_widget_get_style_context(EXT.opp_frame[opp_slot]);
        if (pi == game->currentPlayer && game->handPlaying)
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
    else if (game->handPlaying)
        snprintf(buf, sizeof(buf), my_turn ? "Your turn  [preview]" : "Waiting for player %u  [preview]", game->currentPlayer);
    else
        snprintf(buf, sizeof(buf), "Not connected.");
    gtk_label_set_text(GTK_LABEL(W.log_label), buf);

    gtk_widget_set_sensitive(W.btn_fold,  my_turn);
    gtk_widget_set_sensitive(W.btn_check, my_turn && can_check);
    gtk_widget_set_sensitive(W.btn_call,  my_turn && !can_check);
    gtk_widget_set_sensitive(W.btn_raise, my_turn);
    gtk_widget_set_sensitive(W.btn_ready, C.connected && !game->handPlaying);
    gtk_widget_set_sensitive(W.raise_spin, my_turn);
}

/* -- Action button callbacks ----------------------------------------------- */
void on_fold (GtkButton *b, gpointer d) { (void)b;(void)d; send_gui_move(FOLD,  0); }
void on_check(GtkButton *b, gpointer d) { (void)b;(void)d; send_gui_move(CHECK, 0); }
void on_call (GtkButton *b, gpointer d) { (void)b;(void)d; send_gui_move(CALL,  0); }
void on_raise(GtkButton *b, gpointer d)
{
    (void)b; (void)d;
    uint32_t amount = (uint32_t)gtk_spin_button_get_value(GTK_SPIN_BUTTON(W.raise_spin));
    send_gui_move(RAISE, amount);
}
void on_ready(GtkButton *b, gpointer d)
{
    (void)b; (void)d;
    sendReadyToServer();
    append_chat("ADMIN", "You are ready.");
}

/* -- Chat callbacks -------------------------------------------------------- */
void on_send_chat(GtkButton *b, gpointer d)
{
    (void)b; (void)d;
    const char *text = gtk_entry_get_text(GTK_ENTRY(W.chat_entry));
    if (!text || text[0] == '\0') return;
    if (strcmp(text, "/ready") == 0) {
        sendReadyToServer();
        append_chat("ADMIN", "You are ready.");
    } else {
        sendChatToServer(text);
        append_chat(C.game.players[C.my_player_id].name, text);
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
    gtk_stack_set_visible_child_name(GTK_STACK(W.stack), "game");
    gtk_widget_show_all(W.game_screen);
    gtk_window_resize(GTK_WINDOW(W.window), 900, 660);
    refresh_ui();

    /* Start preview timers so the timer UI is visible */
    if (!C.connected) {
        ext_start_player_timer(&EXT, 0, 30);
        ext_start_player_timer(&EXT, 1, 20);
    }
}
}

/* -- Poll server without blocking GTK ------------------------------------- */
gboolean poll_server_cb(gpointer data)
{
    (void)data;
    if (!C.connected) return FALSE;

    fd_set read_fds;
    struct timeval timeout = {0, 0};
    FD_ZERO(&read_fds);
    FD_SET(C.socket_fd, &read_fds);

    int ready = select(C.socket_fd + 1, &read_fds, NULL, NULL, &timeout);

    if (ready > 0 && FD_ISSET(C.socket_fd, &read_fds)) 
    {
        //write data into this variable
        Message msg;

        //provide client fd and write the type into msg
        if (handle_server_communication(&C, &msg) == 0)
        {
            if(msg.type == MSG_TYPE_GAME_STATE)
            {
                C.game = msg.gameState;
                refresh_ui();
            }
            else if (msg.type == MSG_TYPE_CHAT_MESSAGE)
            {
                appendChat(C.game.players[msg.sender_id].name, msg.chat);
            }
            else if (msg.type == MSG_TYPE_ERROR_MESSAGE)
            {
                gtk_label_set_text(GTK_LABEL(W.log_label), msg.error);
            }
        }
    }

    return C.connected;
}

/* -- Play vs bots callback ------------------------------------------------- */
static void on_play_clicked(GtkButton *btn, gpointer user_data)
{
    (void)btn;
    (void)user_data;

    const char *name = gtk_entry_get_text(GTK_ENTRY(W.name_entry));
    if (strlen(name) == 0) {
        gtk_label_set_text(GTK_LABEL(W.login_status), "Enter your name first.");
        return;
    }

    C.connected    = 0;
    C.my_player_id = 0;

    GameState *g = &C.game;
    g->pot            = 150;
    g->currentBet     = 50;
    g->playerCount    = 6;
    g->communityCount = 3;
    g->stage          = 1;
    g->handPlaying    = 1;
    g->currentPlayer  = 0;

    g->community[0].rank = ACE;   g->community[0].suit = HEARTS;
    g->community[1].rank = KING;  g->community[1].suit = SPADES;
    g->community[2].rank = QUEEN; g->community[2].suit = DIAMONDS;

    g->players[0].has_cards      = 1;
    g->players[0].hand[0].rank   = JACK; g->players[0].hand[0].suit = CLUBS;
    g->players[0].hand[1].rank   = TEN;  g->players[0].hand[1].suit = HEARTS;
    g->players[0].chips          = 1000;
    g->players[0].current_bet    = 50;
    g->players[0].status         = PLAYER_ACTIVE;
    strncpy(g->players[0].name, name, MAX_NAME_LENTH - 1);

    const char *opp_names[] = {"Alice", "Bob", "Charlie", "Diana", "Eve"};
    for (int i = 1; i <= 5; i++) {
        g->players[i].status      = PLAYER_ACTIVE;
        g->players[i].chips       = 800 + i * 100;
        g->players[i].current_bet = 50;
        g->players[i].has_cards   = 1;
        strncpy(g->players[i].name, opp_names[i-1], MAX_NAME_LENTH - 1);
    }

    gtk_label_set_text(GTK_LABEL(W.login_status), "");
    show_game_screen();
}

/* -- Connect to server callback ------------------------------------------- */
void on_connect_clicked(GtkButton *btn, gpointer user_data)
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

    if (W.net_source) { g_source_remove(W.net_source); W.net_source = 0; }
    GIOChannel *ch = g_io_channel_unix_new(sockfd);
    g_io_channel_set_encoding(ch, NULL, NULL);
    W.net_source = g_io_add_watch(ch, G_IO_IN | G_IO_HUP | G_IO_ERR, on_server_data, NULL);
    g_io_channel_unref(ch);

    gtk_label_set_text(GTK_LABEL(W.login_status), "");
    show_game_screen();
}

/* -- Build: opponent seat timer row helper --------------------------------- */
void build_seat_timer(GtkWidget *col, SeatTimer *t, const char *bar_name, const char *lbl_class)
{
    GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    gtk_box_pack_start(GTK_BOX(col), row, FALSE, FALSE, 2);

    t->bar = gtk_progress_bar_new();
    if (bar_name) gtk_widget_set_name(t->bar, bar_name);
    else gtk_style_context_add_class(gtk_widget_get_style_context(t->bar), "seat-timer-bar");
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(t->bar), 0.0);
    gtk_widget_set_hexpand(t->bar, TRUE);
    gtk_widget_hide(t->bar);
    gtk_box_pack_start(GTK_BOX(row), t->bar, TRUE, TRUE, 0);

    t->label = gtk_label_new("");
    if (lbl_class) gtk_style_context_add_class(gtk_widget_get_style_context(t->label), lbl_class);
    gtk_box_pack_start(GTK_BOX(row), t->label, FALSE, FALSE, 0);

    t->timer_id     = 0;
    t->seconds_left = 0;
    t->turn_seconds = TURN_SECONDS;
}

/* -- Build: game screen ---------------------------------------------------- */
GtkWidget *build_game_screen_widget(void)
{
    GtkWidget *root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_name(root, "game-root");

    /* top bar */
    GtkWidget *topbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 16);
    gtk_widget_set_margin_start(topbar, 20);
    gtk_widget_set_margin_end(topbar, 20);
    gtk_widget_set_margin_top(topbar, 10);
    gtk_box_pack_start(GTK_BOX(root), topbar, FALSE, FALSE, 0);
    GtkWidget *title = gtk_label_new("ANTEATER POKER");
    gtk_widget_set_name(title, "login-title");
    gtk_box_pack_start(GTK_BOX(topbar), title, FALSE, FALSE, 0);
    W.deck_count_label = gtk_label_new("Players: 0");
    gtk_box_pack_end(GTK_BOX(topbar), W.deck_count_label, FALSE, FALSE, 0);

    /* main area */
    GtkWidget *main_area = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_widget_set_margin_start(main_area, 20);
    gtk_widget_set_margin_end(main_area, 20);
    gtk_box_pack_start(GTK_BOX(root), main_area, TRUE, TRUE, 0);

    /* opponent row */
    GtkWidget *opp_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_halign(opp_row, GTK_ALIGN_FILL);
    gtk_box_pack_start(GTK_BOX(main_area), opp_row, FALSE, FALSE, 4);

    for (int i = 0; i < GUI_OPPONENT_SLOTS; i++) {
        GtkWidget *frame = gtk_event_box_new();
        gtk_style_context_add_class(gtk_widget_get_style_context(frame), "opp-frame");
        W.opp_frame[i] = frame;

        GtkWidget *col = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
        gtk_widget_set_margin_start(col, 8); gtk_widget_set_margin_end(col, 8);
        gtk_widget_set_margin_top(col, 6);   gtk_widget_set_margin_bottom(col, 6);
        gtk_container_add(GTK_CONTAINER(frame), col);

        GtkWidget *card_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
        gtk_box_pack_start(GTK_BOX(col), card_row, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(card_row), make_card_widget(28, 40), FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(card_row), make_card_widget(28, 40), FALSE, FALSE, 0);

        W.opp_name[i] = gtk_label_new("Empty");
        gtk_style_context_add_class(gtk_widget_get_style_context(W.opp_name[i]), "opp-name");
        gtk_widget_set_halign(W.opp_name[i], GTK_ALIGN_START);

        W.opp_chips[i] = gtk_label_new("$0  |  bet $0");
        gtk_style_context_add_class(gtk_widget_get_style_context(W.opp_chips[i]), "opp-chips");
        gtk_widget_set_halign(W.opp_chips[i], GTK_ALIGN_START);

        W.opp_status[i] = gtk_label_new("Waiting");
        gtk_style_context_add_class(gtk_widget_get_style_context(W.opp_status[i]), "opp-status");
        gtk_widget_set_halign(W.opp_status[i], GTK_ALIGN_START);

        gtk_box_pack_start(GTK_BOX(col), W.opp_name[i],   FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(col), W.opp_chips[i],  FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(col), W.opp_status[i], FALSE, FALSE, 0);

        build_seat_timer(col, &W.opp_timer[i], NULL, "seat-timer-label");

        gtk_box_pack_start(GTK_BOX(opp_row), frame, TRUE, TRUE, 6);
    }

    /* felt + anteater side panel */
    GtkWidget *table_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_box_pack_start(GTK_BOX(main_area), table_row, TRUE, TRUE, 0);

    GtkWidget *felt = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_widget_set_name(felt, "felt");
    gtk_widget_set_margin_start(felt, 40); gtk_widget_set_margin_end(felt, 40);
    gtk_widget_set_vexpand(felt, TRUE);
    gtk_widget_set_valign(felt, GTK_ALIGN_CENTER);
    GtkWidget *table_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_box_pack_start(GTK_BOX(table_row), felt, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(table_row), build_anteater_deck_panel(&EXT), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(main_area), table_row, TRUE, TRUE, 0);

    W.stage_label = gtk_label_new("WAITING");
    gtk_widget_set_name(W.stage_label, "stage-label");
    gtk_box_pack_start(GTK_BOX(felt), W.stage_label, FALSE, FALSE, 4);

    W.pot_label = gtk_label_new("Pot: $0");
    gtk_widget_set_name(W.pot_label, "pot-label");
    gtk_box_pack_start(GTK_BOX(felt), W.pot_label, FALSE, FALSE, 0);

    GtkWidget *comm_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_set_halign(comm_row, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(felt), comm_row, FALSE, FALSE, 8);
    for (int i = 0; i < 5; i++) {
        W.community_cards[i] = make_card_widget(48, 68);
        gtk_box_pack_start(GTK_BOX(comm_row), W.community_cards[i], FALSE, FALSE, 0);
    }

    W.log_label = gtk_label_new("Not connected.");
    gtk_widget_set_name(W.log_label, "log-label");
    gtk_box_pack_start(GTK_BOX(felt), W.log_label, FALSE, FALSE, 4);

    /* anteater side deck panel */
    W.anteater_panel = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_widget_set_name(W.anteater_panel, "anteater-panel");
    gtk_widget_set_size_request(W.anteater_panel, 110, -1);
    GtkWidget *ant_title = gtk_label_new("ANTEATER\nDECK");
    gtk_widget_set_name(ant_title, "anteater-title");
    gtk_label_set_justify(GTK_LABEL(ant_title), GTK_JUSTIFY_CENTER);
    gtk_box_pack_start(GTK_BOX(W.anteater_panel), ant_title, FALSE, FALSE, 0);
    W.anteater_count_label = gtk_label_new("Anteater:\n0 cards");
    gtk_widget_set_name(W.anteater_count_label, "anteater-count");
    gtk_box_pack_start(GTK_BOX(W.anteater_panel), W.anteater_count_label, FALSE, FALSE, 0);
    W.btn_draw_anteater = gtk_button_new_with_label("Draw");
    gtk_widget_set_name(W.btn_draw_anteater, "btn-draw-anteater");
    gtk_widget_set_sensitive(W.btn_draw_anteater, FALSE);
    gtk_box_pack_start(GTK_BOX(W.anteater_panel), W.btn_draw_anteater, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(table_row), W.anteater_panel, FALSE, FALSE, 0);

    /* my hand */
    GtkWidget *hand_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_widget_set_halign(hand_box, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(main_area), hand_box, FALSE, FALSE, 4);
    GtkWidget *hand_lbl = gtk_label_new("Your hand");
    gtk_style_context_add_class(gtk_widget_get_style_context(hand_lbl), "opp-chips");
    gtk_box_pack_start(GTK_BOX(hand_box), hand_lbl, FALSE, FALSE, 0);
    GtkWidget *hand_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_halign(hand_row, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(hand_box), hand_row, FALSE, FALSE, 0);
    W.my_cards[0] = make_card_widget(58, 82);
    W.my_cards[1] = make_card_widget(58, 82);
    gtk_box_pack_start(GTK_BOX(hand_row), W.my_cards[0], FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hand_row), W.my_cards[1], FALSE, FALSE, 0);

    /* my timer */
    GtkWidget *my_timer_col = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_halign(my_timer_col, GTK_ALIGN_CENTER);
    gtk_widget_set_size_request(my_timer_col, 200, -1);
    gtk_box_pack_start(GTK_BOX(main_area), my_timer_col, FALSE, FALSE, 0);
    build_seat_timer(my_timer_col, &W.my_timer, "my-timer-bar", "my-timer-label");

    /* action buttons */
    GtkWidget *btn_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_halign(btn_row, GTK_ALIGN_CENTER);
    gtk_widget_set_margin_bottom(btn_row, 8);
    gtk_box_pack_start(GTK_BOX(main_area), btn_row, FALSE, FALSE, 4);

    W.btn_fold  = gtk_button_new_with_label("Fold");
    W.btn_check = gtk_button_new_with_label("Check");
    W.btn_call  = gtk_button_new_with_label("Call");
    W.btn_raise = gtk_button_new_with_label("Raise");
    W.btn_ready = gtk_button_new_with_label("Ready");
    W.btn_shop  = gtk_button_new_with_label("Shop");

    gtk_widget_set_name(W.btn_fold,  "btn-fold");
    gtk_widget_set_name(W.btn_check, "btn-check");
    gtk_widget_set_name(W.btn_call,  "btn-call");
    gtk_widget_set_name(W.btn_raise, "btn-raise");
    gtk_widget_set_name(W.btn_ready, "btn-ready");
    gtk_widget_set_name(W.btn_shop,  "btn-shop");

    const char *ab = "action-btn";
    gtk_style_context_add_class(gtk_widget_get_style_context(W.btn_fold),  ab);
    gtk_style_context_add_class(gtk_widget_get_style_context(W.btn_check), ab);
    gtk_style_context_add_class(gtk_widget_get_style_context(W.btn_call),  ab);
    gtk_style_context_add_class(gtk_widget_get_style_context(W.btn_raise), ab);
    gtk_style_context_add_class(gtk_widget_get_style_context(W.btn_ready), ab);
    gtk_style_context_add_class(gtk_widget_get_style_context(W.btn_shop),  ab);

    g_signal_connect(W.btn_fold,  "clicked", G_CALLBACK(on_fold),  NULL);
    g_signal_connect(W.btn_check, "clicked", G_CALLBACK(on_check), NULL);
    g_signal_connect(W.btn_call,  "clicked", G_CALLBACK(on_call),  NULL);
    g_signal_connect(W.btn_raise, "clicked", G_CALLBACK(on_raise), NULL);
    g_signal_connect(W.btn_ready, "clicked", G_CALLBACK(on_ready), NULL);

    GtkAdjustment *adj = gtk_adjustment_new(50, 10, 10000, 10, 50, 0);
    W.raise_spin = gtk_spin_button_new(adj, 10, 0);
    gtk_widget_set_size_request(W.raise_spin, 80, -1);

    gtk_box_pack_start(GTK_BOX(btn_row), W.btn_fold,   FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(btn_row), W.btn_check,  FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(btn_row), W.btn_call,   FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(btn_row), W.raise_spin, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(btn_row), W.btn_raise,  FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(btn_row), W.btn_ready,  FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(btn_row), W.btn_shop,   FALSE, FALSE, 0);

    /* chat box */
    GtkWidget *chat_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_widget_set_name(chat_box, "chat-box");
    gtk_widget_set_size_request(chat_box, -1, 140);
    gtk_widget_set_margin_start(chat_box, 12);
    gtk_widget_set_margin_end(chat_box, 12);
    gtk_widget_set_margin_bottom(chat_box, 8);
    gtk_box_pack_start(GTK_BOX(root), chat_box, FALSE, FALSE, 0);

    GtkWidget *chat_title = gtk_label_new("CHAT");
    gtk_widget_set_name(chat_title, "chat-title");
    gtk_widget_set_halign(chat_title, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(chat_box), chat_title, FALSE, FALSE, 0);

    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
        GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_vexpand(scroll, TRUE);
    gtk_box_pack_start(GTK_BOX(chat_box), scroll, TRUE, TRUE, 0);

    W.chat_log = gtk_text_view_new();
    gtk_widget_set_name(W.chat_log, "chat-log");
    gtk_text_view_set_editable(GTK_TEXT_VIEW(W.chat_log), FALSE);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(W.chat_log), FALSE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(W.chat_log), GTK_WRAP_WORD_CHAR);
    gtk_container_add(GTK_CONTAINER(scroll), W.chat_log);

    GtkWidget *input_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_box_pack_start(GTK_BOX(chat_box), input_row, FALSE, FALSE, 0);

    W.chat_entry = gtk_entry_new();
    gtk_widget_set_name(W.chat_entry, "chat-entry");
    gtk_entry_set_placeholder_text(GTK_ENTRY(W.chat_entry), "Type a message or /ready…");
    gtk_widget_set_hexpand(W.chat_entry, TRUE);
    g_signal_connect(W.chat_entry, "activate", G_CALLBACK(on_chat_activate), NULL);
    gtk_box_pack_start(GTK_BOX(input_row), W.chat_entry, TRUE, TRUE, 0);

    W.btn_send_chat = gtk_button_new_with_label("Send");
    gtk_widget_set_name(W.btn_send_chat, "btn-send-chat");
    g_signal_connect(W.btn_send_chat, "clicked", G_CALLBACK(on_send_chat), NULL);
    gtk_box_pack_start(GTK_BOX(input_row), W.btn_send_chat, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(root), build_chat_box(&EXT), FALSE, FALSE, 0);
    refresh_ui();
    return root;
}

/* -- Build the login screen ------------------------------------------------ */
GtkWidget *build_login_screen(void)
{
    GtkWidget *outer = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_valign(outer, GTK_ALIGN_CENTER);
    gtk_widget_set_halign(outer, GTK_ALIGN_CENTER);

    GtkWidget *card = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_name(card, "login-card");
    gtk_widget_set_size_request(card, 360, -1);
    gtk_box_pack_start(GTK_BOX(outer), card, FALSE, FALSE, 0);

    GtkWidget *t = gtk_label_new("ANTEATER POKER");
    gtk_widget_set_name(t, "login-title");
    gtk_box_pack_start(GTK_BOX(card), t, FALSE, FALSE, 0);

    GtkWidget *s = gtk_label_new("TEAM 99%  ·  UCI");
    gtk_widget_set_name(s, "login-sub");
    gtk_box_pack_start(GTK_BOX(card), s, FALSE, FALSE, 6);

    GtkWidget *suits = gtk_label_new("♠  ♥  ♦  ♣");
    gtk_widget_set_name(suits, "login-suits");
    gtk_box_pack_start(GTK_BOX(card), suits, FALSE, FALSE, 0);

    /* helper macro for form rows */
#define FIELD(lbl_text, entry_ptr, ph, def) do { \
    GtkWidget *lbl = gtk_label_new(lbl_text); \
    gtk_widget_set_halign(lbl, GTK_ALIGN_START); \
    gtk_style_context_add_class(gtk_widget_get_style_context(lbl), "field-lbl"); \
    gtk_box_pack_start(GTK_BOX(card), lbl, FALSE, FALSE, 10); \
    entry_ptr = gtk_entry_new(); \
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry_ptr), ph); \
    if (def[0]) gtk_entry_set_text(GTK_ENTRY(entry_ptr), def); \
    gtk_box_pack_start(GTK_BOX(card), entry_ptr, FALSE, FALSE, 6); \
} while(0)

    FIELD("PLAYER NAME", W.name_entry, "Enter your name", "");
    gtk_entry_set_max_length(GTK_ENTRY(W.name_entry), 31);

    /* -- Play vs bots button -- */
    GtkWidget *btn = gtk_button_new_with_label("PLAY  ♠  vs Bots");
    gtk_widget_set_name(btn, "connect-btn");
    gtk_box_pack_start(GTK_BOX(card), btn, FALSE, FALSE, 20);
    g_signal_connect(btn, "clicked", G_CALLBACK(on_play_clicked), NULL);
    g_signal_connect_swapped(W.name_entry, "activate", G_CALLBACK(gtk_button_clicked), btn);

    /* -- Divider -- */
    GtkWidget *divider = gtk_label_new("────  OR  ────");
    gtk_widget_set_name(divider, "divider-label");
    gtk_widget_set_halign(divider, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(card), divider, FALSE, FALSE, 6);

    /* -- Server fields -- */
    FIELD("SERVER HOST", W.host_entry, "localhost", "localhost");
    FIELD("PORT", W.port_entry, "10160", "10160");
#undef FIELD

    /* -- Connect to server button -- */
    GtkWidget *conn_btn = gtk_button_new_with_label("CONNECT TO SERVER");
    gtk_widget_set_name(conn_btn, "offline-btn");
    gtk_box_pack_start(GTK_BOX(card), conn_btn, FALSE, FALSE, 10);
    g_signal_connect(conn_btn, "clicked", G_CALLBACK(on_connect_clicked), NULL);
    g_signal_connect_swapped(W.port_entry, "activate", G_CALLBACK(gtk_button_clicked), conn_btn);

    W.login_status = gtk_label_new("");
    gtk_widget_set_name(W.login_status, "login-status");
    gtk_label_set_line_wrap(GTK_LABEL(W.login_status), TRUE);
    gtk_box_pack_start(GTK_BOX(card), W.login_status, FALSE, FALSE, 0);

    return outer;
}

/* -- Main ------------------------------------------------------------------ */
int main(int argc, char *argv[])
{
    gtk_init(&argc, &argv);
    init_client_state(&C);

    GtkCssProvider *css_prov = gtk_css_provider_new();
    gtk_css_provider_load_from_data(css_prov, CSS, -1, NULL);
    gtk_style_context_add_provider_for_screen(
        gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(css_prov),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    W.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(W.window), "Anteater Poker");
    gtk_window_set_default_size(GTK_WINDOW(W.window), 420, 520);
    gtk_window_set_position(GTK_WINDOW(W.window), GTK_WIN_POS_CENTER);
    g_signal_connect(W.window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    /* stack holds both screens; we swap visibility manually */
    W.stack = gtk_stack_new();
    gtk_container_add(GTK_CONTAINER(W.window), W.stack);

    W.login_screen = build_login_screen();
    W.game_screen = build_game_screen();

    gtk_stack_add_named(GTK_STACK(W.stack), W.login_screen, "login");
    gtk_stack_add_named(GTK_STACK(W.stack), W.game_screen, "game");
    gtk_stack_set_visible_child_name(GTK_STACK(W.stack), "login");

    gtk_widget_show_all(W.window);
    gtk_widget_hide(W.game_screen);

    gtk_main();
    return 0;
}

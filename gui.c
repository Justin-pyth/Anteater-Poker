#include "gui.h"
#include "gui_helpers.h"
#include "rules.h"        /* evaluateHand for the last-hand panel rankings */
#include <stdio.h>
#include <string.h>

/* -- Globals --------------------------------------------------------------- */
AppWidgets W;
ClientState C;

/* snapshot of the most recently completed hand (for the Last Hand Info panel) */
static GameState g_last_hand;
static gboolean  g_have_last_hand = FALSE;

static const char *hand_type_name(int type)
{
    switch (type) {
        case ROYAL_FLUSH:    return "Royal Flush";
        case STRAIGHT_FLUSH: return "Straight Flush";
        case FOUR_KIND:      return "Four of a Kind";
        case FULL_HOUSE:     return "Full House";
        case FLUSH:          return "Flush";
        case STRAIGHT:       return "Straight";
        case THREE_KIND:     return "Three of a Kind";
        case TWO_PAIR:       return "Two Pair";
        case PAIR:           return "Pair";
        case HIGHT_CARD:     return "High Card";
        default:             return "-";
    }
}

/* -- Chat display ---------------------------------------------------------- */
void append_chat(const char *sender, const char *msg, const char *tag_name)
{
    if (!W.chat_log) return;
    GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(W.chat_log));

    //give different color text (server vs player)
    if (!gtk_text_tag_table_lookup(gtk_text_buffer_get_tag_table(buf), "server"))
    {
        gtk_text_buffer_create_tag(buf, "server", "foreground", "#4B5563", "style", PANGO_STYLE_ITALIC, NULL);
    }
    if (!gtk_text_tag_table_lookup(gtk_text_buffer_get_tag_table(buf), "server_alert"))
    {
        gtk_text_buffer_create_tag(buf, "server_alert", "foreground", "#B45309", NULL);
    } 
    if (!gtk_text_tag_table_lookup(gtk_text_buffer_get_tag_table(buf), "player"))
    {
        gtk_text_buffer_create_tag(buf, "player", "foreground", "#000000", NULL);
    }
    //also add a bold tag for the sender names
    if (!gtk_text_tag_table_lookup(gtk_text_buffer_get_tag_table(buf), "bold"))
    {
        gtk_text_buffer_create_tag(buf, "bold", "weight", PANGO_WEIGHT_BOLD, NULL);
    }

    GtkTextIter end;
    gtk_text_buffer_get_end_iter(buf, &end);

    //make the sender name bold, rest of the text unbolded
    gtk_text_buffer_insert_with_tags_by_name(buf, &end, sender ? sender : "?", -1, "bold", tag_name, NULL);
    gtk_text_buffer_insert_with_tags_by_name(buf, &end, ": ", -1, tag_name, NULL);
    gtk_text_buffer_insert_with_tags_by_name(buf, &end, msg ? msg : "", -1, tag_name, NULL);
    gtk_text_buffer_insert_with_tags_by_name(buf, &end, "\n", -1, tag_name, NULL);

    gtk_text_buffer_get_end_iter(buf, &end);
    GtkTextMark *mark = gtk_text_buffer_get_mark(buf, "insert");
    gtk_text_buffer_move_mark(buf, mark, &end);
    gtk_text_view_scroll_mark_onscreen(GTK_TEXT_VIEW(W.chat_log), mark);
}


/* -- Dispatch one parsed message ------------------------------------------- */
static void dispatch_message(Message *msg)
{
    if (msg->type == MSG_TYPE_GAME_STATE) {
        C.game = msg->gameState;
        if (C.game.yourPlayerID >= MAX_PLAYERS) {
            //not seated yet: show the blocking seat-select overlay (re-populated each
            //state so seats taken by others lose their Select button)
            show_seat_select();
        } else {
            hide_seat_select();
            refresh_ui();
        }
    } else if (msg->type == MSG_TYPE_CHAT_MESSAGE) {
        if (msg->sender_id >= MAX_PLAYERS)
            append_chat("SERVER", msg->chat, "server");
        else
            append_chat(C.game.players[msg->sender_id].name, msg->chat, "player");
    } else if (msg->type == MSG_TYPE_ERROR_MESSAGE) {
        gtk_label_set_text(GTK_LABEL(W.log_label), msg->error);
    } else if (msg->type == MSG_CD_SIGNAL) {
        uint8_t target = msg->sender_id;
        stop_my_timer();
        for (int i = 0; i < GUI_OPPONENT_SLOTS; i++) stop_player_timer(i);
        if (target == C.my_player_id) {
            start_my_timer(TURN_SECONDS);
        } else if (target < MAX_PLAYERS) {
            //strip tile index == seat index
            start_player_timer(target, TURN_SECONDS);
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

    if (me->status == PLAYER_FOLDED) {
        gtk_widget_set_opacity(W.my_cards[0], 0);
        gtk_widget_set_opacity(W.my_cards[1], 0);
    } else {
        gtk_widget_set_opacity(W.my_cards[0], 1);
        gtk_widget_set_opacity(W.my_cards[1], 1);
    }
} else {
    gtk_widget_set_opacity(W.my_cards[0], 0);
    gtk_widget_set_opacity(W.my_cards[1], 0);
}

    /* small/big blind marker for your own seat (only during a live hand) */
    BlindKind myBlind = BLIND_NONE;
    if (game->handPlaying) {
        if      (C.my_player_id == game->smallBlindIndex) myBlind = BLIND_SB;
        else if (C.my_player_id == game->bigBlindIndex)   myBlind = BLIND_BB;
    }
    set_blind_marker(W.self_blind, myBlind);

    /* each seat maps to a fixed strip tile (tile index == seat index) */
    for (int pi = 0; pi < MAX_PLAYERS; pi++) {
        Player *p = &game->players[pi];

        if (p->status == PLAYER_EMPTY) {
            gtk_label_set_text(GTK_LABEL(W.opp_name[pi]),   "Empty");
            gtk_label_set_text(GTK_LABEL(W.opp_chips[pi]),  "$0  |  bet $0");
            gtk_label_set_text(GTK_LABEL(W.opp_status[pi]), "Waiting");
            gtk_widget_set_opacity(W.opp_cards[pi][0], 0);
            gtk_widget_set_opacity(W.opp_cards[pi][1], 0);
            set_blind_marker(W.opp_blind[pi], BLIND_NONE);
            gtk_style_context_remove_class(
                gtk_widget_get_style_context(W.opp_frame[pi]), "active-seat");
            continue;
        }

        gtk_widget_set_opacity(W.opp_cards[pi][0], 1);
        gtk_widget_set_opacity(W.opp_cards[pi][1], 1);

        /* small/big blind marker for this seat (only during a live hand) */
        BlindKind oppBlind = BLIND_NONE;
        if (game->handPlaying) {
            if      (pi == game->smallBlindIndex) oppBlind = BLIND_SB;
            else if (pi == game->bigBlindIndex)   oppBlind = BLIND_BB;
        }
        set_blind_marker(W.opp_blind[pi], oppBlind);

        gtk_label_set_text(GTK_LABEL(W.opp_name[pi]),
            p->name[0] ? p->name : "Player");
        snprintf(buf, sizeof(buf), "$%u  |  bet $%u", p->chips, p->total_bet);
        gtk_label_set_text(GTK_LABEL(W.opp_chips[pi]), buf);

        const char *st = "Waiting";
        if      (p->status == PLAYER_FOLDED)
        {
            gtk_widget_set_opacity(W.opp_cards[pi][0], 0);
            gtk_widget_set_opacity(W.opp_cards[pi][1], 0);
            st = "Folded";
        }
        else if (p->status == PLAYER_ALL_IN)        st = "All in";
        else if (p->status == PLAYER_DISCONNECTED)
        {
            gtk_widget_set_opacity(W.opp_cards[pi][0], 0);
            gtk_widget_set_opacity(W.opp_cards[pi][1], 0);
            st = "Disconnected";
        }
        else if (p->status == PLAYER_SPECTATING)
        {
            gtk_widget_set_opacity(W.opp_cards[pi][0], 0);
            gtk_widget_set_opacity(W.opp_cards[pi][1], 0);
            st = "Spectating";
        }
        else if (p->status == PLAYER_PLAYING && pi == game->currentPlayer && game->handPlaying) st = "Acting";
        gtk_label_set_text(GTK_LABEL(W.opp_status[pi]), st);

        if (p->has_cards) {
            if (card_is_known(p->hand[0]))
                set_card_face(W.opp_cards[pi][0], p->hand[0], 1);
            else
                set_card_back(W.opp_cards[pi][0]);
            if (card_is_known(p->hand[1]))
                set_card_face(W.opp_cards[pi][1], p->hand[1], 1);
            else
                set_card_back(W.opp_cards[pi][1]);
        } else {
            set_card_back(W.opp_cards[pi][0]);
            set_card_back(W.opp_cards[pi][1]);
        }

        GtkStyleContext *ctx = gtk_widget_get_style_context(W.opp_frame[pi]);
        if (p->status == PLAYER_PLAYING && pi == game->currentPlayer && game->handPlaying)
            gtk_style_context_add_class(ctx, "active-seat");
        else
            gtk_style_context_remove_class(ctx, "active-seat");
    }

    /* One-shot chip-win popup: fire once when a hand ends with a single winner.
       Alpha has no showdown flag, so trigger on the handPlaying true->false edge.
       endHand() has already zeroed the pot by now, so use the pot captured on the
       last live frame (prev_pot). */
    static int      prev_handPlaying = 0;
    static uint32_t prev_pot         = 0;
    if (prev_handPlaying && !game->handPlaying && game->winnerID < MAX_PLAYERS) {
        const char *wname = (game->winnerID == C.my_player_id)
            ? "You"
            : (game->players[game->winnerID].name[0]
               ? game->players[game->winnerID].name : "Player");
        start_chip_win_anim(wname, prev_pot);
    }
    /* snapshot the just-finished hand for the Last Hand Info panel (this edge fires
       on the HAND_COMPLETE broadcast, where all cards/community/winner are present) */
    if (prev_handPlaying && !game->handPlaying) {
        g_last_hand = *game;
        g_have_last_hand = TRUE;
        update_last_hand_panel();
    }
    prev_handPlaying = game->handPlaying;
    if (game->handPlaying) prev_pot = game->pot;  /* remember pot while the hand is live */

    //leaderboard check
    static int check_gameover = 0;
    if(!check_gameover && game->gameOver)
        show_leaderboard();
    check_gameover = game->gameOver;

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

    gboolean shop_ok = shop_is_available();
    if (W.btn_shop)
        gtk_widget_set_sensitive(W.btn_shop, shop_ok);
    if (shop_is_open()) {
        if (!shop_ok)
            shop_close();
        else
            refresh_shop_ui();
    }
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

void on_ready_clicked(GtkButton *b, gpointer d)
{
    (void)b; (void)d;
    if (W.leaderboard)
        gtk_widget_hide(W.leaderboard);
    sendReadyToServer();

}

void on_quit_clicked(GtkButton *b, gpointer d)
{
    (void)b; (void)d;
    gtk_main_quit();
}

/* -- Shop callbacks -------------------------------------------------------- */
void on_shop_clicked(GtkButton *b, gpointer d)
{
    (void)b; (void)d;
    shop_open();
}

void on_shop_back(GtkButton *b, gpointer d)
{
    (void)b; (void)d;
    shop_on_back();
}

void on_shop_confirm(GtkButton *b, gpointer d)
{
    (void)b; (void)d;
    shop_on_confirm();
}

void on_shop_card_clicked(GtkButton *b, gpointer d)
{
    (void)b;
    shop_on_card_slot_clicked(GPOINTER_TO_INT(d));
}

gboolean on_shop_my_card_press(GtkWidget *w, GdkEventButton *ev, gpointer d)
{
    (void)w;
    if (ev->type != GDK_BUTTON_PRESS || ev->button != 1) return FALSE;
    return shop_on_my_card_clicked(GPOINTER_TO_INT(d));
}

gboolean on_shop_opp_frame_press(GtkWidget *w, GdkEventButton *ev, gpointer d)
{
    (void)w;
    if (ev->type != GDK_BUTTON_PRESS || ev->button != 1) return FALSE;
    return shop_on_opponent_clicked(GPOINTER_TO_INT(d));
}

gboolean on_shop_opp_card_press(GtkWidget *w, GdkEventButton *ev, gpointer d)
{
    (void)w;
    if (ev->type != GDK_BUTTON_PRESS || ev->button != 1) return FALSE;
    int packed = GPOINTER_TO_INT(d);
    return shop_on_opp_card_clicked(packed / 2, packed % 2);
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
    append_chat("SERVER", "Type /ready to ready up.", "server_alert");
}


void show_leaderboard(void)
{
    if(!W.leaderboard) return; //check if it exists

    GameState *g = &C.game;

    //sort players by ascending place, keeping unplaced players at the end
    int order[MAX_PLAYERS];
    for(int i = 0; i < MAX_PLAYERS; i++) order[i] = i;
    for(int i = 0; i < MAX_PLAYERS-1; i++)
        for(int j = i+1; j < MAX_PLAYERS; j++)
        {
            int place_i = g->players[order[i]].place;
            int place_j = g->players[order[j]].place;

            if(place_i == 0) place_i = MAX_PLAYERS + 1;
            if(place_j == 0) place_j = MAX_PLAYERS + 1;

            if(place_i > place_j)
            {
                int t = order[i];
                order[i] = order[j];
                order[j] = t;
            }
        }
    
    //for ordering from 1->2->3->4->5->6
    int display_place = 0; //leaderboard place displayed
    int previous_place = 0;//raw place (inaccurate because of multiple people getting out at once)
    for(int i = 0; i < MAX_PLAYERS; i++)
    {
        Player *p = &g->players[order[i]];
        char buf[32];

        //if there are skipped places, fix
        if(p->place > 0 && p->place != previous_place)
        {
            display_place++;
            previous_place = p->place;
        }

        //add their name
        if(W.lb_name[i])
            gtk_label_set_text(GTK_LABEL(W.lb_name[i]), p->name[0] ? p->name : "Empty");
        if(W.lb_place[i]) //their place
        {
            snprintf(buf, sizeof(buf), "%d", display_place);
            gtk_label_set_text(GTK_LABEL(W.lb_place[i]), p->place > 0 ? buf : "-");
        } //and their last card (if it existed)
        if(W.lb_card[i][0])
            set_card_face(W.lb_card[i][0], p->hand[0], card_is_known(p->hand[0]));
        if(W.lb_card[i][1])
            set_card_face(W.lb_card[i][1], p->hand[1], card_is_known(p->hand[1]));
    }

    gtk_window_set_transient_for(GTK_WINDOW(W.leaderboard), GTK_WINDOW(W.window));
    gtk_widget_show_all(W.leaderboard);
}

/* -- Seat selection overlay ------------------------------------------------ */
void show_seat_select(void)
{
    if (!W.seat_select) return;

    GameState *g = &C.game;
    for (int i = 0; i < MAX_PLAYERS; i++) {
        Player *p = &g->players[i];
        int empty = (p->status == PLAYER_EMPTY);

        if (W.seat_sel_name[i])
            gtk_label_set_text(GTK_LABEL(W.seat_sel_name[i]),
                empty ? "— Empty Seat —" : (p->name[0] ? p->name : "Player"));

        //a Select button only appears next to an empty seat
        if (W.seat_sel_btn[i])
            gtk_widget_set_visible(W.seat_sel_btn[i], empty);
    }

    gtk_window_set_transient_for(GTK_WINDOW(W.seat_select), GTK_WINDOW(W.window));
    gtk_window_set_modal(GTK_WINDOW(W.seat_select), TRUE);
    gtk_widget_show(W.seat_select);
}

void hide_seat_select(void)
{
    if (W.seat_select)
        gtk_widget_hide(W.seat_select);
}

void on_seat_select_clicked(GtkButton *b, gpointer seat)
{
    (void)b;
    //the confirming game state (with a real yourPlayerID) closes the overlay
    sendSeatSelectToServer((uint8_t)GPOINTER_TO_INT(seat));
}

/* -- Last Hand Info side panel --------------------------------------------- */
void on_last_hand_toggle(GtkButton *b, gpointer d)
{
    (void)b; (void)d;
    if (!W.last_hand_revealer) return;
    gboolean open = gtk_revealer_get_reveal_child(GTK_REVEALER(W.last_hand_revealer));
    gtk_revealer_set_reveal_child(GTK_REVEALER(W.last_hand_revealer), !open);
}

void update_last_hand_panel(void)
{
    if (!W.last_hand_revealer) return; //panel not built yet

    if (!g_have_last_hand) {
        for (int i = 0; i < 5; i++)
            if (W.lh_community[i]) set_card_back(W.lh_community[i]);
        for (int i = 0; i < MAX_PLAYERS; i++) {
            if (W.lh_name[i])  gtk_label_set_text(GTK_LABEL(W.lh_name[i]), "-");
            if (W.lh_rank[i])  gtk_label_set_text(GTK_LABEL(W.lh_rank[i]), "");
            if (W.lh_cards[i][0]) set_card_back(W.lh_cards[i][0]);
            if (W.lh_cards[i][1]) set_card_back(W.lh_cards[i][1]);
            if (W.lh_name[i])
                gtk_style_context_remove_class(
                    gtk_widget_get_style_context(W.lh_name[i]), "lh-winner");
        }
        return;
    }

    GameState *g = &g_last_hand;

    for (int i = 0; i < 5; i++) {
        if (!W.lh_community[i]) continue;
        if (i < g->communityCount)
            set_card_face(W.lh_community[i], g->community[i], card_is_known(g->community[i]));
        else
            set_card_back(W.lh_community[i]);
    }

    for (int i = 0; i < MAX_PLAYERS; i++) {
        Player *p = &g->players[i];
        int empty = (p->status == PLAYER_EMPTY);

        if (W.lh_name[i])
            gtk_label_set_text(GTK_LABEL(W.lh_name[i]),
                empty ? "Empty" : (p->name[0] ? p->name : "Player"));

        if (!empty && p->has_cards) {
            set_card_face(W.lh_cards[i][0], p->hand[0], card_is_known(p->hand[0]));
            set_card_face(W.lh_cards[i][1], p->hand[1], card_is_known(p->hand[1]));
        } else {
            set_card_back(W.lh_cards[i][0]);
            set_card_back(W.lh_cards[i][1]);
        }

        const char *rank = "";
        if (empty)                            rank = "";
        else if (p->status == PLAYER_FOLDED)  rank = "Folded";
        else if (p->has_cards && card_is_known(p->hand[0])) {
            int score = evaluateHand(g, p->hand);
            rank = hand_type_name((score >> 20) & 0xF);
        } else                                rank = "-";
        if (W.lh_rank[i]) gtk_label_set_text(GTK_LABEL(W.lh_rank[i]), rank);

        if (W.lh_name[i]) {
            GtkStyleContext *ctx = gtk_widget_get_style_context(W.lh_name[i]);
            if (!empty && i == (int)g->winnerID)
                gtk_style_context_add_class(ctx, "lh-winner");
            else
                gtk_style_context_remove_class(ctx, "lh-winner");
        }
    }
}

/* The panel is defined in anteater_poker.glade (the last_hand_* / lh_* widgets).
   This binds those widgets and attaches the card draw callbacks; the toggle's
   "clicked" signal is wired by gtk_builder_connect_signals -> on_last_hand_toggle. */
void build_last_hand_panel(GtkBuilder *builder)
{
    char id[24];

    W.last_hand_revealer = GTK_WIDGET(gtk_builder_get_object(builder, "last_hand_revealer"));
    W.last_hand_toggle   = GTK_WIDGET(gtk_builder_get_object(builder, "last_hand_toggle"));

    for (int i = 0; i < 5; i++) {
        snprintf(id, sizeof(id), "lh_comm_%d", i);
        W.lh_community[i] = GTK_WIDGET(gtk_builder_get_object(builder, id));
        init_card_widget(W.lh_community[i]);
    }

    for (int i = 0; i < MAX_PLAYERS; i++) {
        snprintf(id, sizeof(id), "lh_name_%d", i);   W.lh_name[i]    = GTK_WIDGET(gtk_builder_get_object(builder, id));
        snprintf(id, sizeof(id), "lh_rank_%d", i);   W.lh_rank[i]    = GTK_WIDGET(gtk_builder_get_object(builder, id));
        snprintf(id, sizeof(id), "lh_card_%d_a", i); W.lh_cards[i][0] = GTK_WIDGET(gtk_builder_get_object(builder, id));
        snprintf(id, sizeof(id), "lh_card_%d_b", i); W.lh_cards[i][1] = GTK_WIDGET(gtk_builder_get_object(builder, id));
        init_card_widget(W.lh_cards[i][0]);
        init_card_widget(W.lh_cards[i][1]);
    }

    update_last_hand_panel();
}

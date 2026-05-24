#include "gui.h"
#include "gui_extensions.h"
static GuiExtensions EXT;

/* -- Suits / Ranks --------------------------------------------------------- */
static const char *RANK_STR[13] = {
    "2", "3", "4", "5", "6", "7", "8", "9", "10", "J", "Q", "K", "A"
};

static const char *SUIT_STR[4] = {"♥", "♦", "♣", "♠"};
static const int SUIT_RED[4] = {1, 1, 0, 0};

/* -- Widget references ----------------------------------------------------- */
typedef struct {
    /* login screen */
    GtkWidget *login_screen;
    GtkWidget *game_screen;
    GtkWidget *window;
    GtkWidget *stack;
    GtkWidget *name_entry;
    GtkWidget *host_entry;
    GtkWidget *port_entry;
    GtkWidget *login_status;

    /* chat screen */
    GtkWidget *chat_log;
    GtkWidget *chat_input;

    /* game screen */
    GtkWidget *pot_label;
    GtkWidget *stage_label;
    GtkWidget *log_label;
    GtkWidget *community_cards[5];
    GtkWidget *my_cards[2];
    GtkWidget *deck_count_label;

    /* opponent seats: top-left, top-center, top-right */
    GtkWidget *opp_name[3];
    GtkWidget *opp_chips[3];
    GtkWidget *opp_status[3];
    GtkWidget *opp_frame[3];

    /* action buttons */
    GtkWidget *btn_fold;
    GtkWidget *btn_check;
    GtkWidget *btn_call;
    GtkWidget *btn_raise;
    GtkWidget *raise_spin;

    guint net_timer;
} AppWidgets;

typedef struct {
    Card card;
    int face_up;
} CardDrawData;

static AppWidgets W;
static ClientState C;

/* --------------------------------------------------------------------------
   CSS
   -------------------------------------------------------------------------- */
static const char *CSS =
/* window / global */
"window { background-color: #0d1117; }"
"label  { color: #e6edf3; font-family: 'Georgia', serif; }"

/* login card */
"#login-card {"
"  background-color: #161b22;"
"  border-radius: 16px;"
"  border: 1px solid #30363d;"
"  padding: 40px;"
"}"
"#login-title {"
"  font-size: 30px; font-weight: bold;"
"  color: #e6c87a; letter-spacing: 4px;"
"}"
"#login-sub   { font-size: 13px; color: #8b949e; letter-spacing: 2px; }"
"#login-suits { font-size: 20px; color: #e6c87a; }"
"label.field-lbl { font-size: 11px; color: #8b949e; letter-spacing: 1px; }"
"entry {"
"  background-color: #0d1117; color: #e6edf3;"
"  border: 1px solid #30363d; border-radius: 8px;"
"  padding: 10px 14px; font-family: 'Georgia', serif; font-size: 14px;"
"}"
"entry:focus { border-color: #e6c87a; }"
"#connect-btn {"
"  background-color: #e6c87a; color: #0d1117; border: none;"
"  border-radius: 8px; padding: 12px 0;"
"  font-family: 'Georgia', serif; font-size: 15px; font-weight: bold;"
"  letter-spacing: 2px;"
"}"
"#connect-btn:hover  { background-color: #f0d898; }"
"#connect-btn:active { background-color: #c9a84c; }"
"#login-status { font-size: 12px; color: #f85149; }"
"#offline-btn {"
"  background-color: #e6c87a; color: #0d1117;"
"  border: 1px solid #30363d; border-radius: 8px; padding: 10px 0;"
"  font-family: 'Georgia', serif; font-size: 13px; letter-spacing: 1px;"
"}"
"#offline-btn:hover { background-color: #f0d898; color: #0d1117; border-color: #8b949e; }"
"#divider-label { font-size: 11px; color: #30363d; letter-spacing: 3px; }"

/* game screen */
"#game-root { background-color: #0d1117; }"
"#felt {"
"  background-color: #1a5c35;"
"  border: 6px solid #5c3210;"
"  border-radius: 120px;"
"}"
"#pot-label   { font-size: 15px; color: #ffd700; font-weight: bold; }"
"#stage-label { font-size: 12px; color: #a0c8a0; letter-spacing: 2px; }"
"#log-label   { font-size: 12px; color: #7ab870; }"

/* opponent frames */
".opp-frame {"
"  background-color: rgba(0,0,0,0.45);"
"  border: 1px solid #2d5a3d;"
"  border-radius: 10px;"
"  padding: 6px 10px;"
"}"
".opp-frame.active-seat {"
"  border-color: #ffd700;"
"  background-color: rgba(255,215,0,0.08);"
"}"
".opp-name   { font-size: 13px; font-weight: bold; color: #e0f0e0; }"
".opp-chips  { font-size: 11px; color: #a0c8a0; }"
".opp-status { font-size: 10px; color: #ffd700; }"

/* action buttons */
".action-btn {"
"  border-radius: 8px; font-family: 'Georgia', serif;"
"  font-size: 13px; font-weight: bold; letter-spacing: 1px;"
"  padding: 10px 18px; border: 1px solid;"
"}"
"#btn-fold  { background-color: #3a1a1a; border-color: #c0392b; color: #e87a7a; }"
"#btn-check { background-color: #1a2a3a; border-color: #2980b9; color: #7ab8e8; }"
"#btn-call  { background-color: #1a3a1a; border-color: #27ae60; color: #7ae890; }"
"#btn-raise { background-color: #3a3a1a; border-color: #f39c12; color: #f0c050; }"
".action-btn:disabled { opacity: 0.3; }";

void appendChat(const char *sender, const char *chatMessage)
{
    GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(W.chat_log));
    GtkTextIter last;

    gtk_text_buffer_get_end_iter(buf, &last);

    // "Name : "
    gtk_text_buffer_insert(buf, &last, sender, -1);
    gtk_text_buffer_insert(buf, &last, " : ", -1);

    //add message after "Name : "
    gtk_text_buffer_insert(buf, &last, message, -1);
    gtk_text_buffer_insert(buf, &last, "\n", -1);
}

static int card_is_known(Card card)
{
    return card.rank != UNKNOW_R && card.suit != UNKNOW_S;
}

static int rank_index(uint8_t rank)
{
    if (rank < TWO || rank > ACE) return 0;
    return rank - TWO;
}

static const char *suit_label(uint8_t suit)
{
    if (suit < HEARTS || suit > SPADES) return "?";
    return SUIT_STR[suit - HEARTS];
}

static int suit_is_red(uint8_t suit)
{
    if (suit < HEARTS || suit > SPADES) return 0;
    return SUIT_RED[suit - HEARTS];
}

/* -- Drawing: a single card ------------------------------------------------ */
static gboolean draw_card_cb(GtkWidget *widget, cairo_t *cr, gpointer data)
{
    CardDrawData *d = (CardDrawData *)data;
    int w = gtk_widget_get_allocated_width(widget);
    int h = gtk_widget_get_allocated_height(widget);

    /* card background */
    if (d->face_up)
        cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    else
        cairo_set_source_rgb(cr, 0.1, 0.18, 0.5);

    double r = 5.0;
    cairo_move_to(cr, r, 0);
    cairo_line_to(cr, w - r, 0); cairo_arc(cr, w - r, r, r, -G_PI / 2, 0);
    cairo_line_to(cr, w, h - r); cairo_arc(cr, w - r, h - r, r, 0, G_PI / 2);
    cairo_line_to(cr, r, h); cairo_arc(cr, r, h - r, r, G_PI / 2, G_PI);
    cairo_line_to(cr, 0, r); cairo_arc(cr, r, r, r, G_PI, -G_PI / 2);
    cairo_close_path(cr);
    cairo_fill_preserve(cr);

    cairo_set_source_rgb(cr, 0.6, 0.6, 0.6);
    cairo_set_line_width(cr, 0.8);
    cairo_stroke(cr);

    if (!d->face_up || !card_is_known(d->card)) return FALSE;

    /* rank + suit text */
    if (suit_is_red(d->card.suit))
        cairo_set_source_rgb(cr, 0.80, 0.15, 0.15);
    else
        cairo_set_source_rgb(cr, 0.10, 0.10, 0.10);

    cairo_select_font_face(cr, "Georgia", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, w < 36 ? 9 : 12);
    cairo_move_to(cr, 4, 14);
    cairo_show_text(cr, RANK_STR[rank_index(d->card.rank)]);

    cairo_set_font_size(cr, w < 36 ? 10 : 14);
    cairo_move_to(cr, 4, h - 4);
    cairo_show_text(cr, suit_label(d->card.suit));

    return FALSE;
}

/* helper to create a card drawing area */
static GtkWidget *make_card_widget(int w, int h)
{
    GtkWidget *da = gtk_drawing_area_new();
    gtk_widget_set_size_request(da, w, h);
    CardDrawData *d = g_new0(CardDrawData, 1);
    g_object_set_data_full(G_OBJECT(da), "card-data", d, g_free);
    g_signal_connect(da, "draw", G_CALLBACK(draw_card_cb), d);
    return da;
}

static void set_card_face(GtkWidget *da, Card c, int face_up)
{
    CardDrawData *d = g_object_get_data(G_OBJECT(da), "card-data");
    if (!d) return;
    d->card = c;
    d->face_up = face_up;
    gtk_widget_queue_draw(da);
}

static void set_card_back(GtkWidget *da)
{
    CardDrawData *d = g_object_get_data(G_OBJECT(da), "card-data");
    if (!d) return;
    d->face_up = 0;
    gtk_widget_queue_draw(da);
}

static void infer_my_player_id(void)
{
    for (int i = 0; i < MAX_PLAYERS; i++) {
        Player *p = &C.game.players[i];
        if (p->has_cards && card_is_known(p->hand[0])) {
            C.my_player_id = i;
            return;
        }
    }
}

/* -- UI refresh ------------------------------------------------------------ */
static void refresh_ui(void)
{
    GameState *game = &C.game;
    char buf[128];

    infer_my_player_id();

    /* pot / stage */
    snprintf(buf, sizeof(buf), "Pot:  $%u", game->pot);
    gtk_label_set_text(GTK_LABEL(W.pot_label), buf);

    static const char *stage_names[] = {"PREFLOP", "FLOP", "TURN", "RIVER"};
    gtk_label_set_text(GTK_LABEL(W.stage_label),
                       game->stage <= RIVER ? stage_names[game->stage] : "WAITING");

    snprintf(buf, sizeof(buf), "Players: %u", game->playerCount);
    gtk_label_set_text(GTK_LABEL(W.deck_count_label), buf);

    /* community cards */
    for (int i = 0; i < 5; i++) {
        if (i < game->communityCount)
            set_card_face(W.community_cards[i], game->community[i], 1);
        else
            set_card_back(W.community_cards[i]);
    }

    /* my hand */
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
    for (int pi = 0; pi < MAX_PLAYERS && opp_slot < EXT_OPP_SEATS; pi++) {
        if (pi == C.my_player_id) continue;
        Player *p = &game->players[pi];
        if (p->status == PLAYER_EMPTY) continue;

        gtk_label_set_text(GTK_LABEL(EXT.opp_name[opp_slot]), p->name[0] ? p->name : "Player");
        snprintf(buf, sizeof(buf), "$%u  |  bet $%u", p->chips, p->current_bet);
        gtk_label_set_text(GTK_LABEL(EXT.opp_chips[opp_slot]), buf);

        const char *st = "Waiting";
        if (p->status == PLAYER_FOLDED) st = "Folded";
        else if (p->status == PLAYER_ALL_IN) st = "All in";
        else if (p->status == PLAYER_DISCONNECTED) st = "Disconnected";
        else if (p->status == PLAYER_SPECTATING) st = "Spectating";
        else if (pi == game->currentPlayer && game->handPlaying) st = "Acting";
        gtk_label_set_text(GTK_LABEL(EXT.opp_status[opp_slot]), st);

        GtkStyleContext *ctx = gtk_widget_get_style_context(EXT.opp_frame[opp_slot]);
        if (pi == game->currentPlayer && game->handPlaying)
            gtk_style_context_add_class(ctx, "active-seat");
        else
            gtk_style_context_remove_class(ctx, "active-seat");
        opp_slot++;
    }

    for (int i = opp_slot; i < EXT_OPP_SEATS; i++) {
        gtk_label_set_text(GTK_LABEL(EXT.opp_name[i]), "Empty");
        gtk_label_set_text(GTK_LABEL(EXT.opp_chips[i]), "$0  |  bet $0");
        gtk_label_set_text(GTK_LABEL(EXT.opp_status[i]), "Waiting");
        gtk_style_context_remove_class(gtk_widget_get_style_context(EXT.opp_frame[i]), "active-seat");
    }

    /* action buttons: only enabled on my turn */
    int my_turn = C.connected && game->handPlaying && game->currentPlayer == C.my_player_id;
    int can_check = me->current_bet >= game->currentBet;

    if (C.connected && game->handPlaying)
        snprintf(buf, sizeof(buf), my_turn ? "Your turn" : "Waiting for player %u", game->currentPlayer);
    else if (C.connected)
        snprintf(buf, sizeof(buf), "Connected. Waiting for hand.");
    else
        snprintf(buf, sizeof(buf), "Not connected.");
    gtk_label_set_text(GTK_LABEL(W.log_label), buf);

    gtk_widget_set_sensitive(W.btn_fold, my_turn);
    gtk_widget_set_sensitive(W.btn_check, my_turn && can_check);
    gtk_widget_set_sensitive(W.btn_call, my_turn && !can_check);
    gtk_widget_set_sensitive(W.btn_raise, my_turn);
    gtk_widget_set_sensitive(W.raise_spin, my_turn);
}

/* -- Send moves to the server --------------------------------------------- */
static void send_gui_move(MoveType move, uint32_t amount)
{
    if (!C.connected) return;

    PlayerAction action;
    action.playerID = C.my_player_id;
    action.move = move;
    action.amount = amount;
    send_action(&C, &action);
}

/* -- Action button callbacks ---------------------------------------------- */
static void on_fold(GtkButton *b, gpointer d)  { (void)b; (void)d; send_gui_move(FOLD, 0); }
static void on_check(GtkButton *b, gpointer d) { (void)b; (void)d; send_gui_move(CHECK, 0); }
static void on_call(GtkButton *b, gpointer d)  { (void)b; (void)d; send_gui_move(CALL, 0); }
static void on_raise(GtkButton *b, gpointer d)
{
    (void)b; (void)d;
    uint32_t amount = (uint32_t)gtk_spin_button_get_value(GTK_SPIN_BUTTON(W.raise_spin));
    send_gui_move(RAISE, amount);
}

/* -- Switch to game screen ------------------------------------------------- */
static void show_game_screen(void)
{
    gtk_stack_set_visible_child_name(GTK_STACK(W.stack), "game");
    gtk_widget_show_all(W.game_screen);
    gtk_window_resize(GTK_WINDOW(W.window), 860, 620);
    refresh_ui();
}

/* -- Poll server without blocking GTK ------------------------------------- */
static gboolean poll_server_cb(gpointer data)
{
    (void)data;
    if (!C.connected) return FALSE;

    fd_set read_fds;
    struct timeval timeout = {0, 0};
    FD_ZERO(&read_fds);
    FD_SET(C.socket_fd, &read_fds);

    int ready = select(C.socket_fd + 1, &read_fds, NULL, NULL, &timeout);
    if (ready > 0 && FD_ISSET(C.socket_fd, &read_fds)) {
        handle_server_communication(&C);
        refresh_ui();
    }

    return C.connected;
}

/* -- Play vs bots callback ------------------------------------------------- */
static void on_play_clicked(GtkButton *btn, gpointer user_data)
{
    (void)btn;
    (void)user_data;
    gtk_label_set_text(GTK_LABEL(W.login_status), "Offline mode is disabled for server testing.");
}

/* -- Connect to server callback ------------------------------------------- */
static void on_connect_clicked(GtkButton *btn, gpointer user_data)
{
    (void)btn;
    (void)user_data;

    const char *name = gtk_entry_get_text(GTK_ENTRY(W.name_entry));
    const char *host = gtk_entry_get_text(GTK_ENTRY(W.host_entry));
    const char *port = gtk_entry_get_text(GTK_ENTRY(W.port_entry));

    if (C.connected) {
        gtk_label_set_text(GTK_LABEL(W.login_status), "Already connected.");
        show_game_screen();
        return;
    }

    if (strlen(name) == 0) { gtk_label_set_text(GTK_LABEL(W.login_status), "Enter your name."); return; }
    if (strlen(host) == 0) { gtk_label_set_text(GTK_LABEL(W.login_status), "Enter a host."); return; }
    if (strlen(port) == 0) { gtk_label_set_text(GTK_LABEL(W.login_status), "Enter a port."); return; }

    init_client_state(&C);
    int sockfd = connect_to_server(host, atoi(port));
    if (sockfd < 0) {
        gtk_label_set_text(GTK_LABEL(W.login_status), "Could not connect to server.");
        return;
    }

    C.socket_fd = sockfd;
    C.connected = 1;
    C.my_player_id = 0;

    if (W.net_timer)
        g_source_remove(W.net_timer);
    W.net_timer = g_timeout_add(100, poll_server_cb, NULL);

    gtk_label_set_text(GTK_LABEL(W.login_status), "");
    show_game_screen();
}

/* -- Build the game screen ------------------------------------------------- */
static GtkWidget *build_game_screen(void)
{
    /* root overlay lets us position felt freely */
    GtkWidget *root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_name(root, "game-root");

    /* -- top bar -- */
    GtkWidget *topbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 16);
    gtk_widget_set_margin_start(topbar, 20);
    gtk_widget_set_margin_end(topbar, 20);
    gtk_widget_set_margin_top(topbar, 10);
    gtk_box_pack_start(GTK_BOX(root), topbar, FALSE, FALSE, 0);

    GtkWidget *title_lbl = gtk_label_new("ANTEATER POKER");
    gtk_widget_set_name(title_lbl, "login-title");
    gtk_box_pack_start(GTK_BOX(topbar), title_lbl, FALSE, FALSE, 0);

    W.deck_count_label = gtk_label_new("Players: 0");
    gtk_box_pack_end(GTK_BOX(topbar), W.deck_count_label, FALSE, FALSE, 0);

    /* -- main area: opponents + felt + hand -- */
    GtkWidget *main_area = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_widget_set_margin_start(main_area, 20);
    gtk_widget_set_margin_end(main_area, 20);
    gtk_box_pack_start(GTK_BOX(root), main_area, TRUE, TRUE, 0);

    /* --- opponent row (6 seats across the top) --- */
    GtkWidget *opp_row = build_six_seat_row(&EXT);
    gtk_box_pack_start(GTK_BOX(main_area), opp_row, FALSE, FALSE, 4);
    }

    /* --- felt table area --- */
    GtkWidget *felt = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_widget_set_name(felt, "felt");
    gtk_widget_set_margin_start(felt, 40);
    gtk_widget_set_margin_end(felt, 40);
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

    /* community card row */
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

    /* --- my hand area --- */
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

    /* --- action buttons --- */
    GtkWidget *btn_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_halign(btn_row, GTK_ALIGN_CENTER);
    gtk_widget_set_margin_bottom(btn_row, 12);
    gtk_box_pack_start(GTK_BOX(main_area), btn_row, FALSE, FALSE, 4);

    W.btn_fold = gtk_button_new_with_label("Fold");
    W.btn_check = gtk_button_new_with_label("Check");
    W.btn_call = gtk_button_new_with_label("Call");
    W.btn_raise = gtk_button_new_with_label("Raise");

    gtk_widget_set_name(W.btn_fold, "btn-fold");
    gtk_widget_set_name(W.btn_check, "btn-check");
    gtk_widget_set_name(W.btn_call, "btn-call");
    gtk_widget_set_name(W.btn_raise, "btn-raise");

    gtk_style_context_add_class(gtk_widget_get_style_context(W.btn_fold), "action-btn");
    gtk_style_context_add_class(gtk_widget_get_style_context(W.btn_check), "action-btn");
    gtk_style_context_add_class(gtk_widget_get_style_context(W.btn_call), "action-btn");
    gtk_style_context_add_class(gtk_widget_get_style_context(W.btn_raise), "action-btn");

    g_signal_connect(W.btn_fold, "clicked", G_CALLBACK(on_fold), NULL);
    g_signal_connect(W.btn_check, "clicked", G_CALLBACK(on_check), NULL);
    g_signal_connect(W.btn_call, "clicked", G_CALLBACK(on_call), NULL);
    g_signal_connect(W.btn_raise, "clicked", G_CALLBACK(on_raise), NULL);

    /* raise spin button */
    GtkAdjustment *adj = gtk_adjustment_new(50, 10, 10000, 10, 50, 0);
    W.raise_spin = gtk_spin_button_new(adj, 10, 0);
    gtk_widget_set_size_request(W.raise_spin, 80, -1);

    gtk_box_pack_start(GTK_BOX(btn_row), W.btn_fold, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(btn_row), W.btn_check, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(btn_row), W.btn_call, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(btn_row), W.raise_spin, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(btn_row), W.btn_raise, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(btn_row), build_shop_button(&EXT), FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(root), build_chat_box(&EXT), FALSE, FALSE, 0);
    refresh_ui();
    return root;
}

/* -- Build the login screen ------------------------------------------------ */
static GtkWidget *build_login_screen(void)
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

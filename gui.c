#include <gtk/gtk.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

/* ── Suits / Ranks ──────────────────────────────────────────────────────────── */
static const char *SUIT_SYM[4]  = {"♠","♥","♦","♣"};
static const int   SUIT_RED[4]  = {0, 1, 1, 0};
static const char *RANK_STR[13] = {"2","3","4","5","6","7","8","9","10","J","Q","K","A"};

typedef struct { int suit; int rank; } Card;

/* ── Game state ─────────────────────────────────────────────────────────────── */
#define MAX_PLAYERS 4
#define DECK_SIZE   52

typedef enum { STATUS_ACTIVE, STATUS_FOLDED, STATUS_ALLIN } PStatus;

typedef struct {
    char    name[32];
    int     chips;
    int     bet;
    PStatus status;
    Card    hand[2];
    int     is_bot;
} Player;

typedef enum { STAGE_PREFLOP, STAGE_FLOP, STAGE_TURN, STAGE_RIVER, STAGE_SHOWDOWN } Stage;

typedef struct {
    Card    deck[DECK_SIZE];
    int     deck_top;
    Card    community[5];
    int     revealed;
    Stage   stage;
    int     pot;
    int     current_bet;
    Player  players[MAX_PLAYERS];
    int     active_idx;   /* whose turn */
    int     my_idx;       /* local player index */
    int     game_over;
    int     num_players;
} GameState;

static GameState G;

/* ── Widget references ──────────────────────────────────────────────────────── */
typedef struct {
    /* login screen */
    GtkWidget *login_screen;
    GtkWidget *game_screen;
    GtkWidget *window;
    GtkWidget *name_entry;
    GtkWidget *host_entry;
    GtkWidget *port_entry;
    GtkWidget *login_status;

    /* game screen */
    GtkWidget *pot_label;
    GtkWidget *stage_label;
    GtkWidget *log_label;
    GtkWidget *community_cards[5];   /* GtkDrawingArea */
    GtkWidget *my_cards[2];
    GtkWidget *deck_count_label;

    /* opponent seats: top-left, top-center, top-right  (indices 1,2,3) */
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

    guint bot_timer;
} AppWidgets;

static AppWidgets W;

/* ────────────────────────────────────────────────────────────────────────────
   CSS
   ──────────────────────────────────────────────────────────────────────────── */
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
".action-btn:disabled { opacity: 0.3; }"
;

/* ── Deck helpers ───────────────────────────────────────────────────────────── */
static void shuffle_deck(void)
{
    for (int i = 0; i < DECK_SIZE; i++) {
        G.deck[i].suit = i / 13;
        G.deck[i].rank = i % 13;
    }
    for (int i = DECK_SIZE - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        Card tmp = G.deck[i]; G.deck[i] = G.deck[j]; G.deck[j] = tmp;
    }
    G.deck_top = 0;
}

static Card draw_card(void) { return G.deck[G.deck_top++]; }

/* ── Drawing: a single card ─────────────────────────────────────────────────── */
typedef struct { Card card; int face_up; int w; int h; } CardDrawData;

static gboolean draw_card_cb(GtkWidget *widget, cairo_t *cr, gpointer data)
{
    CardDrawData *d = (CardDrawData *)data;
    int w = gtk_widget_get_allocated_width(widget);
    int h = gtk_widget_get_allocated_height(widget);

    /* card background */
    if (d->face_up) {
        cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    } else {
        /* back: dark blue hatched */
        cairo_set_source_rgb(cr, 0.1, 0.18, 0.5);
    }
    double r = 5.0;
    cairo_move_to(cr, r, 0);
    cairo_line_to(cr, w-r, 0); cairo_arc(cr, w-r, r, r, -G_PI/2, 0);
    cairo_line_to(cr, w, h-r); cairo_arc(cr, w-r, h-r, r, 0, G_PI/2);
    cairo_line_to(cr, r, h);   cairo_arc(cr, r, h-r, r, G_PI/2, G_PI);
    cairo_line_to(cr, 0, r);   cairo_arc(cr, r, r, r, G_PI, -G_PI/2);
    cairo_close_path(cr);
    cairo_fill_preserve(cr);

    cairo_set_source_rgb(cr, 0.6, 0.6, 0.6);
    cairo_set_line_width(cr, 0.8);
    cairo_stroke(cr);

    if (!d->face_up) return FALSE;

    /* rank + suit text */
    int red = SUIT_RED[d->card.suit];
    if (red) cairo_set_source_rgb(cr, 0.80, 0.15, 0.15);
    else     cairo_set_source_rgb(cr, 0.10, 0.10, 0.10);

    cairo_select_font_face(cr, "Georgia", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, w < 36 ? 9 : 12);

    char label[8];
    snprintf(label, sizeof(label), "%s", RANK_STR[d->card.rank]);
    cairo_move_to(cr, 4, 14);
    cairo_show_text(cr, label);

    cairo_set_font_size(cr, w < 36 ? 10 : 14);
    cairo_move_to(cr, 4, h - 4);
    cairo_show_text(cr, SUIT_SYM[d->card.suit]);

    return FALSE;
}

/* helper to create a card drawing area */
static GtkWidget *make_card_widget(int w, int h)
{
    GtkWidget *da = gtk_drawing_area_new();
    gtk_widget_set_size_request(da, w, h);
    CardDrawData *d = g_new0(CardDrawData, 1);
    d->face_up = 0;
    g_object_set_data_full(G_OBJECT(da), "card-data", d, g_free);
    g_signal_connect(da, "draw", G_CALLBACK(draw_card_cb), d);
    return da;
}

static void set_card_face(GtkWidget *da, Card c, int face_up)
{
    CardDrawData *d = g_object_get_data(G_OBJECT(da), "card-data");
    if (!d) return;
    d->card    = c;
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

/* ── UI refresh ─────────────────────────────────────────────────────────────── */
static void refresh_ui(void)
{
    /* pot / stage */
    char buf[128];
    snprintf(buf, sizeof(buf), "Pot:  $%d", G.pot);
    gtk_label_set_text(GTK_LABEL(W.pot_label), buf);

    static const char *stage_names[] = {"PREFLOP","FLOP","TURN","RIVER","SHOWDOWN"};
    gtk_label_set_text(GTK_LABEL(W.stage_label), stage_names[G.stage]);

    snprintf(buf, sizeof(buf), "Deck: %d", DECK_SIZE - G.deck_top);
    gtk_label_set_text(GTK_LABEL(W.deck_count_label), buf);

    /* community cards */
    for (int i = 0; i < 5; i++) {
        if (i < G.revealed)
            set_card_face(W.community_cards[i], G.community[i], 1);
        else
            set_card_back(W.community_cards[i]);
    }

    /* my hand */
    set_card_face(W.my_cards[0], G.players[G.my_idx].hand[0], 1);
    set_card_face(W.my_cards[1], G.players[G.my_idx].hand[1], 1);

    /* opponent seats (players 1,2,3 → opp slots 0,1,2) */
    for (int i = 0; i < 3; i++) {
        int pi = i + 1;
        Player *p = &G.players[pi];

        gtk_label_set_text(GTK_LABEL(W.opp_name[i]),  p->name);

        snprintf(buf, sizeof(buf), "$%d  |  bet $%d", p->chips, p->bet);
        gtk_label_set_text(GTK_LABEL(W.opp_chips[i]), buf);

        const char *st = "";
        if (p->status == STATUS_FOLDED) st = "Folded";
        else if (pi == G.active_idx && !G.game_over) st = "▶ Acting";
        else st = "Waiting";
        gtk_label_set_text(GTK_LABEL(W.opp_status[i]), st);

        /* highlight active seat */
        GtkStyleContext *ctx = gtk_widget_get_style_context(W.opp_frame[i]);
        if (pi == G.active_idx && !G.game_over)
            gtk_style_context_add_class(ctx, "active-seat");
        else
            gtk_style_context_remove_class(ctx, "active-seat");
    }

    /* action buttons: only enabled on my turn */
    int my_turn = (G.active_idx == G.my_idx) && !G.game_over;
    Player *me  = &G.players[G.my_idx];
    int can_check = (G.current_bet <= me->bet);

    gtk_widget_set_sensitive(W.btn_fold,  my_turn);
    gtk_widget_set_sensitive(W.btn_check, my_turn && can_check);
    gtk_widget_set_sensitive(W.btn_call,  my_turn && !can_check);
    gtk_widget_set_sensitive(W.btn_raise, my_turn);
    gtk_widget_set_sensitive(W.raise_spin, my_turn);
}

/* ── Round logic ────────────────────────────────────────────────────────────── */
static int next_active(int from)
{
    int idx = (from + 1) % G.num_players;
    for (int t = 0; t < G.num_players; t++) {
        if (G.players[idx].status == STATUS_ACTIVE) return idx;
        idx = (idx + 1) % G.num_players;
    }
    return from;
}

static int active_count(void)
{
    int n = 0;
    for (int i = 0; i < G.num_players; i++)
        if (G.players[i].status == STATUS_ACTIVE) n++;
    return n;
}

static int all_bets_match(void)
{
    for (int i = 0; i < G.num_players; i++)
        if (G.players[i].status == STATUS_ACTIVE && G.players[i].bet != G.current_bet)
            return 0;
    return 1;
}

static void end_hand(void)
{
    G.game_over = 1;
    G.revealed  = 5;

    /* pick a random winner from non-folded players (real eval goes here) */
    int pool[MAX_PLAYERS], n = 0;
    for (int i = 0; i < G.num_players; i++)
        if (G.players[i].status != STATUS_FOLDED) pool[n++] = i;
    int winner = (n > 0) ? pool[rand() % n] : 0;
    G.players[winner].chips += G.pot;

    char msg[128];
    snprintf(msg, sizeof(msg), "  %s wins $%d!  (New hand auto-deals in 3s)",
             G.players[winner].name, G.pot);
    gtk_label_set_text(GTK_LABEL(W.log_label), msg);
    refresh_ui();
}

static void advance_stage(void)
{
    G.stage++;
    for (int i = 0; i < G.num_players; i++)
        if (G.players[i].status == STATUS_ACTIVE) G.players[i].bet = 0;
    G.current_bet = 0;

    if (G.stage == STAGE_FLOP)     G.revealed = 3;
    if (G.stage == STAGE_TURN)     G.revealed = 4;
    if (G.stage == STAGE_RIVER)    G.revealed = 5;
    if (G.stage == STAGE_SHOWDOWN) { end_hand(); return; }

    G.active_idx = 0;
    static const char *sn[] = {"Preflop","Flop","Turn","River","Showdown"};
    char buf[64]; snprintf(buf, sizeof(buf), "  --- %s ---", sn[G.stage]);
    gtk_label_set_text(GTK_LABEL(W.log_label), buf);
    refresh_ui();
}

static int check_round_over(void)
{
    if (active_count() <= 1) { end_hand(); return 1; }
    if (all_bets_match())    { advance_stage(); return 1; }
    return 0;
}

/* forward decl */
static gboolean bot_turn_cb(gpointer data);

static void schedule_bot(void)
{
    if (W.bot_timer) g_source_remove(W.bot_timer);
    W.bot_timer = g_timeout_add(800, bot_turn_cb, NULL);
}

static void apply_action(int pi, const char *action, int amount)
{
    Player *p = &G.players[pi];
    char buf[128];

    if (strcmp(action, "fold") == 0) {
        p->status = STATUS_FOLDED;
        snprintf(buf, sizeof(buf), "  %s folded.", p->name);
    } else if (strcmp(action, "check") == 0) {
        snprintf(buf, sizeof(buf), "  %s checked.", p->name);
    } else if (strcmp(action, "call") == 0) {
        int diff = G.current_bet - p->bet;
        int paid = diff < p->chips ? diff : p->chips;
        p->chips -= paid; G.pot += paid; p->bet += paid;
        snprintf(buf, sizeof(buf), "  %s called $%d.", p->name, paid);
    } else { /* raise */
        int diff = amount - p->bet;
        int paid = diff < p->chips ? diff : p->chips;
        p->chips -= paid; G.pot += paid; p->bet += paid;
        G.current_bet = p->bet;
        snprintf(buf, sizeof(buf), "  %s raised to $%d.", p->name, p->bet);
    }
    gtk_label_set_text(GTK_LABEL(W.log_label), buf);
}

/* ── Bot AI (simple random) ─────────────────────────────────────────────────── */
static gboolean bot_turn_cb(gpointer data)
{
    (void)data;
    W.bot_timer = 0;
    if (G.game_over) return FALSE;

    Player *p = &G.players[G.active_idx];
    if (!p->is_bot || p->status == STATUS_FOLDED) return FALSE;

    int diff = G.current_bet - p->bet;
    double r  = (double)rand() / RAND_MAX;

    if (diff == 0) {
        if (r < 0.3) apply_action(G.active_idx, "raise", G.current_bet + 20);
        else         apply_action(G.active_idx, "check", 0);
    } else {
        if      (r < 0.2) apply_action(G.active_idx, "fold",  0);
        else if (r < 0.6) apply_action(G.active_idx, "call",  0);
        else              apply_action(G.active_idx, "raise", G.current_bet + 20);
    }

    if (check_round_over()) return FALSE;
    G.active_idx = next_active(G.active_idx);
    refresh_ui();
    if (G.players[G.active_idx].is_bot) schedule_bot();
    return FALSE;
}

/* ── Deal a new hand ────────────────────────────────────────────────────────── */
static gboolean new_hand_cb(gpointer data);

static void new_hand(void)
{
    srand((unsigned)time(NULL));
    shuffle_deck();

    G.revealed     = 0;
    G.stage        = STAGE_PREFLOP;
    G.pot          = 30;   /* blinds */
    G.current_bet  = 20;
    G.game_over    = 0;
    G.num_players  = 4;
    G.my_idx       = 0;

    /* community */
    for (int i = 0; i < 5; i++) G.community[i] = draw_card();

    /* deal players */
    for (int i = 0; i < G.num_players; i++) {
        G.players[i].chips  = (i == 0) ? 1000 : 980 + i * 10;
        G.players[i].bet    = (i == 0) ? 10 : (i == 1) ? 20 : 0;
        G.players[i].status = STATUS_ACTIVE;
        G.players[i].is_bot = (i != 0);
        G.players[i].hand[0] = draw_card();
        G.players[i].hand[1] = draw_card();
    }
    /* player name was set at login; bots get defaults */
    snprintf(G.players[1].name, 32, "Bot 2");
    snprintf(G.players[2].name, 32, "Bot 3");
    snprintf(G.players[3].name, 32, "Bot 4");

    G.active_idx = 2; /* UTG after blinds */

    gtk_label_set_text(GTK_LABEL(W.log_label),
        "  New hand. Blinds posted ($10/$20). Bot 3 to act.");
    refresh_ui();
    schedule_bot();
}

/* auto-deal callback after hand ends */
static gboolean new_hand_cb(gpointer data)
{
    (void)data;
    new_hand();
    return FALSE;
}

static void schedule_new_hand(void)
{
    g_timeout_add(3000, new_hand_cb, NULL);
}

/* ── Action button callbacks ────────────────────────────────────────────────── */
static void on_fold(GtkButton *b, gpointer d)  { (void)b;(void)d;
    if (G.game_over || G.active_idx != G.my_idx) return;
    apply_action(G.my_idx, "fold", 0);
    if (check_round_over()) { schedule_new_hand(); return; }
    G.active_idx = next_active(G.my_idx);
    refresh_ui();
    if (G.players[G.active_idx].is_bot) schedule_bot();
}
static void on_check(GtkButton *b, gpointer d) { (void)b;(void)d;
    if (G.game_over || G.active_idx != G.my_idx) return;
    if (G.current_bet > G.players[G.my_idx].bet) return;
    apply_action(G.my_idx, "check", 0);
    if (check_round_over()) { schedule_new_hand(); return; }
    G.active_idx = next_active(G.my_idx);
    refresh_ui();
    if (G.players[G.active_idx].is_bot) schedule_bot();
}
static void on_call(GtkButton *b, gpointer d)  { (void)b;(void)d;
    if (G.game_over || G.active_idx != G.my_idx) return;
    apply_action(G.my_idx, "call", 0);
    if (check_round_over()) { schedule_new_hand(); return; }
    G.active_idx = next_active(G.my_idx);
    refresh_ui();
    if (G.players[G.active_idx].is_bot) schedule_bot();
}
static void on_raise(GtkButton *b, gpointer d) { (void)b;(void)d;
    if (G.game_over || G.active_idx != G.my_idx) return;
    int amount = (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(W.raise_spin));
    if (amount <= G.current_bet) return;
    apply_action(G.my_idx, "raise", amount);
    if (check_round_over()) { schedule_new_hand(); return; }
    G.active_idx = next_active(G.my_idx);
    refresh_ui();
    if (G.players[G.active_idx].is_bot) schedule_bot();
}

/* ── Switch to game screen ──────────────────────────────────────────────────── */
static void show_game_screen(const char *player_name)
{
    snprintf(G.players[0].name, 32, "%s", player_name);
    gtk_widget_hide(W.login_screen);
    gtk_widget_show_all(W.game_screen);
    gtk_window_resize(GTK_WINDOW(W.window), 860, 620);
    new_hand();
}

/* ── Play vs bots callback ──────────────────────────────────────────────────── */
static void on_play_clicked(GtkButton *btn, gpointer user_data)
{
    (void)btn; (void)user_data;
    const char *name = gtk_entry_get_text(GTK_ENTRY(W.name_entry));
    if (strlen(name) == 0) {
        gtk_label_set_text(GTK_LABEL(W.login_status), "⚠  Enter your name first.");
        return;
    }
    gtk_label_set_text(GTK_LABEL(W.login_status), "");
    show_game_screen(name);
}

/* ── Connect to server callback ─────────────────────────────────────────────── */
static void on_connect_clicked(GtkButton *btn, gpointer user_data)
{
    (void)btn; (void)user_data;
    const char *name = gtk_entry_get_text(GTK_ENTRY(W.name_entry));
    const char *host = gtk_entry_get_text(GTK_ENTRY(W.host_entry));
    const char *port = gtk_entry_get_text(GTK_ENTRY(W.port_entry));

    if (strlen(name) == 0) { gtk_label_set_text(GTK_LABEL(W.login_status), "⚠  Enter your name."); return; }
    if (strlen(host) == 0) { gtk_label_set_text(GTK_LABEL(W.login_status), "⚠  Enter a host."); return; }
    if (strlen(port) == 0) { gtk_label_set_text(GTK_LABEL(W.login_status), "⚠  Enter a port."); return; }

    char msg[128];
    snprintf(msg, sizeof(msg), "Connecting to %s:%s â¦", host, port);
    gtk_label_set_text(GTK_LABEL(W.login_status), msg);

    /* TODO: replace show_game_screen with connect_to_server(host, atoi(port)) */
    show_game_screen(name);
}

/* ── Build the game screen ──────────────────────────────────────────────────── */
static GtkWidget *build_game_screen(void)
{
    /* root overlay lets us position felt freely */
    GtkWidget *root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_name(root, "game-root");

    /* ── top bar ── */
    GtkWidget *topbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 16);
    gtk_widget_set_margin_start(topbar, 20);
    gtk_widget_set_margin_end(topbar,  20);
    gtk_widget_set_margin_top(topbar,  10);
    gtk_box_pack_start(GTK_BOX(root), topbar, FALSE, FALSE, 0);

    GtkWidget *title_lbl = gtk_label_new("ANTEATER POKER");
    gtk_widget_set_name(title_lbl, "login-title"); /* reuse gold style */
    gtk_box_pack_start(GTK_BOX(topbar), title_lbl, FALSE, FALSE, 0);

    W.deck_count_label = gtk_label_new("Deck: 52");
    gtk_box_pack_end(GTK_BOX(topbar), W.deck_count_label, FALSE, FALSE, 0);

    /* ── main area: opponents + felt + hand ── */
    GtkWidget *main_area = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_widget_set_margin_start(main_area, 20);
    gtk_widget_set_margin_end(main_area,   20);
    gtk_box_pack_start(GTK_BOX(root), main_area, TRUE, TRUE, 0);

    /* --- opponent row (3 seats across the top) --- */
    GtkWidget *opp_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_halign(opp_row, GTK_ALIGN_FILL);
    gtk_box_pack_start(GTK_BOX(main_area), opp_row, FALSE, FALSE, 4);

    static const char *opp_defaults[3] = {"Bot 2","Bot 3","Bot 4"};
    for (int i = 0; i < 3; i++) {
        GtkWidget *frame = gtk_event_box_new();
        gtk_widget_set_name(frame, "opp-frame");
        GtkStyleContext *ctx = gtk_widget_get_style_context(frame);
        gtk_style_context_add_class(ctx, "opp-frame");
        W.opp_frame[i] = frame;

        GtkWidget *col = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
        gtk_widget_set_margin_start(col, 8);
        gtk_widget_set_margin_end(col,   8);
        gtk_widget_set_margin_top(col,   6);
        gtk_widget_set_margin_bottom(col,6);
        gtk_container_add(GTK_CONTAINER(frame), col);

        /* two face-down card icons for opponents */
        GtkWidget *card_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
        gtk_box_pack_start(GTK_BOX(col), card_row, FALSE, FALSE, 0);
        GtkWidget *c1 = make_card_widget(28, 40);
        GtkWidget *c2 = make_card_widget(28, 40);
        gtk_box_pack_start(GTK_BOX(card_row), c1, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(card_row), c2, FALSE, FALSE, 0);

        W.opp_name[i] = gtk_label_new(opp_defaults[i]);
        gtk_style_context_add_class(gtk_widget_get_style_context(W.opp_name[i]), "opp-name");
        gtk_widget_set_halign(W.opp_name[i], GTK_ALIGN_START);

        W.opp_chips[i] = gtk_label_new("$1000  |  bet $0");
        gtk_style_context_add_class(gtk_widget_get_style_context(W.opp_chips[i]), "opp-chips");
        gtk_widget_set_halign(W.opp_chips[i], GTK_ALIGN_START);

        W.opp_status[i] = gtk_label_new("Waiting");
        gtk_style_context_add_class(gtk_widget_get_style_context(W.opp_status[i]), "opp-status");
        gtk_widget_set_halign(W.opp_status[i], GTK_ALIGN_START);

        gtk_box_pack_start(GTK_BOX(col), W.opp_name[i],   FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(col), W.opp_chips[i],  FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(col), W.opp_status[i], FALSE, FALSE, 0);

        /* distribute evenly */
        gtk_box_pack_start(GTK_BOX(opp_row), frame, TRUE, TRUE, 6);
    }

    /* --- felt table area --- */
    GtkWidget *felt = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_widget_set_name(felt, "felt");
    gtk_widget_set_margin_start(felt, 40);
    gtk_widget_set_margin_end(felt,   40);
    gtk_widget_set_vexpand(felt, TRUE);
    gtk_widget_set_valign(felt, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(main_area), felt, TRUE, TRUE, 0);

    W.stage_label = gtk_label_new("PREFLOP");
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

    W.log_label = gtk_label_new("  Connecting …");
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

    W.btn_fold  = gtk_button_new_with_label("Fold");
    W.btn_check = gtk_button_new_with_label("Check");
    W.btn_call  = gtk_button_new_with_label("Call");
    W.btn_raise = gtk_button_new_with_label("Raise");

    gtk_widget_set_name(W.btn_fold,  "btn-fold");
    gtk_widget_set_name(W.btn_check, "btn-check");
    gtk_widget_set_name(W.btn_call,  "btn-call");
    gtk_widget_set_name(W.btn_raise, "btn-raise");

    GtkStyleContext *ctx;
    ctx = gtk_widget_get_style_context(W.btn_fold);  gtk_style_context_add_class(ctx,"action-btn");
    ctx = gtk_widget_get_style_context(W.btn_check); gtk_style_context_add_class(ctx,"action-btn");
    ctx = gtk_widget_get_style_context(W.btn_call);  gtk_style_context_add_class(ctx,"action-btn");
    ctx = gtk_widget_get_style_context(W.btn_raise); gtk_style_context_add_class(ctx,"action-btn");

    g_signal_connect(W.btn_fold,  "clicked", G_CALLBACK(on_fold),  NULL);
    g_signal_connect(W.btn_check, "clicked", G_CALLBACK(on_check), NULL);
    g_signal_connect(W.btn_call,  "clicked", G_CALLBACK(on_call),  NULL);
    g_signal_connect(W.btn_raise, "clicked", G_CALLBACK(on_raise), NULL);

    /* raise spin button */
    GtkAdjustment *adj = gtk_adjustment_new(50, 10, 10000, 10, 50, 0);
    W.raise_spin = gtk_spin_button_new(adj, 10, 0);
    gtk_widget_set_size_request(W.raise_spin, 80, -1);

    gtk_box_pack_start(GTK_BOX(btn_row), W.btn_fold,  FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(btn_row), W.btn_check, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(btn_row), W.btn_call,  FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(btn_row), W.raise_spin,FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(btn_row), W.btn_raise, FALSE, FALSE, 0);

    return root;
}

/* ── Build the login screen ─────────────────────────────────────────────────── */
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

    /* ── Play vs bots button ── */
    GtkWidget *btn = gtk_button_new_with_label("PLAY  ♠  vs Bots");
    gtk_widget_set_name(btn, "connect-btn");
    gtk_box_pack_start(GTK_BOX(card), btn, FALSE, FALSE, 20);
    g_signal_connect(btn, "clicked", G_CALLBACK(on_play_clicked), NULL);
    g_signal_connect_swapped(W.name_entry, "activate", G_CALLBACK(gtk_button_clicked), btn);

    /* ── Divider ── */
    GtkWidget *divider = gtk_label_new("────  OR  ────");
    gtk_widget_set_name(divider, "divider-label");
    gtk_widget_set_halign(divider, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(card), divider, FALSE, FALSE, 6);

    /* ── Server fields ── */
    FIELD("SERVER HOST", W.host_entry, "localhost", "localhost");
    FIELD("PORT",        W.port_entry, "10160",     "10160");
#undef FIELD

    /* ── Connect to server button ── */
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

/* ── Main ───────────────────────────────────────────────────────────────────── */
int main(int argc, char *argv[])
{
    srand((unsigned)time(NULL));
    gtk_init(&argc, &argv);

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
    GtkWidget *stack = gtk_stack_new();
    gtk_container_add(GTK_CONTAINER(W.window), stack);

    W.login_screen = build_login_screen();
    W.game_screen  = build_game_screen();

    gtk_stack_add_named(GTK_STACK(stack), W.login_screen, "login");
    gtk_stack_add_named(GTK_STACK(stack), W.game_screen,  "game");
    gtk_stack_set_visible_child_name(GTK_STACK(stack), "login");

    gtk_widget_show_all(W.window);
    gtk_widget_hide(W.game_screen);

    gtk_main();
    return 0;
}
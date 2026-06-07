#include "server_gui.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/* =========================================================================
   Single-instance widget store
   ========================================================================= */
typedef struct {
    GtkWidget *window;

    /* top status row */
    GtkWidget *lbl_pot;
    GtkWidget *lbl_stage;
    GtkWidget *lbl_turn;
    GtkWidget *lbl_phase;
    GtkWidget *lbl_hand;
    GtkWidget *lbl_pcount;
    GtkWidget *lbl_port;
    GtkWidget *lbl_community;

    /* per-seat row labels */
    GtkWidget *seat_name  [MAX_PLAYERS];
    GtkWidget *seat_chips [MAX_PLAYERS];
    GtkWidget *seat_bet   [MAX_PLAYERS];
    GtkWidget *seat_status[MAX_PLAYERS];
    GtkWidget *seat_cards [MAX_PLAYERS];

    /* event log */
    GtkWidget *log_view;

    ServerState *state;   /* backing state for button actions */
} ServerGui;

static ServerGui SG;      /* zero-initialized */

static int server_gui_seat_has_client(ServerState *state, int seat)
{
    for (int i = 0; i < MAX_CLIENTS; i++)
        if (state->clients[i].connected && state->clients[i].id == seat)
            return 1;
    return 0;
}

static void server_gui_clear_unoccupied_seats(ServerState *state)
{
    state->game.playerCount = 0;
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (server_gui_seat_has_client(state, i)) {
            state->game.players[i].status = PLAYER_CONNECTED;
            state->game.players[i].chips = INIT_CHIPS;
            state->game.players[i].place = 0;
            state->game.playerCount++;
        } else {
            memset(&state->game.players[i], 0, sizeof(Player));
            state->game.players[i].id = i;
            state->game.players[i].status = PLAYER_EMPTY;
        }
    }
}

/* =========================================================================
   Small enum -> text helpers
   ========================================================================= */
static const char *stage_name(uint8_t s)
{
    switch (s) {
        case PREFLOP: return "PREFLOP";
        case FLOP:    return "FLOP";
        case TURN:    return "TURN";
        case RIVER:   return "RIVER";
        default:      return "-";
    }
}

static const char *phase_name(HandPhase p)
{
    switch (p) {
        case HAND_IDLE:     return "IDLE";
        case HAND_BETTING:  return "BETTING";
        case HAND_RUNOUT:   return "RUNOUT";
        case HAND_COMPLETE: return "COMPLETE";
        default:            return "-";
    }
}

static const char *status_name(uint8_t st)
{
    switch (st) {
        case PLAYER_EMPTY:        return "empty";
        case PLAYER_CONNECTED:    return "connected";
        case PLAYER_READY:        return "ready";
        case PLAYER_PLAYING:      return "playing";
        case PLAYER_SPECTATING:   return "spectating";
        case PLAYER_FOLDED:       return "folded";
        case PLAYER_ALL_IN:       return "all-in";
        case PLAYER_DISCONNECTED: return "disconnected";
        default:                  return "?";
    }
}

/* "AS", "Th", "??" for hidden/empty */
static void card_str(Card c, char *out, size_t n)
{
    static const char *RANKS = "?23456789TJQKA"; /* index by Rank enum 0..13 */
    static const char *SUITS = "?HDCS";          /* index by Suit enum 0..4  */
    if (c.rank == UNKNOW_R || c.suit == UNKNOW_S || c.rank > ACE || c.suit > SPADES) {
        snprintf(out, n, "??");
        return;
    }
    snprintf(out, n, "%c%c", RANKS[c.rank], SUITS[c.suit]);
}

/* =========================================================================
   Button / window callbacks  (no game-flow buttons -- admin only)
   ========================================================================= */

/* Restart: full reset to a clean lobby. Restores chips, clears the board,
   sets players back to CONNECTED, then broadcasts. Does NOT force a deal --
   the next hand begins through the normal ready-up flow. */
static void on_restart_clicked(GtkButton *b, gpointer u)
{
    (void)b; (void)u;
    if (!SG.state) return;
    resetGame(&SG.state->game);
    server_gui_clear_unoccupied_seats(SG.state);
    SG.state->timer_active = 0;
    SG.state->time_between_hands = 0;
    broadcast_game_state(SG.state);
    broadcast_chat_message(SG.state, MAX_PLAYERS, "Type /ready to ready up.");
    server_gui_log("Game reset - chips restored. Waiting for players to ready up.");
}

/* Stop Server: end the main loop cleanly (cleanup_server runs after). */
static void on_stop_clicked(GtkButton *b, gpointer u)
{
    (void)b; (void)u;
    server_gui_log("Stopping server...");
    if (SG.state) SG.state->running = 0;
}

/* Clear Log: empty the on-screen event log. */
static void on_clearlog_clicked(GtkButton *b, gpointer u)
{
    (void)b; (void)u;
    if (!SG.log_view) return;
    GtkTextBuffer *tb = gtk_text_view_get_buffer(GTK_TEXT_VIEW(SG.log_view));
    gtk_text_buffer_set_text(tb, "", -1);
}

static gboolean on_window_delete(GtkWidget *w, GdkEvent *e, gpointer u)
{
    (void)w; (void)e; (void)u;
    if (SG.state) SG.state->running = 0; /* closing the window stops the server */
    return FALSE;                        /* allow the window to be destroyed */
}

/* =========================================================================
   CSS -- high-contrast, readable
   ========================================================================= */
static const char *SGUI_CSS =
/* Force dark background on ALL container types — without this GTK3 renders
   GtkBox and GtkGrid with the system light theme, making light text invisible */
"window, box, grid, scrolledwindow { background-color: #0d1117; }"
"label  { color: #c9d1d9; font-family: 'Georgia', serif; }"

"#sg-title  { font-size: 22px; font-weight: bold; color: #e6c87a; letter-spacing: 3px; }"
"#sg-stat   { font-size: 14px; color: #a8d8ea; }"         /* light blue: readable on dark */
"#sg-port   { font-size: 14px; color: #7ab8e8; font-weight: bold; }"
"#sg-head   { font-size: 13px; color: #8b949e; font-weight: bold; letter-spacing: 1px; }"
"#sg-cell   { font-size: 14px; color: #e0e6ed; }"         /* near-white: readable on dark */
"#sg-active { font-size: 14px; color: #ffd700; font-weight: bold; }"

/* GtkTextView needs two selectors: the widget and its inner text node */
"textview {"
"  background-color: #0d1117;"
"  font-family: monospace; font-size: 12px;"
"  border: 1px solid #30363d; border-radius: 6px; padding: 4px;"
"}"
"textview text { background-color: #0d1117; color: #8fd98f; }"

".sg-btn {"
"  border-radius: 8px; font-family: 'Georgia', serif; font-size: 14px;"
"  font-weight: bold; letter-spacing: 1px; padding: 11px 24px; border: none;"
"  background-image: none; color: #ffffff;"
"}"
".sg-btn label { color: #ffffff; font-weight: bold; }"
"#sg-restart { background-color: #2ea043; color: #ffffff; }"
"#sg-restart:hover  { background-color: #3fb950; }"
"#sg-restart:active { background-color: #258838; }"
"#sg-stop    { background-color: #c0392b; color: #ffffff; }"
"#sg-stop:hover  { background-color: #da4b3b; }"
"#sg-stop:active { background-color: #a52f23; }"
"#sg-clear   { background-color: #444c56; color: #ffffff; }"
"#sg-clear:hover  { background-color: #545d68; }"
"#sg-clear:active { background-color: #383f47; }";

/* =========================================================================
   Build helpers
   ========================================================================= */
static GtkWidget *mklabel(const char *text, const char *name)
{
    GtkWidget *l = gtk_label_new(text);
    if (name) gtk_widget_set_name(l, name);
    gtk_widget_set_halign(l, GTK_ALIGN_START);
    return l;
}

static GtkWidget *mkbutton(const char *text, const char *name, GCallback cb)
{
    GtkWidget *btn = gtk_button_new_with_label(text);
    gtk_widget_set_name(btn, name);
    gtk_style_context_add_class(gtk_widget_get_style_context(btn), "sg-btn");
    g_signal_connect(btn, "clicked", cb, NULL);
    return btn;
}

/* =========================================================================
   Public: init
   ========================================================================= */
void server_gui_init(ServerState *state)
{
    SG.state = state;

    GtkCssProvider *css = gtk_css_provider_new();
    gtk_css_provider_load_from_data(css, SGUI_CSS, -1, NULL);
    gtk_style_context_add_provider_for_screen(
        gdk_screen_get_default(), GTK_STYLE_PROVIDER(css),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(css);

    SG.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(SG.window), "Anteater Poker - Server");
    gtk_window_set_default_size(GTK_WINDOW(SG.window), 560, 500);
    gtk_container_set_border_width(GTK_CONTAINER(SG.window), 16);
    g_signal_connect(SG.window, "delete-event", G_CALLBACK(on_window_delete), NULL);

    GtkWidget *root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_container_add(GTK_CONTAINER(SG.window), root);

    /* title + port */
    GtkWidget *title = mklabel("ANTEATER POKER  -  SERVER", "sg-title");
    gtk_widget_set_halign(title, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(root), title, FALSE, FALSE, 0);

    char portbuf[32];
    snprintf(portbuf, sizeof(portbuf), "Listening on port %d", PORT);
    SG.lbl_port = mklabel(portbuf, "sg-port");
    gtk_widget_set_halign(SG.lbl_port, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(root), SG.lbl_port, FALSE, FALSE, 0);

    /* status row */
    GtkWidget *stat = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 16);
    SG.lbl_pot    = mklabel("Pot: $0",    "sg-stat");
    SG.lbl_stage  = mklabel("Stage: -",   "sg-stat");
    SG.lbl_turn   = mklabel("Turn: -",    "sg-stat");
    SG.lbl_phase  = mklabel("Phase: -",   "sg-stat");
    SG.lbl_hand   = mklabel("Hand: idle", "sg-stat");
    SG.lbl_pcount = mklabel("Players: 0", "sg-stat");
    gtk_box_pack_start(GTK_BOX(stat), SG.lbl_pot,    FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(stat), SG.lbl_stage,  FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(stat), SG.lbl_turn,   FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(stat), SG.lbl_phase,  FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(stat), SG.lbl_hand,   FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(stat), SG.lbl_pcount, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(root), stat, FALSE, FALSE, 0);

    SG.lbl_community = mklabel("Board: -", "sg-stat");
    gtk_box_pack_start(GTK_BOX(root), SG.lbl_community, FALSE, FALSE, 0);

    /* player table (read-only monitor) */
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid), 20);
    gtk_grid_set_row_spacing(GTK_GRID(grid), 5);
    gtk_box_pack_start(GTK_BOX(root), grid, FALSE, FALSE, 6);

    const char *heads[] = { "Seat", "Name", "Chips", "Bet", "Status", "Cards" };
    for (int c = 0; c < 6; c++)
        gtk_grid_attach(GTK_GRID(grid), mklabel(heads[c], "sg-head"), c, 0, 1, 1);

    for (int i = 0; i < MAX_PLAYERS; i++) {
        char seat[8];
        snprintf(seat, sizeof(seat), "%d", i);
        gtk_grid_attach(GTK_GRID(grid), mklabel(seat, "sg-cell"), 0, i + 1, 1, 1);

        SG.seat_name[i]   = mklabel("-", "sg-cell");
        SG.seat_chips[i]  = mklabel("-", "sg-cell");
        SG.seat_bet[i]    = mklabel("-", "sg-cell");
        SG.seat_status[i] = mklabel("-", "sg-cell");
        SG.seat_cards[i]  = mklabel("-", "sg-cell");
        gtk_grid_attach(GTK_GRID(grid), SG.seat_name[i],   1, i + 1, 1, 1);
        gtk_grid_attach(GTK_GRID(grid), SG.seat_chips[i],  2, i + 1, 1, 1);
        gtk_grid_attach(GTK_GRID(grid), SG.seat_bet[i],    3, i + 1, 1, 1);
        gtk_grid_attach(GTK_GRID(grid), SG.seat_status[i], 4, i + 1, 1, 1);
        gtk_grid_attach(GTK_GRID(grid), SG.seat_cards[i],  5, i + 1, 1, 1);
    }

    /* button row -- admin controls only */
    GtkWidget *btns = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_widget_set_halign(btns, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(btns), mkbutton("Restart Game", "sg-restart", G_CALLBACK(on_restart_clicked)),  FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(btns), mkbutton("Stop Server",  "sg-stop",    G_CALLBACK(on_stop_clicked)),     FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(btns), mkbutton("Clear Log",    "sg-clear",   G_CALLBACK(on_clearlog_clicked)), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(root), btns, FALSE, FALSE, 8);

    /* event log */
    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
                                   GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_vexpand(scroll, TRUE);
    SG.log_view = gtk_text_view_new();
    gtk_widget_set_name(SG.log_view, "sg-log");
    gtk_text_view_set_editable(GTK_TEXT_VIEW(SG.log_view), FALSE);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(SG.log_view), FALSE);
    gtk_container_add(GTK_CONTAINER(scroll), SG.log_view);
    gtk_box_pack_start(GTK_BOX(root), scroll, TRUE, TRUE, 0);

    gtk_widget_show_all(SG.window);
    server_gui_log("Server monitor ready. Listening on port %d.", PORT);
}

/* =========================================================================
   Public: refresh
   ========================================================================= */
void server_gui_refresh(const GameState *gs)
{
    if (!SG.window || !gs) return;
    char buf[128];

    snprintf(buf, sizeof(buf), "Pot: $%u", gs->pot);                 gtk_label_set_text(GTK_LABEL(SG.lbl_pot), buf);
    snprintf(buf, sizeof(buf), "Stage: %s", stage_name(gs->stage));  gtk_label_set_text(GTK_LABEL(SG.lbl_stage), buf);
    snprintf(buf, sizeof(buf), "Turn: %u", gs->currentPlayer);       gtk_label_set_text(GTK_LABEL(SG.lbl_turn), buf);
    snprintf(buf, sizeof(buf), "Phase: %s", phase_name(gs->phase));  gtk_label_set_text(GTK_LABEL(SG.lbl_phase), buf);
    snprintf(buf, sizeof(buf), "Hand: %s", gs->handPlaying ? "playing" : "idle");
    gtk_label_set_text(GTK_LABEL(SG.lbl_hand), buf);
    snprintf(buf, sizeof(buf), "Players: %u", gs->playerCount);      gtk_label_set_text(GTK_LABEL(SG.lbl_pcount), buf);

    /* board */
    char board[64] = "Board: ";
    if (gs->communityCount == 0) {
        strcat(board, "-");
    } else {
        for (int i = 0; i < gs->communityCount && i < 5; i++) {
            char cs[4];
            card_str(gs->community[i], cs, sizeof(cs));
            strcat(board, cs);
            strcat(board, " ");
        }
    }
    gtk_label_set_text(GTK_LABEL(SG.lbl_community), board);

    /* seats */
    for (int i = 0; i < MAX_PLAYERS; i++) {
        const Player *p = &gs->players[i];

        gtk_label_set_text(GTK_LABEL(SG.seat_name[i]),
            p->name[0] ? p->name : (p->status == PLAYER_EMPTY ? "-" : "Player"));

        snprintf(buf, sizeof(buf), "$%u", p->chips);
        gtk_label_set_text(GTK_LABEL(SG.seat_chips[i]), buf);

        snprintf(buf, sizeof(buf), "$%u", p->current_bet);
        gtk_label_set_text(GTK_LABEL(SG.seat_bet[i]), buf);

        gtk_label_set_text(GTK_LABEL(SG.seat_status[i]), status_name(p->status));

        if (p->has_cards) {
            char a[4], b[4];
            card_str(p->hand[0], a, sizeof(a));
            card_str(p->hand[1], b, sizeof(b));
            snprintf(buf, sizeof(buf), "%s %s", a, b);
        } else {
            snprintf(buf, sizeof(buf), "-");
        }
        gtk_label_set_text(GTK_LABEL(SG.seat_cards[i]), buf);

        /* highlight whoever is to act */
        const char *nm = (gs->handPlaying && i == gs->currentPlayer) ? "sg-active" : "sg-cell";
        gtk_widget_set_name(SG.seat_name[i], nm);
    }
}

/* =========================================================================
   Public: pump + log
   ========================================================================= */
void server_gui_pump(void)
{
    while (gtk_events_pending())
        gtk_main_iteration_do(FALSE);
}

void server_gui_log(const char *fmt, ...)
{
    if (!SG.log_view) return;

    char msg[512];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(msg, sizeof(msg), fmt, ap);
    va_end(ap);

    char line[544];
    snprintf(line, sizeof(line), "%s\n", msg);

    GtkTextBuffer *tb = gtk_text_view_get_buffer(GTK_TEXT_VIEW(SG.log_view));
    GtkTextIter end;
    gtk_text_buffer_get_end_iter(tb, &end);
    gtk_text_buffer_insert(tb, &end, line, -1);

    gtk_text_buffer_get_end_iter(tb, &end);
    GtkTextMark *mark = gtk_text_buffer_get_mark(tb, "insert");
    gtk_text_buffer_move_mark(tb, mark, &end);
    gtk_text_view_scroll_mark_onscreen(GTK_TEXT_VIEW(SG.log_view), mark);
}

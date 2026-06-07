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
        default:      return "—";
    }
}

static const char *phase_name(HandPhase p)
{
    switch (p) {
        case HAND_IDLE:     return "IDLE";
        case HAND_BETTING:  return "BETTING";
        case HAND_RUNOUT:   return "RUNOUT";
        case HAND_COMPLETE: return "COMPLETE";
        default:            return "—";
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
   Button / window callbacks
   ========================================================================= */
static void on_newhand_clicked(GtkButton *b, gpointer u)
{
    (void)b; (void)u;
    if (!SG.state) return;
    start_new_hand(SG.state);            /* same entry the READY path uses */
    server_gui_log("New hand started.");
}

static void on_restart_clicked(GtkButton *b, gpointer u)
{
    (void)b; (void)u;
    if (!SG.state) return;
    GameState *g = &SG.state->game;

    /* free disconnected seats and bot seats (resetGame would otherwise keep them).
       Emptied bot seats are automatically refilled by addBot() on the next /ready
       or New Hand, so the bots rejoin the next hand. */
    for (int s = 0; s < MAX_PLAYERS; s++) {
        if (g->players[s].status == PLAYER_DISCONNECTED || isBot(g->players[s].name)) {
            memset(&g->players[s], 0, sizeof(Player));
            g->players[s].id = s;
            g->players[s].status = PLAYER_EMPTY;
        }
    }

    /* initial state: remaining (human) players back to $1000, board/pot cleared, gameOver=0 */
    resetGame(g);

    broadcast_chat_message(SG.state, MAX_PLAYERS, "Game restarted — chips reset to $1000.");
    broadcast_game_state(SG.state);
    server_gui_log("Game restarted — chips reset, bot & disconnected seats cleared.");
}

static void on_addbots_clicked(GtkButton *b, gpointer u)
{
    (void)b; (void)u;
    if (!SG.state) return;
    addBot(&SG.state->game, true);       /* fills empty seats (shuffle bot names) */
    broadcast_game_state(SG.state);
    server_gui_log("Bots added to empty seats.");
}

static void on_quit_clicked(GtkButton *b, gpointer u)
{
    (void)b; (void)u;
    if (SG.state) {
        broadcast_chat_message(SG.state, MAX_PLAYERS, "Server has been closed.");
        SG.state->running = 0;           /* let server.c's loop exit + cleanup */
    }
}

static gboolean on_window_delete(GtkWidget *w, GdkEvent *e, gpointer u)
{
    (void)w; (void)e; (void)u;
    if (SG.state) {
        broadcast_chat_message(SG.state, MAX_PLAYERS, "Server has been closed.");
        SG.state->running = 0;           /* closing the window stops the server */
    }
    return FALSE;                        /* allow the window to be destroyed */
}

/* =========================================================================
   Public: init
   ========================================================================= */
/* relay used by broadcast_chat_message() to mirror client chat into the monitor */
static void sg_chat_relay(const char *line)
{
    server_gui_log("%s", line);
}

void server_gui_init(ServerState *state)
{
    SG.state = state;

    /* mirror all client/server chat into the event log */
    server_chat_log_hook = sg_chat_relay;

    /* CSS — loaded from server_gui.css (working dir) so Glade can preview the
       same styling via its interface-css-provider-path reference */
    GtkCssProvider *css = gtk_css_provider_new();
    GError *css_err = NULL;
    if (!gtk_css_provider_load_from_path(css, "server_gui.css", &css_err)) {
        fprintf(stderr, "server_gui: could not load server_gui.css: %s\n",
                css_err ? css_err->message : "unknown error");
        g_clear_error(&css_err);
    }
    gtk_style_context_add_provider_for_screen(
        gdk_screen_get_default(), GTK_STYLE_PROVIDER(css),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(css);

    /* UI is defined in server_gui.glade (loaded from the working directory) */
    GtkBuilder *b = gtk_builder_new_from_file("server_gui.glade");
#define SG_GET(id) GTK_WIDGET(gtk_builder_get_object(b, (id)))

    SG.window        = SG_GET("sg_window");
    SG.lbl_pot       = SG_GET("lbl_pot");
    SG.lbl_stage     = SG_GET("lbl_stage");
    SG.lbl_turn      = SG_GET("lbl_turn");
    SG.lbl_phase     = SG_GET("lbl_phase");
    SG.lbl_hand      = SG_GET("lbl_hand");
    SG.lbl_pcount    = SG_GET("lbl_pcount");
    SG.lbl_community = SG_GET("lbl_community");
    SG.log_view      = SG_GET("sg_log_view");

    for (int i = 0; i < MAX_PLAYERS; i++) {
        char id[24];
        snprintf(id, sizeof id, "seat_name_%d",   i); SG.seat_name[i]   = SG_GET(id);
        snprintf(id, sizeof id, "seat_chips_%d",  i); SG.seat_chips[i]  = SG_GET(id);
        snprintf(id, sizeof id, "seat_bet_%d",    i); SG.seat_bet[i]    = SG_GET(id);
        snprintf(id, sizeof id, "seat_status_%d", i); SG.seat_status[i] = SG_GET(id);
        snprintf(id, sizeof id, "seat_cards_%d",  i); SG.seat_cards[i]  = SG_GET(id);
    }

    /* signals (handlers stay static -> wired here rather than via Glade) */
    g_signal_connect(SG.window,            "delete-event", G_CALLBACK(on_window_delete),   NULL);
    g_signal_connect(SG_GET("sg_newhand"), "clicked",      G_CALLBACK(on_newhand_clicked), NULL);
    g_signal_connect(SG_GET("sg_restart"), "clicked",      G_CALLBACK(on_restart_clicked), NULL);
    g_signal_connect(SG_GET("sg_addbots"), "clicked",      G_CALLBACK(on_addbots_clicked), NULL);
    g_signal_connect(SG_GET("sg_quit"),    "clicked",      G_CALLBACK(on_quit_clicked),    NULL);

#undef SG_GET
    g_object_unref(b);

    gtk_widget_show_all(SG.window);
    server_gui_log("Server monitor ready.");
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
        strcat(board, "—");
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
            p->name[0] ? p->name : (p->status == PLAYER_EMPTY ? "—" : "Player"));

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
            snprintf(buf, sizeof(buf), "—");
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

    /* autoscroll */
    gtk_text_buffer_get_end_iter(tb, &end);
    GtkTextMark *mark = gtk_text_buffer_get_mark(tb, "insert");
    gtk_text_buffer_move_mark(tb, mark, &end);
    gtk_text_view_scroll_mark_onscreen(GTK_TEXT_VIEW(SG.log_view), mark);
}

#include "gui_helpers.h"
#include <stdio.h>
#include <string.h>

/* -- Rank / suit ----------------------------------------------------------- */
const char *RANK_STR[13] = {
    "2","3","4","5","6","7","8","9","10","J","Q","K","A"
};
const char *SUIT_STR[4]  = {"♥","♦","♣","♠"};
const int   SUIT_RED[4]  = {1, 1, 0, 0};

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
int card_is_known(Card card)
{
    return card.rank != UNKNOW_R && card.suit != UNKNOW_S;
}

/* -- Card drawing ---------------------------------------------------------- */
static gboolean draw_card_cb(GtkWidget *widget, cairo_t *cr, gpointer data)
{
    CardDrawData *d = (CardDrawData *)data;
    int w = gtk_widget_get_allocated_width(widget);
    int h = gtk_widget_get_allocated_height(widget);

    if (d->face_up) cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    else            cairo_set_source_rgb(cr, 0.1, 0.18, 0.5);

    double r = 5.0;
    cairo_move_to(cr, r, 0);
    cairo_line_to(cr, w-r, 0); cairo_arc(cr, w-r, r,   r,  -G_PI/2, 0);
    cairo_line_to(cr, w, h-r); cairo_arc(cr, w-r, h-r, r,  0,       G_PI/2);
    cairo_line_to(cr, r, h);   cairo_arc(cr, r,   h-r, r,  G_PI/2,  G_PI);
    cairo_line_to(cr, 0, r);   cairo_arc(cr, r,   r,   r,  G_PI,   -G_PI/2);
    cairo_close_path(cr);
    cairo_fill_preserve(cr);
    cairo_set_source_rgb(cr, 0.6, 0.6, 0.6);
    cairo_set_line_width(cr, 0.8);
    cairo_stroke(cr);

    if (!d->face_up || !card_is_known(d->card)) return FALSE;

    if (suit_is_red(d->card.suit)) cairo_set_source_rgb(cr, 0.80, 0.15, 0.15);
    else                           cairo_set_source_rgb(cr, 0.10, 0.10, 0.10);

    cairo_select_font_face(cr, "Georgia", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, w < 36 ? 9 : 12);
    cairo_move_to(cr, 4, 14);
    cairo_show_text(cr, RANK_STR[rank_index(d->card.rank)]);
    cairo_set_font_size(cr, w < 36 ? 10 : 14);
    cairo_move_to(cr, 4, h-4);
    cairo_show_text(cr, suit_label(d->card.suit));
    return FALSE;
}

GtkWidget *make_card_widget(int w, int h)
{
    GtkWidget *da = gtk_drawing_area_new();
    gtk_widget_set_size_request(da, w, h);
    CardDrawData *d = g_new0(CardDrawData, 1);
    g_object_set_data_full(G_OBJECT(da), "card-data", d, g_free);
    g_signal_connect(da, "draw", G_CALLBACK(draw_card_cb), d);
    return da;
}

void init_card_widget(GtkWidget *da)
{
    CardDrawData *d = g_new0(CardDrawData, 1);
    g_object_set_data_full(G_OBJECT(da), "card-data", d, g_free);
    g_signal_connect(da, "draw", G_CALLBACK(draw_card_cb), d);
}

void set_card_face(GtkWidget *da, Card c, int face_up)
{
    CardDrawData *d = g_object_get_data(G_OBJECT(da), "card-data");
    if (!d) return;
    d->card    = c;
    d->face_up = face_up;
    gtk_widget_queue_draw(da);
}

void set_card_back(GtkWidget *da)
{
    CardDrawData *d = g_object_get_data(G_OBJECT(da), "card-data");
    if (!d) return;
    d->face_up = 0;
    gtk_widget_queue_draw(da);
}

/* -- Timer internals ------------------------------------------------------- */
typedef struct { SeatTimer *t; } TimerCBData;

static gboolean timer_tick_cb(gpointer user_data)
{
    TimerCBData *cb = (TimerCBData *)user_data;
    SeatTimer   *t  = cb->t;

    if (t->seconds_left <= 0) {
        gtk_widget_hide(t->bar);
        gtk_label_set_text(GTK_LABEL(t->label), "");
        t->timer_id = 0;
        if (t->is_my_timer) {
            send_gui_move(FOLD, 0);
        }
        g_free(cb);
        return G_SOURCE_REMOVE;
    }
    t->seconds_left--;
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(t->bar),
        (double)t->seconds_left / (double)t->turn_seconds);
    char buf[16];
    snprintf(buf, sizeof(buf), "%d:%02d", t->seconds_left/60, t->seconds_left%60);
    gtk_label_set_text(GTK_LABEL(t->label), buf);
    return G_SOURCE_CONTINUE;
}

void start_seat_timer(SeatTimer *t, int seconds)
{
    if (t->timer_id) { g_source_remove(t->timer_id); t->timer_id = 0; }
    t->turn_seconds = seconds;
    t->seconds_left = seconds;
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(t->bar), 1.0);
    gtk_widget_show(t->bar);
    char buf[16];
    snprintf(buf, sizeof(buf), "%d:%02d", seconds/60, seconds%60);
    gtk_label_set_text(GTK_LABEL(t->label), buf);
    TimerCBData *cb = g_new0(TimerCBData, 1);
    cb->t = t;
    t->timer_id = g_timeout_add_seconds(1, timer_tick_cb, cb);
}

void stop_seat_timer(SeatTimer *t)
{
    if (t->timer_id) { g_source_remove(t->timer_id); t->timer_id = 0; }
    gtk_widget_hide(t->bar);
    gtk_label_set_text(GTK_LABEL(t->label), "");
}

/* -- Public timer API ------------------------------------------------------ */
void start_player_timer(int seat, int seconds)
{
    if (seat < 0 || seat >= GUI_OPPONENT_SLOTS) return;
    start_seat_timer(&W.opp_timer[seat], seconds);
}
void stop_player_timer(int seat)
{
    if (seat < 0 || seat >= GUI_OPPONENT_SLOTS) return;
    stop_seat_timer(&W.opp_timer[seat]);
}
void start_my_timer(int seconds) { start_seat_timer(&W.my_timer, seconds); }
void stop_my_timer(void)         { stop_seat_timer(&W.my_timer); }

/* -- Network send helpers -------------------------------------------------- */
void send_gui_move(MoveType move, uint32_t amount)
{
    if (!C.connected) return;
    PlayerAction action;
    action.playerID       = C.my_player_id;
    action.move           = move;
    action.amount         = amount;
    action.target         = 0;
    action.useSpecialCard = 0;
    send_action(&C, &action);
}

void sendReadyToServer(void)
{
    if (!C.connected) return;
    Message msg;
    msg.type = MSG_TYPE_READY;
    uint8_t buffer[BUFFER_SIZE];
    uint32_t len = prepare_payload(buffer, MSG_TYPE_READY, &msg);
    send_to_server(&C, buffer, len);
}

void sendNameToServer(const char *name)
{
    if (!C.connected || !name) return;
    Message msg;
    msg.type      = MSG_TYPE_JOIN;
    msg.sender_id = C.my_player_id;
    strncpy(msg.chat, name, MAX_NAME_LENTH - 1);
    msg.chat[MAX_NAME_LENTH - 1] = '\0';
    uint8_t buffer[BUFFER_SIZE];
    uint32_t len = prepare_payload(buffer, MSG_TYPE_JOIN, &msg);
    send_to_server(&C, buffer, len);
}

void sendChatToServer(const char *text)
{
    if (!C.connected) return;
    Message msg;
    msg.type      = MSG_TYPE_CHAT_MESSAGE;
    msg.sender_id = C.my_player_id;
    strncpy(msg.chat, text, MAX_PAYLOAD_SIZE - 1);
    msg.chat[MAX_PAYLOAD_SIZE - 1] = '\0';
    uint8_t buffer[BUFFER_SIZE];
    uint32_t len = prepare_payload(buffer, MSG_TYPE_CHAT_MESSAGE, &msg);
    send_to_server(&C, buffer, len);
}

void update_blind_chips(int dealer_idx, int num_players) {
    int sb_idx = (dealer_idx + 1) % num_players;
    int bb_idx = (dealer_idx + 2) % num_players;

    for (int i = 0; i < GUI_OPPONENT_SLOTS; i++) {
        if (!W.opp_blind[i]) continue;

        GtkStyleContext *ctx = gtk_widget_get_style_context(W.opp_blind[i]);
        gtk_style_context_remove_class(ctx, "chip-dealer");
        gtk_style_context_remove_class(ctx, "chip-sb");
        gtk_style_context_remove_class(ctx, "chip-bb");

        if (i == dealer_idx) {
            gtk_label_set_text(GTK_LABEL(W.opp_blind[i]), "D");
            gtk_style_context_add_class(ctx, "chip-dealer");
        } else if (i == sb_idx) {
            gtk_label_set_text(GTK_LABEL(W.opp_blind[i]), "SB");
            gtk_style_context_add_class(ctx, "chip-sb");
        } else if (i == bb_idx) {
            gtk_label_set_text(GTK_LABEL(W.opp_blind[i]), "BB");
            gtk_style_context_add_class(ctx, "chip-bb");
        } else {
            gtk_label_set_text(GTK_LABEL(W.opp_blind[i]), "");
        }
    }
}
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

/* -- Blind markers --------------------------------------------------------- */
static GdkPixbuf *blind_sb_pix = NULL;   // small-blind chip
static GdkPixbuf *blind_bb_pix = NULL;   // big-blind chip
static int        blind_pix_loaded = 0;

static void load_blind_pixbufs(void)
{
    if (blind_pix_loaded) return;
    blind_pix_loaded = 1;   // load once; relative path resolves when run from the repo dir
    blind_sb_pix = gdk_pixbuf_new_from_file("img/small blind.png", NULL);
    blind_bb_pix = gdk_pixbuf_new_from_file("img/big blind.png",   NULL);
}

static gboolean draw_blind_cb(GtkWidget *widget, cairo_t *cr, gpointer data)
{
    BlindKind kind = *(BlindKind *)data;
    GdkPixbuf *pix = (kind == BLIND_SB) ? blind_sb_pix
                   : (kind == BLIND_BB) ? blind_bb_pix
                   : NULL;
    if (!pix) return FALSE;   // BLIND_NONE, or the image failed to load -> draw nothing

    int W  = gtk_widget_get_allocated_width(widget);
    int H  = gtk_widget_get_allocated_height(widget);
    int pw = gdk_pixbuf_get_width(pix);
    int ph = gdk_pixbuf_get_height(pix);
    if (pw <= 0 || ph <= 0) return FALSE;

    // scale to fit while keeping aspect ratio, then center
    double sx = (double)W / pw, sy = (double)H / ph;
    double s  = sx < sy ? sx : sy;
    double dw = pw * s, dh = ph * s;

    cairo_save(cr);
    cairo_translate(cr, (W - dw) / 2.0, (H - dh) / 2.0);
    cairo_scale(cr, s, s);
    gdk_cairo_set_source_pixbuf(cr, pix, 0, 0);
    cairo_paint(cr);
    cairo_restore(cr);
    return FALSE;
}

void init_blind_widget(GtkWidget *da)
{
    load_blind_pixbufs();
    BlindKind *k = g_new0(BlindKind, 1);   // 0 == BLIND_NONE
    g_object_set_data_full(G_OBJECT(da), "blind-kind", k, g_free);
    g_signal_connect(da, "draw", G_CALLBACK(draw_blind_cb), k);
}

void set_blind_marker(GtkWidget *da, BlindKind kind)
{
    BlindKind *k = g_object_get_data(G_OBJECT(da), "blind-kind");
    if (!k) return;
    *k = kind;
    gtk_widget_queue_draw(da);
}

/* -- Chip win floating popup ----------------------------------------------- */
typedef struct { GtkWidget *popup; float alpha; int x, y; guint id; } ChipWin;

static gboolean chip_win_tick(gpointer data)
{
    ChipWin *cw = (ChipWin *)data;
    cw->alpha -= 0.025f;
    cw->y     -= 1;
    if (cw->alpha <= 0.0f || !GTK_IS_WIDGET(cw->popup)) {
        if (GTK_IS_WIDGET(cw->popup)) gtk_widget_destroy(cw->popup);
        g_free(cw);
        return G_SOURCE_REMOVE;
    }
    gtk_widget_set_opacity(cw->popup, (double)cw->alpha);
    gtk_window_move(GTK_WINDOW(cw->popup), cw->x, cw->y);
    return G_SOURCE_CONTINUE;
}

void start_chip_win_anim(const char *winner_name, uint32_t amount)
{
    if (!W.window) return;

    GtkWidget *popup = gtk_window_new(GTK_WINDOW_POPUP);
    gtk_window_set_decorated(GTK_WINDOW(popup), FALSE);
    gtk_window_set_skip_taskbar_hint(GTK_WINDOW(popup), TRUE);
    gtk_window_set_transient_for(GTK_WINDOW(popup), GTK_WINDOW(W.window));

    char buf[128];
    snprintf(buf, sizeof(buf), "%s wins  +$%u",
             (winner_name && winner_name[0]) ? winner_name : "Player", amount);

    char markup[256];
    snprintf(markup, sizeof(markup),
        "<span foreground='#ffd700' font='Georgia Bold 20'>%s</span>", buf);
    GtkWidget *label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label), markup);

    GtkWidget *box = gtk_event_box_new();
    gtk_widget_set_name(box, "chip-win-box");
    gtk_container_add(GTK_CONTAINER(box), label);
    gtk_container_add(GTK_CONTAINER(popup), box);

    GtkCssProvider *prov = gtk_css_provider_new();
    gtk_css_provider_load_from_data(prov,
        "#chip-win-box { background-color: rgba(25,15,45,0.92);"
        "  border: 2px solid #ffd700; border-radius: 10px;"
        "  padding: 10px 22px; }",
        -1, NULL);
    gtk_style_context_add_provider(
        gtk_widget_get_style_context(box),
        GTK_STYLE_PROVIDER(prov),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION + 1);
    g_object_unref(prov);

    int wx = 0, wy = 0, ww = 900, wh = 660;
    gtk_window_get_position(GTK_WINDOW(W.window), &wx, &wy);
    gtk_window_get_size(GTK_WINDOW(W.window), &ww, &wh);
    int px = wx + ww / 2 - 140;
    int py = wy + wh / 2 - 25;
    gtk_window_move(GTK_WINDOW(popup), px, py);
    gtk_widget_show_all(popup);

    ChipWin *cw = g_new0(ChipWin, 1);
    cw->popup = popup;
    cw->alpha = 1.0f;
    cw->x     = px;
    cw->y     = py;
    cw->id    = g_timeout_add(30, chip_win_tick, cw);
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

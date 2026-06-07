#include "gui_helpers.h"
#include "specialCards.h"

void on_shop_card_clicked(GtkButton *b, gpointer d);
#include <stdio.h>
#include <string.h>

#define SHOP_ICON_W 180
#define SHOP_ICON_H 252

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

    /* selection border draws on both face-up and face-down cards (opponent cards
       are usually face-down during targeting) */
    if (d->highlight) {
        cairo_set_source_rgb(cr, 1.0, 0.84, 0.0);  /* gold */
        cairo_set_line_width(cr, 3.0);
        cairo_rectangle(cr, 1.5, 1.5, w - 3.0, h - 3.0);
        cairo_stroke(cr);
    }

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

void set_card_highlight(GtkWidget *da, gboolean on)
{
    if (!da) return;
    CardDrawData *d = g_object_get_data(G_OBJECT(da), "card-data");
    if (!d) return;
    d->highlight = on ? 1 : 0;
    gtk_widget_queue_draw(da);
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
    if (!da) return;
    CardDrawData *d = g_new0(CardDrawData, 1);
    g_object_set_data_full(G_OBJECT(da), "card-data", d, g_free);
    g_signal_connect(da, "draw", G_CALLBACK(draw_card_cb), d);
    gtk_widget_add_events(da, GDK_BUTTON_PRESS_MASK);
}

void set_card_face(GtkWidget *da, Card c, int face_up)
{
    if (!da) return;
    CardDrawData *d = g_object_get_data(G_OBJECT(da), "card-data");
    if (!d) return;
    d->card    = c;
    d->face_up = face_up;
    gtk_widget_queue_draw(da);
}

void set_card_back(GtkWidget *da)
{
    if (!da) return;
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

void sendSeatSelectToServer(uint8_t seat)
{
    if (!C.connected) return;
    Message msg;
    msg.type      = MSG_TYPE_SELECT_SEAT;
    msg.sender_id = seat; // chosen backend seat index (0-5)
    uint8_t buffer[BUFFER_SIZE];
    uint32_t len = prepare_payload(buffer, MSG_TYPE_SELECT_SEAT, &msg);
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

/* -- Shop -------------------------------------------------------------------- */

typedef enum {
    SHOP_STEP_PICK_CARD,
    SHOP_STEP_PICK_TARGET,
    SHOP_STEP_PICK_MY_CARD,
    SHOP_STEP_PICK_OPP_CARD,
    SHOP_STEP_CONFIRM
} ShopStep;

typedef struct {
    gboolean      open;
    ShopStep      step;
    Anteater_shop selected;
    uint8_t       target;
    uint8_t       my_card_idx;
    uint8_t       opp_card_idx;
    int           selected_slot;
} ShopState;

static ShopState shop;
static GdkPixbuf *shop_card_pixbufs[6];
static GdkPixbuf *shop_blank_pixbuf;

static const Anteater_shop SHOP_SLOTS[6] = {
    SWAP1, SWAP2, REVEAL, REDRAW, SWAPOPS, INSTAWIN  /* INSTAWIN last: hidden while disabled */
};

static const char *shop_card_name(Anteater_shop card)
{
    switch (card) {
        case SWAP1:    return "Swap 1 Card";
        case SWAP2:    return "Swap Both Cards";
        case REVEAL:   return "Reveal Community";
        case REDRAW:   return "Redraw Card";
        case INSTAWIN: return "Instant Win";
        case SWAPOPS:  return "Swap Opponent Cards";
        default:       return "Unknown";
    }
}

static uint32_t shop_card_price(const GameState *gs, Anteater_shop card)
{
    switch (card) {
        case SWAP1:    return PRICE_SWAP1;
        case SWAP2:    return PRICE_SWAP2;
        case REVEAL:   return PRICE_REVEAL;
        case REDRAW:   return PRICE_REDRAW;
        case SWAPOPS:  return PRICE_SWAPOPS;
        case INSTAWIN: return gs->pot * 3 / 4;
        default:       return 0;
    }
}

static gboolean shop_can_afford(Anteater_shop card)
{
    if (C.my_player_id >= MAX_PLAYERS) return FALSE;
    Player *me = &C.game.players[C.my_player_id];
    if (me->status != PLAYER_PLAYING) return FALSE;
    return me->chips >= shop_card_price(&C.game, card);
}

static gboolean shop_needs_target(Anteater_shop card)
{
    return card == SWAP1 || card == SWAP2 || card == SWAPOPS;
}

static const char *shop_image_path(Anteater_shop card)
{
    switch (card) {
        case SWAP1:    return "img/swap1_card.png";
        case SWAP2:    return "img/swap2_cards.png";
        case REVEAL:   return "img/reveal_community.jpg";
        case REDRAW:   return "img/redraw.jpg";
        case SWAPOPS:  return "img/swapopp_cards.png";
        case INSTAWIN: return "img/reveal_opponent.jpg";
        default:       return NULL;
    }
}

static void shop_cache_images(void)
{
    if (!shop_blank_pixbuf) {
        shop_blank_pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8,
                                           SHOP_ICON_W, SHOP_ICON_H);
        if (shop_blank_pixbuf)
            gdk_pixbuf_fill(shop_blank_pixbuf, 0x00000000);
    }

    for (int i = 0; i < 6; i++) {
        const char *path = shop_image_path(SHOP_SLOTS[i]);
        if (!path) continue;
        GdkPixbuf *raw = gdk_pixbuf_new_from_file(path, NULL);
        if (!raw) continue;
        shop_card_pixbufs[i] = gdk_pixbuf_scale_simple(
            raw, SHOP_ICON_W, SHOP_ICON_H, GDK_INTERP_BILINEAR);
        g_object_unref(raw);
    }
}

static void shop_clear_icon(void)
{
    if (!W.shop_icon) return;
    if (shop_blank_pixbuf)
        gtk_image_set_from_pixbuf(GTK_IMAGE(W.shop_icon), shop_blank_pixbuf);
    else
        gtk_image_clear(GTK_IMAGE(W.shop_icon));
}

static void shop_show_icon_slot(int slot)
{
    if (!W.shop_icon || slot < 0 || slot >= 6) return;
    if (shop_card_pixbufs[slot])
        gtk_image_set_from_pixbuf(GTK_IMAGE(W.shop_icon), shop_card_pixbufs[slot]);
    else
        shop_clear_icon();
}

static gboolean on_shop_card_enter(GtkWidget *widget, GdkEventCrossing *event,
                                   gpointer user_data)
{
    (void)widget; (void)event;
    shop_show_icon_slot(GPOINTER_TO_INT(user_data));
    return FALSE;
}

static gboolean on_shop_card_leave(GtkWidget *widget, GdkEventCrossing *event,
                                   gpointer user_data)
{
    (void)widget; (void)event; (void)user_data;
    shop_clear_icon();
    return FALSE;
}

void shop_wire_card_buttons(void)
{
    for (int i = 0; i < 6; i++) {
        if (!W.shop_cards[i]) continue;
        g_signal_connect(W.shop_cards[i], "clicked",
                         G_CALLBACK(on_shop_card_clicked), GINT_TO_POINTER(i));
        g_signal_connect(W.shop_cards[i], "enter-notify-event",
                         G_CALLBACK(on_shop_card_enter), GINT_TO_POINTER(i));
        g_signal_connect(W.shop_cards[i], "leave-notify-event",
                         G_CALLBACK(on_shop_card_leave), GINT_TO_POINTER(i));
    }
}

static void shop_set_prompt(const char *msg)
{
    if (W.log_label && msg)
        gtk_label_set_text(GTK_LABEL(W.log_label), msg);
}

/* a seat the server will accept as a powerup target: an opponent holding cards */
static gboolean shop_valid_target(int seat)
{
    if (seat < 0 || seat >= MAX_PLAYERS) return FALSE;
    if (seat == (int)C.my_player_id) return FALSE;
    return C.game.players[seat].has_cards ? TRUE : FALSE;
}

static int shop_valid_target_count(void)
{
    int n = 0;
    for (int s = 0; s < MAX_PLAYERS; s++) if (shop_valid_target(s)) n++;
    return n;
}

static void shop_clear_highlights(void)
{
    for (int i = 0; i < 6; i++) {
        if (W.shop_cards[i])
            gtk_style_context_remove_class(
                gtk_widget_get_style_context(W.shop_cards[i]), "shop-card-selected");
    }
    for (int i = 0; i < GUI_OPPONENT_SLOTS; i++) {
        if (W.opp_frame[i])
            gtk_style_context_remove_class(
                gtk_widget_get_style_context(W.opp_frame[i]), "shop-target-hint");
        if (W.opp_cards[i][0]) set_card_highlight(W.opp_cards[i][0], FALSE);
        if (W.opp_cards[i][1]) set_card_highlight(W.opp_cards[i][1], FALSE);
    }
    if (W.my_cards[0]) set_card_highlight(W.my_cards[0], FALSE);
    if (W.my_cards[1]) set_card_highlight(W.my_cards[1], FALSE);
}

/* highlight every valid opponent's whole tile (SWAP2 / SWAPOPS targeting) */
static void shop_highlight_target_players(void)
{
    for (int i = 0; i < GUI_OPPONENT_SLOTS; i++)
        if (W.opp_frame[i] && shop_valid_target(i))
            gtk_style_context_add_class(
                gtk_widget_get_style_context(W.opp_frame[i]), "shop-target-hint");
}

/* highlight your own two hole cards (REDRAW / SWAP1 first step) */
static void shop_highlight_my_cards(void)
{
    if (W.my_cards[0]) set_card_highlight(W.my_cards[0], TRUE);
    if (W.my_cards[1]) set_card_highlight(W.my_cards[1], TRUE);
}

/* highlight every valid opponent's cards (SWAP1 second step) */
static void shop_highlight_opp_cards(void)
{
    for (int i = 0; i < GUI_OPPONENT_SLOTS; i++) {
        if (!shop_valid_target(i)) continue;
        if (W.opp_cards[i][0]) set_card_highlight(W.opp_cards[i][0], TRUE);
        if (W.opp_cards[i][1]) set_card_highlight(W.opp_cards[i][1], TRUE);
    }
}

static void shop_reset_selection(void)
{
    shop.step           = SHOP_STEP_PICK_CARD;
    shop.selected       = SWAP1;
    shop.target         = 0;
    shop.my_card_idx    = 0;
    shop.opp_card_idx   = 0;
    shop.selected_slot  = -1;
    shop_clear_highlights();
    shop_clear_icon();
    if (W.confirm_button)
        gtk_widget_set_sensitive(W.confirm_button, FALSE);
}

gboolean shop_is_available(void)
{
    if (!C.connected || C.my_player_id >= MAX_PLAYERS) return FALSE;
    if (!shop_window_open(&C.game)) return FALSE;
    if (C.game.currentPlayer != C.my_player_id) return FALSE; //only on your turn
    return C.game.players[C.my_player_id].status == PLAYER_PLAYING;
}

gboolean shop_is_open(void)
{
    return shop.open;
}

static gboolean on_shop_delete_event(GtkWidget *widget, GdkEvent *event, gpointer data)
{
    (void)widget; (void)event; (void)data;
    shop_close();
    return TRUE;
}

void shop_close(void)
{
    shop.open = FALSE;
    shop_reset_selection();
    shop_clear_highlights();
    if (W.shop)
        gtk_widget_hide(W.shop);
}

void shop_init_dialog(void)
{
    if (!W.shop) return;
    g_signal_connect(W.shop, "delete-event", G_CALLBACK(on_shop_delete_event), NULL);
    gtk_window_set_modal(GTK_WINDOW(W.shop), FALSE);
    gtk_widget_hide(W.shop);
    if (W.shop_icon)
        gtk_widget_set_size_request(W.shop_icon, SHOP_ICON_W, SHOP_ICON_H);
    shop_cache_images();
    shop_clear_icon();
}

void shop_open(void)
{
    if (!W.shop || !shop_is_available()) return;
    shop.open = TRUE;
    shop_reset_selection();
    gtk_window_set_transient_for(GTK_WINDOW(W.shop), GTK_WINDOW(W.window));
    gtk_window_set_modal(GTK_WINDOW(W.shop), FALSE);
    gtk_widget_show_all(W.shop);
    refresh_shop_ui();   //after show_all so hidden slots (INSTAWIN) stay hidden
}

static void shop_advance_after_card_pick(void)
{
    //picking a powerup just readies Confirm; any target selection happens on the
    //table after Confirm (so the dialog can get out of the way).
    shop.step = SHOP_STEP_PICK_CARD;
    if (W.confirm_button)
        gtk_widget_set_sensitive(W.confirm_button, shop_can_afford(shop.selected));
}

void shop_on_card_slot_clicked(int slot)
{
    if (!shop.open || slot < 0 || slot >= 6) return;
    if (SHOP_SLOTS[slot] == INSTAWIN) return; /* TEMP: INSTAWIN disabled */
    if (!shop_can_afford(SHOP_SLOTS[slot])) return;

    shop_clear_highlights();
    shop.selected_slot = slot;
    shop.selected      = SHOP_SLOTS[slot];
    if (W.shop_cards[slot])
        gtk_style_context_add_class(
            gtk_widget_get_style_context(W.shop_cards[slot]), "shop-card-selected");

    shop_show_icon_slot(slot);
    shop_advance_after_card_pick();
}

/* build + send the USE_SPECIAL_CARD action from the current shop selection */
static void shop_send_action(void)
{
    PlayerAction action;
    action.playerID       = C.my_player_id;
    action.move           = USE_SPECIAL_CARD;
    action.useSpecialCard = shop.selected;
    action.target         = shop.target;
    action.amount         = shop.my_card_idx | ((uint32_t)shop.opp_card_idx << 8);
    send_action(&C, &action);
}

/* PICK_TARGET — whole-opponent powerups (SWAP2 / SWAPOPS) */
gboolean shop_on_opponent_clicked(int gui_slot)
{
    if (!shop.open || shop.step != SHOP_STEP_PICK_TARGET) return FALSE;
    if (!shop_valid_target(gui_slot)) return FALSE;   //tile index == seat

    shop.target = (uint8_t)gui_slot;
    shop_send_action();
    shop_close();
    return TRUE;
}

/* PICK_MY_CARD — choose which of your cards (REDRAW applies now; SWAP1 advances) */
gboolean shop_on_my_card_clicked(int card_idx)
{
    if (!shop.open || shop.step != SHOP_STEP_PICK_MY_CARD) return FALSE;
    if (card_idx < 0 || card_idx >= HAND_SIZE) return FALSE;

    shop.my_card_idx = (uint8_t)card_idx;

    if (shop.selected == SWAP1) {
        shop_clear_highlights();
        shop.step = SHOP_STEP_PICK_OPP_CARD;
        shop_highlight_opp_cards();
        shop_set_prompt("Shop: click an opponent's card to swap for.");
        return TRUE;
    }

    //REDRAW
    shop_send_action();
    shop_close();
    return TRUE;
}

/* PICK_OPP_CARD — SWAP1: the clicked card identifies both target and which card */
gboolean shop_on_opp_card_clicked(int gui_slot, int card_idx)
{
    if (!shop.open || shop.step != SHOP_STEP_PICK_OPP_CARD) return FALSE;
    if (card_idx < 0 || card_idx >= HAND_SIZE) return FALSE;
    if (!shop_valid_target(gui_slot)) return FALSE;   //tile index == seat

    shop.target       = (uint8_t)gui_slot;
    shop.opp_card_idx = (uint8_t)card_idx;
    shop_send_action();
    shop_close();
    return TRUE;
}

void shop_on_confirm(void)
{
    if (!shop.open || shop.selected_slot < 0) return;
    if (!shop_is_available() || !shop_can_afford(shop.selected)) return;

    //REVEAL needs no target — apply immediately
    if (!shop_needs_target(shop.selected) && shop.selected != REDRAW) {
        shop_send_action();
        shop_close();
        return;
    }

    //otherwise enter on-table targeting: hide the dialog so the table is reachable
    if (W.shop) gtk_widget_hide(W.shop);

    if (shop.selected == REDRAW || shop.selected == SWAP1) {
        shop.step = SHOP_STEP_PICK_MY_CARD;
        shop_clear_highlights();
        shop_highlight_my_cards();
        shop_set_prompt(shop.selected == REDRAW
            ? "Shop: click one of your cards to redraw."
            : "Shop: click one of your cards to swap away.");
        return;
    }

    //SWAP2 / SWAPOPS — pick a whole opponent
    if (shop_valid_target_count() == 0) {
        if (W.shop) gtk_widget_show_all(W.shop);   //nothing to target; stay in picker
        refresh_shop_ui();
        shop_set_prompt("Shop: no opponent available to target.");
        return;
    }
    shop.step = SHOP_STEP_PICK_TARGET;
    shop_clear_highlights();
    shop_highlight_target_players();
    shop_set_prompt("Shop: click an opponent to target.");
}

void shop_on_back(void)
{
    shop_close();
}

void refresh_shop_ui(void)
{
    if (!W.shop) return;

    GameState *g = &C.game;
    for (int i = 0; i < 6; i++) {
        if (!W.shop_cards[i]) continue;
        Anteater_shop card = SHOP_SLOTS[i];

        if (card == INSTAWIN) {            /* TEMP: INSTAWIN hidden while disabled */
            gtk_widget_hide(W.shop_cards[i]);
            continue;
        }

        char label[64];
        uint32_t price = shop_card_price(g, card);
        snprintf(label, sizeof(label), "%s\n$%u", shop_card_name(card), price);
        gtk_button_set_label(GTK_BUTTON(W.shop_cards[i]), label);
        gtk_widget_set_sensitive(W.shop_cards[i], shop_can_afford(card));

        if (shop.selected_slot == i)
            gtk_style_context_add_class(
                gtk_widget_get_style_context(W.shop_cards[i]), "shop-card-selected");
        else
            gtk_style_context_remove_class(
                gtk_widget_get_style_context(W.shop_cards[i]), "shop-card-selected");
    }

    //Confirm is ready once a powerup is picked and affordable; target selection
    //(if any) happens on the table after Confirm.
    if (W.confirm_button)
        gtk_widget_set_sensitive(W.confirm_button,
            shop.selected_slot >= 0 && shop_can_afford(shop.selected));
}

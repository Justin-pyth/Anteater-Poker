/*  gui_extensions.c
 *
 *  Drop-in extra GUI elements for Anteater Poker (GTK 3).
 *  Include gui_extensions.h in gui.c and follow the integration
 *  steps described in the header.
 *
 *  All widget slots that need game/network logic are left empty;
 *  search for "TODO:" to find every hook-in point.
 */

#include "gui_extensions.h"
#include <stdio.h>
#include <string.h>

/* =========================================================================
   Internal CSS – appended to the existing CSS in gui.c
   Call  gtk_css_provider_load_from_data(prov, EXT_CSS, -1, NULL)
   with the SAME provider you already use, or a second one.
   ========================================================================= */
const char *EXT_CSS =

/* --- opponent seat timers --- */
".seat-timer-bar {"
"  min-height: 4px;"
"}"
".seat-timer-label {"
"  font-size: 10px;"
"  color: #ffd700;"
"}"

/* --- my turn timer --- */
"#my-timer-bar {"
"  min-height: 6px;"
"}"
"#my-timer-label {"
"  font-size: 12px;"
"  color: #ffd700;"
"  font-weight: bold;"
"}"

/* --- shop button --- */
"#btn-shop {"
"  background-color: #2a1a3a;"
"  border: 1px solid #9b59b6;"
"  border-radius: 8px;"
"  color: #c39bd3;"
"  font-family: 'Georgia', serif;"
"  font-size: 13px;"
"  font-weight: bold;"
"  letter-spacing: 1px;"
"  padding: 10px 18px;"
"}"
"#btn-shop:hover  { background-color: #3d2755; }"
"#btn-shop:active { background-color: #1e1228; }"

/* --- anteater deck panel --- */
"#anteater-panel {"
"  background-color: #161b22;"
"  border: 1px solid #30363d;"
"  border-radius: 10px;"
"  padding: 8px;"
"}"
"#anteater-title {"
"  font-size: 11px;"
"  color: #e6c87a;"
"  letter-spacing: 2px;"
"}"
"#anteater-count {"
"  font-size: 10px;"
"  color: #8b949e;"
"}"
"#btn-draw-anteater {"
"  background-color: #1a3a1a;"
"  border: 1px solid #27ae60;"
"  border-radius: 6px;"
"  color: #7ae890;"
"  font-family: 'Georgia', serif;"
"  font-size: 12px;"
"  padding: 6px 10px;"
"}"
"#btn-draw-anteater:hover { background-color: #234e23; }"

/* --- chat box --- */
"#chat-box {"
"  background-color: #161b22;"
"  border-top: 1px solid #30363d;"
"  padding: 6px 12px;"
"}"
"#chat-title {"
"  font-size: 11px;"
"  color: #e6c87a;"
"  letter-spacing: 2px;"
"}"
"#chat-log {"
"  background-color: #0d1117;"
"  color: #c9d1d9;"
"  font-family: 'Georgia', serif;"
"  font-size: 12px;"
"  border: 1px solid #30363d;"
"  border-radius: 6px;"
"  padding: 4px;"
"}"
"#chat-entry {"
"  background-color: #0d1117;"
"  color: #e6edf3;"
"  border: 1px solid #30363d;"
"  border-radius: 6px;"
"  font-family: 'Georgia', serif;"
"  font-size: 12px;"
"  padding: 6px 10px;"
"}"
"#btn-send-chat {"
"  background-color: #e6c87a;"
"  color: #0d1117;"
"  border: none;"
"  border-radius: 6px;"
"  font-family: 'Georgia', serif;"
"  font-size: 12px;"
"  font-weight: bold;"
"  padding: 6px 14px;"
"}"
"#btn-send-chat:hover { background-color: #f0d898; }";


/* =========================================================================
   Internal timer callback
   ========================================================================= */

typedef struct {
    SeatTimer *timer;
    int        is_my_timer;   /* 1 = local player, 0 = opponent seat        */
} TimerCBData;

gboolean timer_tick_cb(gpointer user_data)
{
    TimerCBData *cb = (TimerCBData *)user_data;
    SeatTimer   *t  = cb->timer;

    if (t->seconds_left <= 0) {
        /* Time expired – hide bar, clear label */
        gtk_widget_hide(t->bar);
        gtk_label_set_text(GTK_LABEL(t->label), "");
        t->timer_id = 0;
        g_free(cb);
        /* TODO: call your timeout / auto-fold handler here */
        return G_SOURCE_REMOVE;
    }

    t->seconds_left--;

    /* Update progress bar (fraction: 0.0 = empty, 1.0 = full) */
    double frac = (double)t->seconds_left / (double)t->turn_seconds;
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(t->bar), frac);

    /* Update label */
    char buf[16];
    snprintf(buf, sizeof(buf), "%d:%02d",
             t->seconds_left / 60, t->seconds_left % 60);
    gtk_label_set_text(GTK_LABEL(t->label), buf);

    return G_SOURCE_CONTINUE;
}

/* =========================================================================
   1. Six-seat opponent row
   =========================================================================
   Returns an hbox with 5 opponent frames (seats 0-4) arranged evenly.
   The widgets are stored in ext->opp_* arrays for later update by
   your refresh_ui().
   ========================================================================= */
GtkWidget *build_six_seat_row(GuiExtensions *ext)
{
    GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_halign(row, GTK_ALIGN_FILL);

    for (int i = 0; i < EXT_OPP_SEATS; i++) {

        /* Outer event box acts as the "frame" */
        GtkWidget *frame = gtk_event_box_new();
        GtkStyleContext *ctx = gtk_widget_get_style_context(frame);
        gtk_style_context_add_class(ctx, "opp-frame");
        ext->opp_frame[i] = frame;

        GtkWidget *col = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
        gtk_widget_set_margin_start(col, 6);
        gtk_widget_set_margin_end(col, 6);
        gtk_widget_set_margin_top(col, 6);
        gtk_widget_set_margin_bottom(col, 6);
        gtk_container_add(GTK_CONTAINER(frame), col);

        /* Two face-down card placeholders */
        GtkWidget *card_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 3);
        gtk_box_pack_start(GTK_BOX(col), card_row, FALSE, FALSE, 0);

        /* Simple coloured placeholder rectangles (replace with your
           make_card_widget() calls once you integrate) */
        for (int c = 0; c < 2; c++) {
            GtkWidget *placeholder = gtk_drawing_area_new();
            gtk_widget_set_size_request(placeholder, 24, 36);
            /* TODO: connect "draw" signal to draw_card_cb with face-down data */
            gtk_box_pack_start(GTK_BOX(card_row), placeholder, FALSE, FALSE, 0);
        }

        /* Player name */
        ext->opp_name[i] = gtk_label_new("Empty");
        gtk_style_context_add_class(
            gtk_widget_get_style_context(ext->opp_name[i]), "opp-name");
        gtk_widget_set_halign(ext->opp_name[i], GTK_ALIGN_START);
        gtk_box_pack_start(GTK_BOX(col), ext->opp_name[i], FALSE, FALSE, 0);

        /* Chips / bet */
        ext->opp_chips[i] = gtk_label_new("$0  |  bet $0");
        gtk_style_context_add_class(
            gtk_widget_get_style_context(ext->opp_chips[i]), "opp-chips");
        gtk_widget_set_halign(ext->opp_chips[i], GTK_ALIGN_START);
        gtk_box_pack_start(GTK_BOX(col), ext->opp_chips[i], FALSE, FALSE, 0);

        /* Status */
        ext->opp_status[i] = gtk_label_new("Waiting");
        gtk_style_context_add_class(
            gtk_widget_get_style_context(ext->opp_status[i]), "opp-status");
        gtk_widget_set_halign(ext->opp_status[i], GTK_ALIGN_START);
        gtk_box_pack_start(GTK_BOX(col), ext->opp_status[i], FALSE, FALSE, 0);

        /* --- Timer row (bar + label) --- */
        GtkWidget *timer_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
        gtk_box_pack_start(GTK_BOX(col), timer_row, FALSE, FALSE, 2);

        ext->opp_timer[i].bar = gtk_progress_bar_new();
        gtk_style_context_add_class(
            gtk_widget_get_style_context(ext->opp_timer[i].bar),
            "seat-timer-bar");
        gtk_progress_bar_set_fraction(
            GTK_PROGRESS_BAR(ext->opp_timer[i].bar), 0.0);
        gtk_widget_set_hexpand(ext->opp_timer[i].bar, TRUE);
        gtk_widget_hide(ext->opp_timer[i].bar);   /* hidden until turn */
        gtk_box_pack_start(GTK_BOX(timer_row),
                           ext->opp_timer[i].bar, TRUE, TRUE, 0);

        ext->opp_timer[i].label = gtk_label_new("");
        gtk_style_context_add_class(
            gtk_widget_get_style_context(ext->opp_timer[i].label),
            "seat-timer-label");
        gtk_box_pack_start(GTK_BOX(timer_row),
                           ext->opp_timer[i].label, FALSE, FALSE, 0);

        ext->opp_timer[i].timer_id    = 0;
        ext->opp_timer[i].seconds_left = 0;
        ext->opp_timer[i].turn_seconds = 30;

        /* Distribute evenly */
        gtk_box_pack_start(GTK_BOX(row), frame, TRUE, TRUE, 4);
    }

    return row;
}

/* =========================================================================
   2. Anteater deck side panel
   =========================================================================
   Returns a narrow vertical box you can pack to the right of the felt.
   ========================================================================= */
GtkWidget *build_anteater_deck_panel(GuiExtensions *ext)
{
    GtkWidget *panel = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_widget_set_name(panel, "anteater-panel");
    gtk_widget_set_size_request(panel, 110, -1);
    gtk_widget_set_margin_start(panel, 4);
    gtk_widget_set_margin_end(panel, 4);
    gtk_widget_set_margin_top(panel, 8);
    gtk_widget_set_margin_bottom(panel, 8);
    ext->anteater_panel = panel;

    /* Title */
    GtkWidget *title = gtk_label_new("ANTEATER\nDECK");
    gtk_widget_set_name(title, "anteater-title");
    gtk_label_set_justify(GTK_LABEL(title), GTK_JUSTIFY_CENTER);
    gtk_box_pack_start(GTK_BOX(panel), title, FALSE, FALSE, 0);

    /* Card face drawing area – wire "draw" signal to your card renderer */
    GtkWidget *deck_area = gtk_drawing_area_new();
    gtk_widget_set_size_request(deck_area, 70, 100);
    ext->anteater_deck_draw_area = deck_area;
    /* TODO: g_signal_connect(deck_area, "draw", G_CALLBACK(draw_card_cb), &anteater_top_card); */
    gtk_widget_set_halign(deck_area, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(panel), deck_area, FALSE, FALSE, 4);

    /* Card count label */
    ext->anteater_count_label = gtk_label_new("Anteater Deck:\n0 cards");
    gtk_widget_set_name(ext->anteater_count_label, "anteater-count");
    gtk_label_set_justify(GTK_LABEL(ext->anteater_count_label), GTK_JUSTIFY_CENTER);
    gtk_box_pack_start(GTK_BOX(panel), ext->anteater_count_label, FALSE, FALSE, 0);

    /* Draw button */
    ext->btn_draw_anteater = gtk_button_new_with_label("Draw");
    gtk_widget_set_name(ext->btn_draw_anteater, "btn-draw-anteater");
    gtk_widget_set_sensitive(ext->btn_draw_anteater, FALSE);  /* enable on your turn */
    /* TODO: g_signal_connect(ext->btn_draw_anteater, "clicked",
                              G_CALLBACK(on_draw_anteater_clicked), NULL); */
    gtk_box_pack_start(GTK_BOX(panel), ext->btn_draw_anteater, FALSE, FALSE, 0);

    return panel;
}

/* =========================================================================
   3. Shop button
   =========================================================================
   Returns a styled GtkButton.  Pack it into btn_row inside
   build_game_screen() after the existing action buttons.
   ========================================================================= */
GtkWidget *build_shop_button(GuiExtensions *ext)
{
    ext->btn_shop = gtk_button_new_with_label("Shop  🛒");
    gtk_widget_set_name(ext->btn_shop, "btn-shop");
    gtk_style_context_add_class(
        gtk_widget_get_style_context(ext->btn_shop), "action-btn");

    /* TODO: g_signal_connect(ext->btn_shop, "clicked",
                              G_CALLBACK(on_shop_clicked), NULL); */

    return ext->btn_shop;
}

/* =========================================================================
   4. Chat box
   =========================================================================
   Returns a compact chat panel.  Pack it at the bottom of root (the
   outermost vertical box in build_game_screen()).
   ========================================================================= */

/* Internal: send button / Enter key handler */
void on_send_chat_clicked(GtkButton *btn, gpointer user_data)
{
    (void)btn;
    GuiExtensions *ext = (GuiExtensions *)user_data;
    const char *text = gtk_entry_get_text(GTK_ENTRY(ext->chat_entry));
    if (!text || text[0] == '\0') return;

    /* TODO: replace "Me" with the local player's name from your ClientState */
    ext_append_chat(ext, "Me", text);

    /* TODO: call your network send function here, e.g.:
       send_chat_message(&C, text); */

    gtk_entry_set_text(GTK_ENTRY(ext->chat_entry), "");
}

void on_chat_entry_activate(GtkEntry *entry, gpointer user_data)
{
    (void)entry;
    /* Pressing Enter in the text field triggers the send button */
    GuiExtensions *ext = (GuiExtensions *)user_data;
    gtk_button_clicked(GTK_BUTTON(ext->btn_send_chat));
}

GtkWidget *build_chat_box(GuiExtensions *ext)
{
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_widget_set_name(box, "chat-box");
    gtk_widget_set_size_request(box, -1, 140);
    gtk_widget_set_margin_start(box, 12);
    gtk_widget_set_margin_end(box, 12);
    gtk_widget_set_margin_bottom(box, 8);
    ext->chat_box = box;

    /* Title */
    GtkWidget *title = gtk_label_new("CHAT");
    gtk_widget_set_name(title, "chat-title");
    gtk_widget_set_halign(title, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(box), title, FALSE, FALSE, 0);

    /* Scrolled text view */
    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
                                   GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_vexpand(scroll, TRUE);
    gtk_box_pack_start(GTK_BOX(box), scroll, TRUE, TRUE, 0);

    ext->chat_log = gtk_text_view_new();
    gtk_widget_set_name(ext->chat_log, "chat-log");
    gtk_text_view_set_editable(GTK_TEXT_VIEW(ext->chat_log), FALSE);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(ext->chat_log), FALSE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(ext->chat_log), GTK_WRAP_WORD_CHAR);
    gtk_container_add(GTK_CONTAINER(scroll), ext->chat_log);

    /* Input row */
    GtkWidget *input_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_box_pack_start(GTK_BOX(box), input_row, FALSE, FALSE, 0);

    ext->chat_entry = gtk_entry_new();
    gtk_widget_set_name(ext->chat_entry, "chat-entry");
    gtk_entry_set_placeholder_text(GTK_ENTRY(ext->chat_entry), "Type a message…");
    gtk_widget_set_hexpand(ext->chat_entry, TRUE);
    g_signal_connect(ext->chat_entry, "activate",
                     G_CALLBACK(on_chat_entry_activate), ext);
    gtk_box_pack_start(GTK_BOX(input_row), ext->chat_entry, TRUE, TRUE, 0);

    ext->btn_send_chat = gtk_button_new_with_label("Send");
    gtk_widget_set_name(ext->btn_send_chat, "btn-send-chat");
    g_signal_connect(ext->btn_send_chat, "clicked",
                     G_CALLBACK(on_send_chat_clicked), ext);
    gtk_box_pack_start(GTK_BOX(input_row), ext->btn_send_chat, FALSE, FALSE, 0);

    return box;
}

/* =========================================================================
   Timer control
   ========================================================================= */

void ext_start_player_timer(GuiExtensions *ext, int seat, int seconds)
{
    if (seat < 0 || seat >= EXT_OPP_SEATS) return;
    SeatTimer *t = &ext->opp_timer[seat];

    /* Cancel any existing timer for this seat */
    if (t->timer_id) {
        g_source_remove(t->timer_id);
        t->timer_id = 0;
    }

    t->turn_seconds = seconds;
    t->seconds_left = seconds;

    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(t->bar), 1.0);
    gtk_widget_show(t->bar);

    char buf[16];
    snprintf(buf, sizeof(buf), "%d:%02d", seconds / 60, seconds % 60);
    gtk_label_set_text(GTK_LABEL(t->label), buf);

    TimerCBData *cb = g_new0(TimerCBData, 1);
    cb->timer       = t;
    cb->is_my_timer = 0;
    t->timer_id = g_timeout_add_seconds(1, timer_tick_cb, cb);
}

void ext_stop_player_timer(GuiExtensions *ext, int seat)
{
    if (seat < 0 || seat >= EXT_OPP_SEATS) return;
    SeatTimer *t = &ext->opp_timer[seat];

    if (t->timer_id) {
        g_source_remove(t->timer_id);
        t->timer_id = 0;
    }
    gtk_widget_hide(t->bar);
    gtk_label_set_text(GTK_LABEL(t->label), "");
}

void ext_start_my_timer(GuiExtensions *ext, int seconds)
{
    SeatTimer *t = &ext->my_timer;

    if (t->timer_id) {
        g_source_remove(t->timer_id);
        t->timer_id = 0;
    }

    t->turn_seconds = seconds;
    t->seconds_left = seconds;

    if (t->bar) {
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(t->bar), 1.0);
        gtk_widget_show(t->bar);
    }
    if (t->label) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%d:%02d", seconds / 60, seconds % 60);
        gtk_label_set_text(GTK_LABEL(t->label), buf);
    }

    TimerCBData *cb = g_new0(TimerCBData, 1);
    cb->timer       = t;
    cb->is_my_timer = 1;
    t->timer_id = g_timeout_add_seconds(1, timer_tick_cb, cb);
}

void ext_stop_my_timer(GuiExtensions *ext)
{
    SeatTimer *t = &ext->my_timer;
    if (t->timer_id) {
        g_source_remove(t->timer_id);
        t->timer_id = 0;
    }
    if (t->bar)   gtk_widget_hide(t->bar);
    if (t->label) gtk_label_set_text(GTK_LABEL(t->label), "");
}

/* =========================================================================
   Chat helpers
   ========================================================================= */

void ext_append_chat(GuiExtensions *ext, const char *sender, const char *msg)
{
    if (!ext->chat_log) return;

    GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(ext->chat_log));
    GtkTextIter end;
    gtk_text_buffer_get_end_iter(buf, &end);

    /* Format: "Name: message\n" */
    char line[512];
    snprintf(line, sizeof(line), "%s: %s\n", sender ? sender : "?", msg ? msg : "");
    gtk_text_buffer_insert(buf, &end, line, -1);

    /* Auto-scroll to bottom */
    gtk_text_buffer_get_end_iter(buf, &end);
    GtkTextMark *mark = gtk_text_buffer_get_mark(buf, "insert");
    gtk_text_buffer_move_mark(buf, mark, &end);
    gtk_text_view_scroll_mark_onscreen(GTK_TEXT_VIEW(ext->chat_log), mark);
}

/* =========================================================================
   Anteater deck helpers
   ========================================================================= */

void ext_set_anteater_count(GuiExtensions *ext, int count)
{
    if (!ext->anteater_count_label) return;
    char buf[64];
    snprintf(buf, sizeof(buf), "Anteater Deck:\n%d card%s", count, count == 1 ? "" : "s");
    gtk_label_set_text(GTK_LABEL(ext->anteater_count_label), buf);
}
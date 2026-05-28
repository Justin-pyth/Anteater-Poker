#include "gui.h"
#include "gui_helpers.h"

/* -- init_gui -------------------------------------------------------------- */
void init_gui(int *argc, char ***argv)
{
    gtk_init(argc, argv);
    init_client_state(&C);

    GtkCssProvider *css = gtk_css_provider_new();
    gtk_css_provider_load_from_data(css, CSS, -1, NULL);
    gtk_style_context_add_provider_for_screen(
        gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(css),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    GtkBuilder *builder = gtk_builder_new_from_file("anteater_poker.glade");
    gtk_builder_connect_signals(builder, NULL);

#define GET(id) GTK_WIDGET(gtk_builder_get_object(builder, (id)))

    W.window           = GET("window");
    W.stack            = GET("stack");
    W.login_screen     = GET("login_screen");
    W.name_entry       = GET("name_entry");
    W.host_entry       = GET("host_entry");
    W.port_entry       = GET("port_entry");
    W.login_status     = GET("login_status");
    W.game_screen      = GET("game_screen");
    W.pot_label        = GET("pot_label");
    W.stage_label      = GET("stage_label");
    W.log_label        = GET("log_label");
    W.deck_count_label = GET("deck_count_label");
    W.btn_fold         = GET("btn_fold");
    W.btn_check        = GET("btn_check");
    W.btn_call         = GET("btn_call");
    W.btn_raise        = GET("btn_raise");
    W.btn_shop         = GET("btn_shop");
    W.raise_spin       = GET("raise_spin");
    W.anteater_panel        = GET("anteater_panel");
    W.anteater_count_label  = GET("anteater_count_label");
    W.btn_draw_anteater     = GET("btn_draw_anteater");
    W.chat_log       = GET("chat_log");
    W.chat_entry     = GET("chat_entry");
    W.btn_send_chat  = GET("btn_send_chat");
    W.label_call_amnt = GET("label_call_amnt");
    W.label_your_stack = GET("label_your_stack");

    for (int i = 0; i < 5; i++) {
        char id[32];
        snprintf(id, sizeof(id), "community_card_%d", i);
        W.community_cards[i] = GET(id);
        init_card_widget(W.community_cards[i]);
    }
    for (int i = 0; i < 2; i++) {
        char id[32];
        snprintf(id, sizeof(id), "my_card_%d", i);
        W.my_cards[i] = GET(id);
        init_card_widget(W.my_cards[i]);
    }
    for (int i = 0; i < GUI_OPPONENT_SLOTS; i++) {
        char id[64];
        snprintf(id, sizeof(id), "opp_frame_%d",  i); W.opp_frame[i]  = GET(id);
        snprintf(id, sizeof(id), "opp_name_%d",   i); W.opp_name[i]   = GET(id);
        snprintf(id, sizeof(id), "opp_chips_%d",  i); W.opp_chips[i]  = GET(id);
        snprintf(id, sizeof(id), "opp_status_%d", i); W.opp_status[i] = GET(id);

        snprintf(id, sizeof(id), "opp_timer_bar_%d", i);
        W.opp_timer[i].bar = GET(id);
        snprintf(id, sizeof(id), "opp_timer_lbl_%d", i);
        W.opp_timer[i].label = GET(id);
        W.opp_timer[i].timer_id     = 0;
        W.opp_timer[i].seconds_left = 0;
        W.opp_timer[i].turn_seconds = TURN_SECONDS;
        gtk_widget_hide(W.opp_timer[i].bar);

        char cid[64];
        snprintf(cid, sizeof(cid), "opp_card_%da", i); init_card_widget(GET(cid));
        snprintf(cid, sizeof(cid), "opp_card_%db", i); init_card_widget(GET(cid));
    }

    W.my_timer.bar          = GET("my_timer_bar");
    W.my_timer.label        = GET("my_timer_lbl");
    W.my_timer.timer_id     = 0;
    W.my_timer.seconds_left = 0;
    W.my_timer.turn_seconds = TURN_SECONDS;
    gtk_widget_hide(W.my_timer.bar);

#undef GET
    g_object_unref(builder);
    gtk_widget_show_all(W.window);
}

/* -- main ------------------------------------------------------------------ */
int main(int argc, char *argv[])
{
    init_gui(&argc, &argv);
    gtk_main();
    return 0;
}

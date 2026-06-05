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
    W.btn_allin        = GET("btn_ALLIN");
    W.btn_shop         = GET("btn_shop");
    W.raise_spin       = GET("raise_spin");
    W.anteater_panel        = GET("anteater_panel");
    W.anteater_count_label  = GET("anteater_count_label");
    W.btn_draw_anteater     = GET("btn_draw_anteater");
    W.chat_log       = GET("chat_log");
    W.chat_entry     = GET("chat_entry");
    W.btn_send_chat  = GET("btn_send_chat");
    W.ready_btn      = GET("ready_btn");
    W.quit_btn       = GET("quit_btn");
    W.label_call_amnt = GET("label_call_amnt");
    W.label_your_stack = GET("label_your_stack");
    W.leaderboard = GET("leaderboard");
    W.shop          = GET("shop");
    W.shop_text     = GET("shop_text");
    W.confirm_button = GET("confirm_button");
    W.back_button   = GET("back_button");
    for (int i = 0; i < 6; i++) {
        char id[16];
        snprintf(id, sizeof(id), "card%d", i + 1);
        W.shop_cards[i] = GET(id);
    }
    if (W.leaderboard)
        g_signal_connect(W.leaderboard, "delete-event", G_CALLBACK(gtk_widget_hide_on_delete), NULL);
    shop_init_dialog();
    if (W.shop_text)
        gtk_editable_set_editable(GTK_EDITABLE(W.shop_text), FALSE);
    if (W.btn_shop)
        g_signal_connect(W.btn_shop, "clicked", G_CALLBACK(on_shop_clicked), NULL);
    if (W.back_button)
        g_signal_connect(W.back_button, "clicked", G_CALLBACK(on_shop_back), NULL);
    if (W.confirm_button)
        g_signal_connect(W.confirm_button, "clicked", G_CALLBACK(on_shop_confirm), NULL);
    for (int i = 0; i < 6; i++) {
        if (W.shop_cards[i])
            g_signal_connect(W.shop_cards[i], "clicked",
                             G_CALLBACK(on_shop_card_clicked), GINT_TO_POINTER(i));
    }
    if (W.ready_btn)
        g_signal_connect(W.ready_btn, "clicked", G_CALLBACK(on_ready_clicked), NULL);
    if (W.quit_btn)
        g_signal_connect(W.quit_btn, "clicked", G_CALLBACK(on_quit_clicked), NULL);
    for(int i = 0; i < MAX_PLAYERS; i++)
    {
        char id[32];
        snprintf(id, sizeof(id), "leaderBoard_Name%d",  i+1); W.lb_name[i]    = GET(id);
        snprintf(id, sizeof(id), "leaderBoard_Place%d", i+1); W.lb_place[i]   = GET(id);
        snprintf(id, sizeof(id), "card_0_%d",           i+1); W.lb_card[i][0] = GET(id);
        snprintf(id, sizeof(id), "card_1_%d",           i+1); W.lb_card[i][1] = GET(id);
        init_card_widget(W.lb_card[i][0]);
        init_card_widget(W.lb_card[i][1]);
    }

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
        g_signal_connect(W.my_cards[i], "button-press-event",
                         G_CALLBACK(on_shop_my_card_press), GINT_TO_POINTER(i));
    }
    W.self_blind = GET("self_blind");
    init_blind_widget(W.self_blind);
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
        W.opp_timer[i].is_my_timer  = FALSE;
        gtk_widget_hide(W.opp_timer[i].bar);

        char cid[64];
        snprintf(cid, sizeof(cid), "opp_card_%da", i);
        W.opp_cards[i][0] = GET(cid);
        init_card_widget(W.opp_cards[i][0]);
        g_signal_connect(W.opp_cards[i][0], "button-press-event",
                         G_CALLBACK(on_shop_opp_card_press), GINT_TO_POINTER(i * 2));
        snprintf(cid, sizeof(cid), "opp_card_%db", i);
        W.opp_cards[i][1] = GET(cid);
        init_card_widget(W.opp_cards[i][1]);
        g_signal_connect(W.opp_cards[i][1], "button-press-event",
                         G_CALLBACK(on_shop_opp_card_press), GINT_TO_POINTER(i * 2 + 1));
        g_signal_connect(W.opp_frame[i], "button-press-event",
                         G_CALLBACK(on_shop_opp_frame_press), GINT_TO_POINTER(i));

        snprintf(cid, sizeof(cid), "opp_blind_%d", i);
        W.opp_blind[i] = GET(cid);
        init_blind_widget(W.opp_blind[i]);
    }

    W.my_timer.bar          = GET("my_timer_bar");
    W.my_timer.label        = GET("my_timer_lbl");
    W.my_timer.timer_id     = 0;
    W.my_timer.seconds_left = 0;
    W.my_timer.turn_seconds = TURN_SECONDS;
    W.my_timer.is_my_timer  = TRUE;
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

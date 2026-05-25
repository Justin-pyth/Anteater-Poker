#include "gui.h"

/* -- Build: login screen --------------------------------------------------- */
GtkWidget *build_login_screen_widget(void)
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

#define FIELD(lbl_text, entry_ptr, ph, def) do { \
    GtkWidget *lbl = gtk_label_new(lbl_text); \
    gtk_widget_set_halign(lbl, GTK_ALIGN_START); \
    gtk_style_context_add_class(gtk_widget_get_style_context(lbl), "field-lbl"); \
    gtk_box_pack_start(GTK_BOX(card), lbl, FALSE, FALSE, 10); \
    entry_ptr = gtk_entry_new(); \
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry_ptr), ph); \
    if ((def)[0]) gtk_entry_set_text(GTK_ENTRY(entry_ptr), def); \
    gtk_box_pack_start(GTK_BOX(card), entry_ptr, FALSE, FALSE, 6); \
} while(0)

    FIELD("PLAYER NAME", W.name_entry, "Enter your name", "");
    gtk_entry_set_max_length(GTK_ENTRY(W.name_entry), 31);

    GtkWidget *btn = gtk_button_new_with_label("PLAY  ♠  vs Bots");
    gtk_widget_set_name(btn, "connect-btn");
    gtk_box_pack_start(GTK_BOX(card), btn, FALSE, FALSE, 20);
    g_signal_connect(btn, "clicked", G_CALLBACK(on_play_clicked), NULL);

    GtkWidget *divider = gtk_label_new("────  OR  ────");
    gtk_widget_set_name(divider, "divider-label");
    gtk_widget_set_halign(divider, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(card), divider, FALSE, FALSE, 6);

    FIELD("SERVER HOST", W.host_entry, "localhost", "localhost");
    FIELD("PORT",        W.port_entry, "10160",     "10160");
#undef FIELD

    GtkWidget *conn_btn = gtk_button_new_with_label("CONNECT TO SERVER");
    gtk_widget_set_name(conn_btn, "offline-btn");
    gtk_box_pack_start(GTK_BOX(card), conn_btn, FALSE, FALSE, 10);
    g_signal_connect(conn_btn, "clicked", G_CALLBACK(on_connect_clicked), NULL);

    W.login_status = gtk_label_new("");
    gtk_widget_set_name(W.login_status, "login-status");
    gtk_label_set_line_wrap(GTK_LABEL(W.login_status), TRUE);
    gtk_box_pack_start(GTK_BOX(card), W.login_status, FALSE, FALSE, 0);

    return outer;
}

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

    W.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(W.window), "Anteater Poker");
    gtk_window_set_default_size(GTK_WINDOW(W.window), 420, 520);
    gtk_window_set_position(GTK_WINDOW(W.window), GTK_WIN_POS_CENTER);
    g_signal_connect(W.window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    W.stack = gtk_stack_new();
    gtk_container_add(GTK_CONTAINER(W.window), W.stack);

    W.login_screen = build_login_screen_widget();
    W.game_screen  = build_game_screen_widget();

    gtk_stack_add_named(GTK_STACK(W.stack), W.login_screen, "login");
    gtk_stack_add_named(GTK_STACK(W.stack), W.game_screen,  "game");
    gtk_stack_set_visible_child_name(GTK_STACK(W.stack), "login");

    gtk_widget_show_all(W.window);
    gtk_widget_hide(W.game_screen);
}

/* -- main ------------------------------------------------------------------ */
int main(int argc, char *argv[])
{
    init_gui(&argc, &argv);
    gtk_main();
    return 0;
}

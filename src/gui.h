#ifndef TEMP_GUI_H
#define TEMP_GUI_H

#include "gui_assets.h"
#include "gui_helpers.h"

/* -- Lifecycle ------------------------------------------------------------- */
void init_gui(int *argc, char ***argv);

/* -- Screen transitions ---------------------------------------------------- */
void show_game_screen(void);
void show_login_screen(void);

/* -- Display --------------------------------------------------------------- */
void refresh_ui(void);
void infer_my_player_id(void);

/* -- Last Hand Info side panel --------------------------------------------- */
void build_last_hand_panel(GtkBuilder *builder);
void update_last_hand_panel(void);
void on_last_hand_toggle(GtkButton *b, gpointer d);

/* -- Chat ------------------------------------------------------------------ */
void append_chat(const char *sender, const char *msg, const char *tag_name);


/* -- Server IO ------------------------------------------------------------- */
gboolean on_server_data(GIOChannel *ch, GIOCondition cond, gpointer data);

/* -- Action callbacks ------------------------------------------------------ */
void on_fold(GtkButton *b, gpointer d);
void on_check(GtkButton *b, gpointer d);
void on_call(GtkButton *b, gpointer d);
void on_raise(GtkButton *b, gpointer d);
void on_allin(GtkButton *b, gpointer d);

/* -- Connect / login callbacks --------------------------------------------- */
void on_play_clicked(GtkButton *b, gpointer d);
void on_connect_clicked(GtkButton *b, gpointer d);

/* -- Chat callbacks -------------------------------------------------------- */
void on_send_chat(GtkButton *b, gpointer d);
void on_chat_activate(GtkEntry *e, gpointer d);
void on_ready_clicked(GtkButton *b, gpointer d);
void on_quit_clicked(GtkButton *b, gpointer d);

/* -- Shop callbacks -------------------------------------------------------- */
void on_shop_clicked(GtkButton *b, gpointer d);
void on_shop_back(GtkButton *b, gpointer d);
void on_shop_confirm(GtkButton *b, gpointer d);
void on_shop_card_clicked(GtkButton *b, gpointer d);
gboolean on_shop_my_card_press(GtkWidget *w, GdkEventButton *ev, gpointer d);
gboolean on_shop_opp_frame_press(GtkWidget *w, GdkEventButton *ev, gpointer d);
gboolean on_shop_opp_card_press(GtkWidget *w, GdkEventButton *ev, gpointer d);

#endif /* TEMP_GUI_H */

// Declaration of all the different hands in poker
#ifndef GUI_H
#define GUI_H

#include <string.h>
#include <sys/select.h>
#include "protocol.h"
#include "gui_assets.h"
#include "gui_helpers.h"

/* -- Lifecycle ------------------------------------------------------------- */
void init_gui(int *argc, char ***argv);

/* -- Screen transitions ---------------------------------------------------- */
void show_game_screen(void);
void show_login_screen(void);

/* -- Builders -------------------------------------------------------------- */
GtkWidget *build_game_screen_widget(void);
GtkWidget *build_login_screen_widget(void);
void       build_seat_timer(GtkWidget *col, SeatTimer *t,
                            const char *bar_name, const char *lbl_class);

/* -- Display --------------------------------------------------------------- */
void refresh_ui(void);
void infer_my_player_id(void);

/* -- Chat ------------------------------------------------------------------ */
void append_chat(const char *sender, const char *msg);

/* -- Anteater deck --------------------------------------------------------- */
void set_anteater_count(int count);

/* -- Server IO ------------------------------------------------------------- */
gboolean on_server_data(GIOChannel *ch, GIOCondition cond, gpointer data);

/* -- Action callbacks ------------------------------------------------------ */
void on_fold(GtkButton *b, gpointer d);
void on_check(GtkButton *b, gpointer d);
void on_call(GtkButton *b, gpointer d);
void on_raise(GtkButton *b, gpointer d);
void on_ready(GtkButton *b, gpointer d);

/* -- Connect / login callbacks --------------------------------------------- */
void on_play_clicked(GtkButton *b, gpointer d);
void on_connect_clicked(GtkButton *b, gpointer d);

/* -- Chat callbacks -------------------------------------------------------- */
void on_send_chat(GtkButton *b, gpointer d);
void on_chat_activate(GtkEntry *e, gpointer d);

#endif /* TEMP_GUI_H */

#ifndef GUI_EXTENSIONS_H
#define GUI_EXTENSIONS_H

#include <gtk/gtk.h>

/* =========================================================================
   gui_extensions.h  –  Extra GUI elements for Anteater Poker
   =========================================================================
   Features added:
     1. Per-player countdown timers  (6 seats)
     3. Shop button in the action bar
     4. Anteater-card side deck panel
     5. Chat box for player communication
   =========================================================================

#define EXT_OPP_SEATS 5   /* 5 opponents around the table */

/* -------------------------------------------------------------------------
   Per-seat countdown timer state
   ------------------------------------------------------------------------- */
typedef struct {
    GtkWidget *bar;          /* GtkProgressBar – visual countdown            */
    GtkWidget *label;        /* "0:28" text beside the bar                   */
    guint      timer_id;     /* g_timeout_add source id; 0 = not running     */
    gint       seconds_left; /* current value counting down                  */
    gint       turn_seconds; /* full turn length (set when timer starts)     */
} SeatTimer;

/* -------------------------------------------------------------------------
   Master extensions struct
   Keep exactly one of these alongside your AppWidgets.
   ------------------------------------------------------------------------- */
typedef struct {

    /* local player timer */
    SeatTimer  my_timer;

    /* shop */
    GtkWidget *btn_shop;

    /* anteater side deck */
    GtkWidget *anteater_panel;
    GtkWidget *anteater_deck_draw_area;  /* GtkDrawingArea – top card face   */
    GtkWidget *anteater_count_label;
    GtkWidget *btn_draw_anteater;        /* wire to your draw logic          */

    /* chat */
    GtkWidget *chat_box;
    GtkWidget *chat_log;         /* GtkTextView inside a scrolled window     */
    GtkWidget *chat_entry;
    GtkWidget *btn_send_chat;    /* wire to your send logic                  */

} GuiExtensions;

/* =========================================================================
   Public function declarations
   ========================================================================= */

/* Builders – call once from build_game_screen() */
GtkWidget *build_anteater_deck_panel(GuiExtensions *ext);
GtkWidget *build_shop_button        (GuiExtensions *ext);
GtkWidget *build_chat_box           (GuiExtensions *ext);

/* Timer control */
void ext_start_player_timer(GuiExtensions *ext, int seat, int seconds);
void ext_stop_player_timer (GuiExtensions *ext, int seat);
void ext_start_my_timer    (GuiExtensions *ext, int seconds);
void ext_stop_my_timer     (GuiExtensions *ext);

/* Helpers called from your logic */
void ext_append_chat       (GuiExtensions *ext, const char *sender, const char *msg);
void ext_set_anteater_count(GuiExtensions *ext, int count);

#endif /* GUI_EXTENSIONS_H */
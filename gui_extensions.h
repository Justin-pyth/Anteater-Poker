#ifndef GUI_EXTENSIONS_H
#define GUI_EXTENSIONS_H

#include <gtk/gtk.h>

/* =========================================================================
   gui_extensions.h  –  Extra GUI elements for Anteater Poker
   =========================================================================
   Features added:
     1. Per-player countdown timers  (6 seats)
     2. Two extra player seats       (total = 6, up from 4 visible)
     3. Shop button in the action bar
     4. Anteater-card side deck panel
     5. Chat box for player communication
   =========================================================================
   HOW TO USE IN gui.c / main()
   -------------------------------------------------------------------------
     Step 1 – include this header at the top of gui.c:
       #include "gui_extensions.h"

     Step 2 – declare ONE global instance next to AppWidgets:
       static GuiExtensions EXT;

     Step 3 – inside build_game_screen(), replace the 3-seat opp_row block
               with:
               GtkWidget *opp_row = build_six_seat_row(&EXT);
               gtk_box_pack_start(GTK_BOX(main_area), opp_row, FALSE, FALSE, 4);

     Step 4 – wrap the felt in an hbox to add the side deck panel:
               GtkWidget *table_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
               gtk_box_pack_start(GTK_BOX(table_row), felt, TRUE, TRUE, 0);
               gtk_box_pack_start(GTK_BOX(table_row),
                                  build_anteater_deck_panel(&EXT),
                                  FALSE, FALSE, 0);
               gtk_box_pack_start(GTK_BOX(main_area), table_row, TRUE, TRUE, 0);

     Step 5 – add shop button to btn_row (after the existing buttons):
               gtk_box_pack_start(GTK_BOX(btn_row),
                                  build_shop_button(&EXT), FALSE, FALSE, 0);

     Step 6 – add chat box below action buttons:
               gtk_box_pack_start(GTK_BOX(root),
                                  build_chat_box(&EXT), FALSE, FALSE, 0);

     Step 7 – in refresh_ui(), start/stop timers per turn:
               ext_start_player_timer(&EXT, opp_slot, 30);
               ext_stop_player_timer (&EXT, opp_slot);
               ext_start_my_timer    (&EXT, 30);
               ext_stop_my_timer     (&EXT);
   ========================================================================= */

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

    /* 5 opponent seat widgets */
    GtkWidget *opp_frame [EXT_OPP_SEATS];
    GtkWidget *opp_name  [EXT_OPP_SEATS];
    GtkWidget *opp_chips [EXT_OPP_SEATS];
    GtkWidget *opp_status[EXT_OPP_SEATS];
    SeatTimer  opp_timer [EXT_OPP_SEATS];

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
GtkWidget *build_six_seat_row       (GuiExtensions *ext);
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
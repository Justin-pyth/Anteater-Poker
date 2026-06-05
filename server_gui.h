#ifndef SERVER_GUI_H
#define SERVER_GUI_H

#include <gtk/gtk.h>
#include "protocol.h"   /* ServerState, GameState, broadcast_game_state, start_new_hand, addBot */

/*  server_gui.h  —  GTK monitor/admin window for the Anteater Poker server.
    ----------------------------------------------------------------------
    The server keeps its own select() loop; this GUI does NOT take over with
    gtk_main(). Instead the loop calls server_gui_pump() every iteration to
    process pending GTK events, and server_gui_refresh() to redraw state.

    Integration (server.c):
      1.  gtk_init(&argc, &argv);            // before init_server
      2.  server_gui_init(&state);           // after the listen socket is set
      3.  top of the while(running) loop:
              server_gui_pump();
              server_gui_refresh(&state.game);
      4.  give select() a short timeout (e.g. 50 ms) so the loop wakes often
          enough to keep the window responsive.

    NOTE: this is a GTK window, so it needs a display. Over plain SSH it will
    fail with "cannot open display" exactly like the client — run it on a
    machine with a display or with X11 forwarding enabled.
*/

/* Build and show the monitor window. `state` is stored for the button
   callbacks (Restart / Add Bots / New Hand / Quit). Call gtk_init() first. */
void server_gui_init(ServerState *state);

/* Redraw every field from the current game state. Safe to call every loop. */
void server_gui_refresh(const GameState *gs);

/* Process pending GTK events without blocking. Call every loop iteration. */
void server_gui_pump(void);

/* Append a printf-style line to the on-screen event log. */
void server_gui_log(const char *fmt, ...);

#endif /* SERVER_GUI_H */

#ifndef TEMP_GUI_ASSETS_H
#define TEMP_GUI_ASSETS_H

#include <gtk/gtk.h>
#include "protocol.h"

#define GUI_OPPONENT_SLOTS 6
#define TURN_SECONDS       30

/* -- Per-seat countdown timer ---------------------------------------------- */
typedef struct {
    GtkWidget *bar;
    GtkWidget *label;
    guint      timer_id;
    gint       seconds_left;
    gint       turn_seconds;
    gboolean   is_my_timer;
} SeatTimer;

/* -- Card draw helper ------------------------------------------------------- */
typedef struct {
    Card card;
    int  face_up;
} CardDrawData;

/* -- All widget references in one place ------------------------------------ */
typedef struct {
    /* window / navigation */
    GtkWidget *window;
    GtkWidget *stack;

    /* login screen */
    GtkWidget *login_screen;
    GtkWidget *name_entry;
    GtkWidget *host_entry;
    GtkWidget *port_entry;
    GtkWidget *login_status;

    /* game screen */
    GtkWidget *game_screen;
    GtkWidget *pot_label;
    GtkWidget *stage_label;
    GtkWidget *log_label;
    GtkWidget *deck_count_label;
    GtkWidget *community_cards[5];
    GtkWidget *my_cards[2];
    GtkWidget *self_blind;
    GtkWidget *label_call_amnt;
    GtkWidget *label_your_stack;

    /* opponent seats */
    GtkWidget *opp_frame [GUI_OPPONENT_SLOTS];
    GtkWidget *opp_name  [GUI_OPPONENT_SLOTS];
    GtkWidget *opp_chips [GUI_OPPONENT_SLOTS];
    GtkWidget *opp_status[GUI_OPPONENT_SLOTS];
    GtkWidget *opp_cards[GUI_OPPONENT_SLOTS][2];
    GtkWidget *opp_blind [GUI_OPPONENT_SLOTS];
    SeatTimer  opp_timer [GUI_OPPONENT_SLOTS];

    /* local player timer */
    SeatTimer  my_timer;

    /* action buttons */
    GtkWidget *btn_fold;
    GtkWidget *btn_check;
    GtkWidget *btn_allin;
    GtkWidget *btn_call;
    GtkWidget *btn_raise;
    GtkWidget *raise_spin;

    /* shop */
    GtkWidget *btn_shop;
    GtkWidget *shop;
    GtkWidget *shop_text;
    GtkWidget *shop_cards[6];
    GtkWidget *confirm_button;
    GtkWidget *back_button;

    /* chat */
    GtkWidget *chat_log;
    GtkWidget *chat_entry;
    GtkWidget *btn_send_chat;

    /* ready / quit dialog buttons */
    GtkWidget *ready_btn;
    GtkWidget *quit_btn;

    /* leaderboard */
    GtkWidget *leaderboard;
    GtkWidget *lb_name [MAX_PLAYERS];
    GtkWidget *lb_place[MAX_PLAYERS];
    GtkWidget *lb_card [MAX_PLAYERS][2];

    /* seat selection overlay */
    GtkWidget *seat_select;
    GtkWidget *seat_sel_name[MAX_PLAYERS];
    GtkWidget *seat_sel_btn [MAX_PLAYERS];

    guint net_source; /* g_io_add_watch source id */
} AppWidgets;

/* -- Shared globals (defined in temp_gui.c) -------------------------------- */
extern AppWidgets W;
extern ClientState C;

/* -- Merged CSS ------------------------------------------------------------ */
static const char * __attribute__((unused)) CSS =
/* global */
"window { background-color: #0d1117; }"
"label  { color: #000000; font-family: 'Georgia', serif; }"

/* login */
"#login-card { background-color: #161b22; border-radius: 16px;"
"  border: 1px solid #30363d; padding: 40px; }"
"#login-title { font-size: 30px; font-weight: bold;"
"  color: #e6c87a; letter-spacing: 4px; }"
"#login-sub   { font-size: 13px; color: #8b949e; letter-spacing: 2px; }"
"#login-suits { font-size: 20px; color: #e6c87a; }"
"label.field-lbl { font-size: 11px; color: #8b949e; letter-spacing: 1px; }"
"entry { background-color: #0d1117; color: #e6edf3;"
"  border: 1px solid #30363d; border-radius: 8px;"
"  padding: 10px 14px; font-family: 'Georgia', serif; font-size: 14px; }"
"entry:focus { border-color: #e6c87a; }"
"#connect-btn { background-color: #e6c87a; color: #0d1117; border: none;"
"  border-radius: 8px; padding: 12px 0;"
"  font-family: 'Georgia', serif; font-size: 15px; font-weight: bold;"
"  letter-spacing: 2px; }"
"#connect-btn:hover  { background-color: #f0d898; }"
"#connect-btn:active { background-color: #c9a84c; }"
"#login-status { font-size: 12px; color: #f85149; }"
"#offline-btn { background-color: #e6c87a; color: #0d1117;"
"  border: 1px solid #30363d; border-radius: 8px; padding: 10px 0;"
"  font-family: 'Georgia', serif; font-size: 13px; letter-spacing: 1px; }"
"#offline-btn:hover { background-color: #f0d898; color: #0d1117; border-color: #8b949e; }"
"#divider-label { font-size: 11px; color: #30363d; letter-spacing: 3px; }"

/* game screen */
"#game-root { background-color: #0d1117; }"
"#felt { background-color: #1a5c35; border: 6px solid #5c3210; border-radius: 120px; }"
"#pot-label   { font-size: 15px; color: #ffd700; font-weight: bold; }"
"#stage-label { font-size: 12px; color: #a0c8a0; letter-spacing: 2px; }"
"#log-label   { font-size: 12px; color: #7ab870; }"
"#label_your_stack, #big_stack_label { color: #f2c94c; font-weight: bold; }"
"#label_call_amnt, #big_toCall_label { color: #5bdcff; font-weight: bold; }"

/* opponent frames */
".opp-frame { background-color: rgba(0,0,0,0.45);"
"  border: 1px solid #2d5a3d; border-radius: 10px; padding: 6px 10px; }"
".opp-frame.active-seat { border-color: #ffd700;"
"  background-color: rgba(255,215,0,0.08); }"
".opp-name   { font-size: 13px; font-weight: bold; color: #e0f0e0; }"
".opp-chips  { font-size: 11px; color: #a0c8a0; }"
".opp-status { font-size: 10px; color: #ffd700; }"

/* timers */
".seat-timer-bar   { min-height: 4px; }"
".seat-timer-label { font-size: 10px; color: #ffd700; }"
"#my-timer-bar     { min-height: 6px; }"
"#my-timer-label   { font-size: 12px; color: #ffd700; font-weight: bold; }"

/* action buttons */
".action-btn { border-radius: 8px; font-family: 'Georgia', serif;"
"  font-size: 13px; font-weight: bold; letter-spacing: 1px;"
"  padding: 10px 18px; border: 1px solid; }"
"#btn-fold  { background-color: #3a1a1a; border-color: #c0392b; color: #e87a7a; }"
"#btn-check { background-color: #1a2a3a; border-color: #2980b9; color: #7ab8e8; }"
"#btn-call  { background-color: #1a3a1a; border-color: #27ae60; color: #7ae890; }"
"#btn-raise { background-color: #3a3a1a; border-color: #f39c12; color: #f0c050; }"
"#btn-ready { background-color: #2b2438; border-color: #9b7ee8; color: #d6c8ff; }"
".action-btn:disabled { opacity: 0.3; }"

/* shop */
"#btn-shop { background-color: #2a1a3a; border: 1px solid #9b59b6;"
"  border-radius: 8px; color: #c39bd3; font-family: 'Georgia', serif;"
"  font-size: 13px; font-weight: bold; letter-spacing: 1px; padding: 10px 18px; }"
"#btn-shop:hover  { background-color: #3d2755; }"
"#btn-shop:active { background-color: #1e1228; }"
".shop-card-selected { background-color: #4a3060; border: 2px solid #c39bd3; }"
".shop-target-hint  { border-color: #c39bd3;"
"  background-color: rgba(155,89,182,0.15); }"

/* chat */
"#chat-box { background-color: #161b22; border-top: 1px solid #30363d; padding: 6px 12px; }"
"#chat-title { font-size: 11px; color: #e6c87a; letter-spacing: 2px; }"
"#chat-log { background-color: #0d1117; color: #c9d1d9;"
"  font-family: 'Georgia', serif; font-size: 12px;"
"  border: 1px solid #30363d; border-radius: 6px; padding: 4px; }"
"#chat-entry { background-color: #0d1117; color: #e6edf3;"
"  border: 1px solid #30363d; border-radius: 6px;"
"  font-family: 'Georgia', serif; font-size: 12px; padding: 6px 10px; }"
"#btn-send-chat { background-color: #e6c87a; color: #0d1117; border: none;"
"  border-radius: 6px; font-family: 'Georgia', serif;"
"  font-size: 12px; font-weight: bold; padding: 6px 14px; }"
"#btn-send-chat:hover { background-color: #f0d898; }";


#endif /* TEMP_GUI_ASSETS_H */

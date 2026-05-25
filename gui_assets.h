#ifndef GUI_ASSEST_H
#define GUI_ASSEST_H
#include <gtk/gtk.h>
#include <stdlib.h>

#define GUI_OPPONENT_SLOTS 5
/* -- Suits / Ranks --------------------------------------------------------- */
static const char *RANK_STR[13] = {
    "2", "3", "4", "5", "6", "7", "8", "9", "10", "J", "Q", "K", "A"
};

static const char *SUIT_STR[4] = {"♥", "♦", "♣", "♠"};
static const int SUIT_RED[4] = {1, 1, 0, 0};

/* -- Widget references ----------------------------------------------------- */
typedef struct {
    /* login screen */
    GtkWidget *login_screen;
    GtkWidget *game_screen;
    GtkWidget *window;
    GtkWidget *stack;
    GtkWidget *name_entry;
    GtkWidget *host_entry;
    GtkWidget *port_entry;
    GtkWidget *login_status;

    /* chat screen */
    GtkWidget *chat_log;
    GtkWidget *chat_input;

    /* game screen */
    GtkWidget *pot_label;
    GtkWidget *stage_label;
    GtkWidget *log_label;
    GtkWidget *community_cards[5];
    GtkWidget *my_cards[2];
    GtkWidget *deck_count_label;

    /* opponent seats */
    GtkWidget *opp_name[GUI_OPPONENT_SLOTS];
    GtkWidget *opp_chips[GUI_OPPONENT_SLOTS];
    GtkWidget *opp_status[GUI_OPPONENT_SLOTS];
    GtkWidget *opp_frame[GUI_OPPONENT_SLOTS];

    /* action buttons */
    GtkWidget *btn_fold;
    GtkWidget *btn_check;
    GtkWidget *btn_call;
    GtkWidget *btn_raise;
    GtkWidget *btn_ready;
    GtkWidget *raise_spin;

    guint net_timer;
} AppWidgets;

typedef struct {
    Card card;
    int face_up;
} CardDrawData;

static AppWidgets W;
static ClientState C;

/* --------------------------------------------------------------------------
   CSS
   -------------------------------------------------------------------------- */
static const char *CSS =
/* window / global */
"window { background-color: #0d1117; }"
"label  { color: #e6edf3; font-family: 'Georgia', serif; }"

/* login card */
"#login-card {"
"  background-color: #161b22;"
"  border-radius: 16px;"
"  border: 1px solid #30363d;"
"  padding: 40px;"
"}"
"#login-title {"
"  font-size: 30px; font-weight: bold;"
"  color: #e6c87a; letter-spacing: 4px;"
"}"
"#login-sub   { font-size: 13px; color: #8b949e; letter-spacing: 2px; }"
"#login-suits { font-size: 20px; color: #e6c87a; }"
"label.field-lbl { font-size: 11px; color: #8b949e; letter-spacing: 1px; }"
"entry {"
"  background-color: #0d1117; color: #e6edf3;"
"  border: 1px solid #30363d; border-radius: 8px;"
"  padding: 10px 14px; font-family: 'Georgia', serif; font-size: 14px;"
"}"
"entry:focus { border-color: #e6c87a; }"
"#connect-btn {"
"  background-color: #e6c87a; color: #0d1117; border: none;"
"  border-radius: 8px; padding: 12px 0;"
"  font-family: 'Georgia', serif; font-size: 15px; font-weight: bold;"
"  letter-spacing: 2px;"
"}"
"#connect-btn:hover  { background-color: #f0d898; }"
"#connect-btn:active { background-color: #c9a84c; }"
"#login-status { font-size: 12px; color: #f85149; }"
"#offline-btn {"
"  background-color: #e6c87a; color: #0d1117;"
"  border: 1px solid #30363d; border-radius: 8px; padding: 10px 0;"
"  font-family: 'Georgia', serif; font-size: 13px; letter-spacing: 1px;"
"}"
"#offline-btn:hover { background-color: #f0d898; color: #0d1117; border-color: #8b949e; }"
"#divider-label { font-size: 11px; color: #30363d; letter-spacing: 3px; }"

/* game screen */
"#game-root { background-color: #0d1117; }"
"#felt {"
"  background-color: #1a5c35;"
"  border: 6px solid #5c3210;"
"  border-radius: 120px;"
"}"
"#pot-label   { font-size: 15px; color: #ffd700; font-weight: bold; }"
"#stage-label { font-size: 12px; color: #a0c8a0; letter-spacing: 2px; }"
"#log-label   { font-size: 12px; color: #7ab870; }"

/* opponent frames */
".opp-frame {"
"  background-color: rgba(0,0,0,0.45);"
"  border: 1px solid #2d5a3d;"
"  border-radius: 10px;"
"  padding: 6px 10px;"
"}"
".opp-frame.active-seat {"
"  border-color: #ffd700;"
"  background-color: rgba(255,215,0,0.08);"
"}"
".opp-name   { font-size: 13px; font-weight: bold; color: #e0f0e0; }"
".opp-chips  { font-size: 11px; color: #a0c8a0; }"
".opp-status { font-size: 10px; color: #ffd700; }"

/* action buttons */
".action-btn {"
"  border-radius: 8px; font-family: 'Georgia', serif;"
"  font-size: 13px; font-weight: bold; letter-spacing: 1px;"
"  padding: 10px 18px; border: 1px solid;"
"}"
"#btn-fold  { background-color: #3a1a1a; border-color: #c0392b; color: #e87a7a; }"
"#btn-check { background-color: #1a2a3a; border-color: #2980b9; color: #7ab8e8; }"
"#btn-call  { background-color: #1a3a1a; border-color: #27ae60; color: #7ae890; }"
"#btn-raise { background-color: #3a3a1a; border-color: #f39c12; color: #f0c050; }"
"#btn-ready { background-color: #2b2438; border-color: #9b7ee8; color: #d6c8ff; }"
".action-btn:disabled { opacity: 0.6; }"
"button label { color: inherit; }"
"spinbutton { background-color: #1a1a2a; color: #e6edf3; border: 1px solid #30363d; border-radius: 8px; }"
"spinbutton button { background-color: #2a2a3a; color: #e6edf3; min-width: 16px; }";

#endif

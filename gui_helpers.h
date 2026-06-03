#ifndef TEMP_GUI_HELPERS_H
#define TEMP_GUI_HELPERS_H

#include "gui_assets.h"

/* -- Rank / suit ----------------------------------------------------------- */
extern const char *RANK_STR[13];
extern const char *SUIT_STR[4];
extern const int   SUIT_RED[4];

int         rank_index(uint8_t rank);
const char *suit_label(uint8_t suit);
int         suit_is_red(uint8_t suit);
int         card_is_known(Card card);

/* -- Card drawing ---------------------------------------------------------- */
GtkWidget *make_card_widget(int w, int h);
void       init_card_widget(GtkWidget *da);
void       set_card_face(GtkWidget *da, Card c, int face_up);
void       set_card_back(GtkWidget *da);

/* -- Blind markers --------------------------------------------------------- */
typedef enum { BLIND_NONE, BLIND_SB, BLIND_BB } BlindKind;
void       init_blind_widget(GtkWidget *da);
void       set_blind_marker(GtkWidget *da, BlindKind kind);

/* -- Chip win popup -------------------------------------------------------- */
void       start_chip_win_anim(const char *winner_name, uint32_t amount);

/* -- Timer ----------------------------------------------------------------- */
void start_seat_timer(SeatTimer *t, int seconds);
void stop_seat_timer(SeatTimer *t);

/* -- Public timer API ------------------------------------------------------ */
void start_player_timer(int seat, int seconds);
void stop_player_timer(int seat);
void start_my_timer(int seconds);
void stop_my_timer(void);

/* -- Network send helpers -------------------------------------------------- */
void send_gui_move(MoveType move, uint32_t amount);
void sendReadyToServer(void);
void sendNameToServer(const char *name);
void sendChatToServer(const char *text);

/* -- Leaderboard display -------------------------------------------------------*/
void show_leaderboard(void);

#endif /* TEMP_GUI_HELPERS_H */

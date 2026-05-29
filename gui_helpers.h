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

#endif /* TEMP_GUI_HELPERS_H */

#ifndef SHOP_H
#define SHOP_H

#include <gtk/gtk.h>

#define SHOP_ITEM_COUNT 6

/* -------------------------------------------------------------------------
   One item in the shop
   ------------------------------------------------------------------------- */
typedef struct {
    const char *name;
    int         price;
    const char *image_path;   /* relative path, e.g. "img/redraw.jpg" */
} ShopItem;

/* -------------------------------------------------------------------------
   All shop window state in one struct
   ------------------------------------------------------------------------- */
typedef struct {
    GtkWidget *window;
    GtkWidget *cards[SHOP_ITEM_COUNT];   /* the 6 card buttons            */
    GtkWidget *balance_label;            /* "💰 Balance: 1000 chips"       */
    GtkWidget *feedback_label;           /* purchase result message        */
    int        player_balance;           /* kept in sync with game state   */
} ShopWindow;

/* -------------------------------------------------------------------------
   Public API
   ------------------------------------------------------------------------- */

/* Call once at startup – returns a heap-allocated ShopWindow */
ShopWindow *build_shop_window(GtkWidget *parent_window, int initial_balance);

/* Call from btn_shop "clicked" callback */
void show_shop_window(ShopWindow *shop, int current_balance);

/* Call whenever chips change while shop may be open */
void shop_window_set_balance(ShopWindow *shop, int balance);

#endif /* SHOP_H */
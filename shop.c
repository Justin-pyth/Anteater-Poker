/* =========================================================================
   shop.c  –  Shop window for Anteater Poker
   =========================================================================
   Add to your Makefile alongside gui.c / gui_extensions.c.
   Call build_shop_window() once after gtk_builder_get_objects(),
   store the returned pointer in GuiExtensions (or AppWidgets),
   then call show_shop_window() from your btn_shop "clicked" callback.
   ========================================================================= */

#include "shop.h"
#include <stdio.h>

/* -------------------------------------------------------------------------
   Shop item definitions  –  edit prices / names freely
   ------------------------------------------------------------------------- */
static const ShopItem SHOP_ITEMS[SHOP_ITEM_COUNT] = {
    { "Redraw Hand",       150, "img/redraw.jpg"           },
    { "Reveal Community",  200, "img/reveal_community.jpg" },
    { "Reveal Opponent",   250, "img/reveal_opponent.jpg"  },
    { "Swap 1 Card",       100, "img/swap1_card.png"       },
    { "Swap 2 Cards",      175, "img/swap2_cards.png"      },
    { "Swap Opp Cards",    300, "img/swapopp_cards.png"    },
};

/* -------------------------------------------------------------------------
   CSS for the shop window  –  load with a GtkCssProvider
   ------------------------------------------------------------------------- */
static const char *SHOP_CSS =
/* overlay window */
"#shop-window {"
"  background-color: #0d1117;"
"}"
/* header */
"#shop-title {"
"  font-family: 'Georgia', serif;"
"  font-size: 26px;"
"  font-weight: bold;"
"  color: #e6c87a;"
"  letter-spacing: 6px;"
"}"
"#shop-subtitle {"
"  font-size: 11px;"
"  color: #8b949e;"
"  letter-spacing: 3px;"
"}"
/* card button */
".shop-card {"
"  background-color: #161b22;"
"  border: 1px solid #30363d;"
"  border-radius: 12px;"
"  padding: 0px;"
"}"
".shop-card:hover {"
"  background-color: #1f2937;"
"  border-color: #e6c87a;"
"}"
".shop-card:active {"
"  background-color: #111827;"
"}"
/* item name */
".shop-item-name {"
"  font-family: 'Georgia', serif;"
"  font-size: 12px;"
"  font-weight: bold;"
"  color: #e6edf3;"
"  letter-spacing: 1px;"
"}"
/* price badge */
".shop-item-price {"
"  font-family: 'Georgia', serif;"
"  font-size: 13px;"
"  font-weight: bold;"
"  color: #ffd700;"
"}"
/* balance bar */
"#balance-bar {"
"  background-color: #161b22;"
"  border-top: 1px solid #30363d;"
"  padding: 10px 20px;"
"}"
"#balance-label {"
"  font-family: 'Georgia', serif;"
"  font-size: 14px;"
"  color: #f2c94c;"
"  font-weight: bold;"
"}"
"#btn-shop-back {"
"  background-color: #2a1a3a;"
"  border: 1px solid #9b59b6;"
"  border-radius: 8px;"
"  color: #c39bd3;"
"  font-family: 'Georgia', serif;"
"  font-size: 13px;"
"  font-weight: bold;"
"  letter-spacing: 1px;"
"  padding: 8px 24px;"
"}"
"#btn-shop-back:hover  { background-color: #3d2755; }"
"#btn-shop-back:active { background-color: #1e1228; }"
/* feedback label */
"#shop-feedback {"
"  font-size: 11px;"
"  color: #7ae890;"
"  letter-spacing: 1px;"
"}"
;

/* -------------------------------------------------------------------------
   Internal: called when a shop card is clicked
   ------------------------------------------------------------------------- */
typedef struct {
    ShopWindow *shop;
    int         index;
} CardClickData;

static void on_card_clicked(GtkButton *btn, gpointer user_data)
{
    (void)btn;
    CardClickData *d    = (CardClickData *)user_data;
    ShopWindow    *shop = d->shop;
    int            idx  = d->index;

    const ShopItem *item = &SHOP_ITEMS[idx];

    /* Check if player can afford it */
    if (shop->player_balance < item->price) {
        gtk_label_set_text(GTK_LABEL(shop->feedback_label),
                           "✗ Not enough chips!");
        return;
    }

    /* Deduct and update balance */
    shop->player_balance -= item->price;
    shop_window_set_balance(shop, shop->player_balance);

    /* Show feedback */
    char msg[128];
    snprintf(msg, sizeof(msg), "✓ Purchased: %s", item->name);
    gtk_label_set_text(GTK_LABEL(shop->feedback_label), msg);

    /* TODO: call your game logic here, e.g.:
       apply_shop_item(&C, idx);
       send_shop_purchase(&C, idx);
    */
}

/* -------------------------------------------------------------------------
   Internal: build one card button
   ------------------------------------------------------------------------- */
static GtkWidget *build_card(ShopWindow *shop, int idx)
{
    const ShopItem *item = &SHOP_ITEMS[idx];

    GtkWidget *btn = gtk_button_new();
    gtk_style_context_add_class(gtk_widget_get_style_context(btn), "shop-card");
    gtk_widget_set_size_request(btn, 140, 190);

    /* Vertical box inside the button */
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_widget_set_margin_top   (vbox, 12);
    gtk_widget_set_margin_bottom(vbox, 12);
    gtk_widget_set_margin_start (vbox, 10);
    gtk_widget_set_margin_end   (vbox, 10);
    gtk_container_add(GTK_CONTAINER(btn), vbox);

    /* Image – scale to fixed size */
    GtkWidget *img_widget;
    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file_at_scale(
        item->image_path, 90, 90, TRUE, NULL);
    if (pixbuf) {
        img_widget = gtk_image_new_from_pixbuf(pixbuf);
        g_object_unref(pixbuf);
    } else {
        /* Fallback: show a placeholder label if image not found */
        img_widget = gtk_label_new("🎴");
    }
    gtk_widget_set_halign(img_widget, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(vbox), img_widget, FALSE, FALSE, 0);

    /* Item name */
    GtkWidget *name_lbl = gtk_label_new(item->name);
    gtk_style_context_add_class(gtk_widget_get_style_context(name_lbl),
                                "shop-item-name");
    gtk_label_set_justify(GTK_LABEL(name_lbl), GTK_JUSTIFY_CENTER);
    gtk_label_set_line_wrap(GTK_LABEL(name_lbl), TRUE);
    gtk_widget_set_halign(name_lbl, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(vbox), name_lbl, FALSE, FALSE, 0);

    /* Separator */
    gtk_box_pack_start(GTK_BOX(vbox),
                       gtk_separator_new(GTK_ORIENTATION_HORIZONTAL),
                       FALSE, FALSE, 2);

    /* Price */
    char price_str[32];
    snprintf(price_str, sizeof(price_str), "💰 %d", item->price);
    GtkWidget *price_lbl = gtk_label_new(price_str);
    gtk_style_context_add_class(gtk_widget_get_style_context(price_lbl),
                                "shop-item-price");
    gtk_widget_set_halign(price_lbl, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(vbox), price_lbl, FALSE, FALSE, 0);

    /* Connect click */
    CardClickData *cd = g_new0(CardClickData, 1);
    cd->shop  = shop;
    cd->index = idx;
    g_signal_connect(btn, "clicked", G_CALLBACK(on_card_clicked), cd);
    /* Free cd when button is destroyed */
    g_signal_connect_swapped(btn, "destroy", G_CALLBACK(g_free), cd);

    return btn;
}

/* -------------------------------------------------------------------------
   Internal: back button callback
   ------------------------------------------------------------------------- */
static void on_back_clicked(GtkButton *btn, gpointer user_data)
{
    (void)btn;
    ShopWindow *shop = (ShopWindow *)user_data;
    gtk_widget_hide(shop->window);
    /* Clear feedback when closing */
    gtk_label_set_text(GTK_LABEL(shop->feedback_label), "");
}

/* =========================================================================
   build_shop_window()
   Call once during GUI initialisation.  Pass the main GtkWindow so the
   shop can be set transient (stays on top, centred).
   ========================================================================= */
ShopWindow *build_shop_window(GtkWidget *parent_window, int initial_balance)
{
    ShopWindow *shop = g_new0(ShopWindow, 1);
    shop->player_balance = initial_balance;

    /* --- Apply CSS --- */
    GtkCssProvider *prov = gtk_css_provider_new();
    gtk_css_provider_load_from_data(prov, SHOP_CSS, -1, NULL);
    gtk_style_context_add_provider_for_screen(
        gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(prov),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(prov);

    /* --- Top-level window --- */
    shop->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_widget_set_name(shop->window, "shop-window");
    gtk_window_set_title(GTK_WINDOW(shop->window), "Shop");
    gtk_window_set_default_size(GTK_WINDOW(shop->window), 560, 460);
    gtk_window_set_resizable(GTK_WINDOW(shop->window), FALSE);
    if (parent_window) {
        gtk_window_set_transient_for(GTK_WINDOW(shop->window),
                                     GTK_WINDOW(parent_window));
        gtk_window_set_position(GTK_WINDOW(shop->window),
                                GTK_WIN_POS_CENTER_ON_PARENT);
    }
    /* Hide instead of destroy when X is clicked */
    g_signal_connect(shop->window, "delete-event",
                     G_CALLBACK(gtk_widget_hide_on_delete), NULL);

    /* --- Root vertical box --- */
    GtkWidget *root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(shop->window), root);

    /* --- Header --- */
    GtkWidget *header = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_widget_set_margin_top   (header, 20);
    gtk_widget_set_margin_bottom(header, 16);
    gtk_box_pack_start(GTK_BOX(root), header, FALSE, FALSE, 0);

    GtkWidget *title = gtk_label_new("SHOP");
    gtk_widget_set_name(title, "shop-title");
    gtk_widget_set_halign(title, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(header), title, FALSE, FALSE, 0);

    GtkWidget *sub = gtk_label_new("SPEND YOUR CHIPS WISELY");
    gtk_widget_set_name(sub, "shop-subtitle");
    gtk_widget_set_halign(sub, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(header), sub, FALSE, FALSE, 0);

    /* --- 2x3 Grid of cards --- */
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing   (GTK_GRID(grid), 12);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 12);
    gtk_widget_set_halign(grid, GTK_ALIGN_CENTER);
    gtk_widget_set_margin_start (grid, 20);
    gtk_widget_set_margin_end   (grid, 20);
    gtk_widget_set_margin_bottom(grid, 12);
    gtk_box_pack_start(GTK_BOX(root), grid, TRUE, TRUE, 0);

    for (int i = 0; i < SHOP_ITEM_COUNT; i++) {
        GtkWidget *card = build_card(shop, i);
        shop->cards[i]  = card;
        int col = i % 3;   /* 3 columns */
        int row = i / 3;   /* 2 rows    */
        gtk_grid_attach(GTK_GRID(grid), card, col, row, 1, 1);
    }

    /* --- Feedback label --- */
    shop->feedback_label = gtk_label_new("");
    gtk_widget_set_name(shop->feedback_label, "shop-feedback");
    gtk_widget_set_halign(shop->feedback_label, GTK_ALIGN_CENTER);
    gtk_widget_set_margin_bottom(shop->feedback_label, 4);
    gtk_box_pack_start(GTK_BOX(root), shop->feedback_label, FALSE, FALSE, 0);

    /* --- Bottom bar: balance + back button --- */
    GtkWidget *bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_name(bar, "balance-bar");
    gtk_box_pack_end(GTK_BOX(root), bar, FALSE, FALSE, 0);

    /* Balance label (left) */
    char bal_str[64];
    snprintf(bal_str, sizeof(bal_str), "💰 Balance: %d chips", initial_balance);
    shop->balance_label = gtk_label_new(bal_str);
    gtk_widget_set_name(shop->balance_label, "balance-label");
    gtk_widget_set_halign(shop->balance_label, GTK_ALIGN_START);
    gtk_widget_set_hexpand(shop->balance_label, TRUE);
    gtk_box_pack_start(GTK_BOX(bar), shop->balance_label, TRUE, TRUE, 0);

    /* Back button (right) */
    GtkWidget *back_btn = gtk_button_new_with_label("◀  Back");
    gtk_widget_set_name(back_btn, "btn-shop-back");
    g_signal_connect(back_btn, "clicked", G_CALLBACK(on_back_clicked), shop);
    gtk_box_pack_end(GTK_BOX(bar), back_btn, FALSE, FALSE, 0);

    gtk_widget_show_all(shop->window);
    gtk_widget_hide(shop->window); /* start hidden */

    return shop;
}

/* =========================================================================
   show_shop_window()
   Call from your btn_shop "clicked" callback.
   Pass the current chip count so the balance is always up-to-date.
   ========================================================================= */
void show_shop_window(ShopWindow *shop, int current_balance)
{
    shop_window_set_balance(shop, current_balance);
    gtk_label_set_text(GTK_LABEL(shop->feedback_label), "");
    gtk_widget_show_all(shop->window);
    gtk_window_present(GTK_WINDOW(shop->window));
}

/* =========================================================================
   shop_window_set_balance()
   Call whenever the player's chip count changes while the shop is open.
   ========================================================================= */
void shop_window_set_balance(ShopWindow *shop, int balance)
{
    shop->player_balance = balance;
    char buf[64];
    snprintf(buf, sizeof(buf), "💰 Balance: %d chips", balance);
    gtk_label_set_text(GTK_LABEL(shop->balance_label), buf);
}
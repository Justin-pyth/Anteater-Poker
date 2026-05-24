#include "extraGUI.h"

GtkWidget chat_log;
GtkWidget chat_input;

chat_log   = GtkTextView;
chat_input = GtkEntry;


#include <gtk/gtk.h>
 
/* Application state */
typedef struct {
    GtkWidget *label_time;
    GtkWidget *btn_start;
    GtkWidget *btn_reset;
    guint      timer_id;
    gboolean   running;
    gint       elapsed_seconds;
} TimerApp;

/* Format seconds into HH:MM:SS and update the label */
static void update_label(TimerApp *app) {
    gint h = app->elapsed_seconds / 3600;
    gint m = (app->elapsed_seconds % 3600) / 60;
    gint s = app->elapsed_seconds % 60;
 
    gchar *markup = g_markup_printf_escaped(
        "<span font='32' weight='bold' font_family='monospace'>"
        "%02d:%02d:%02d</span>", h, m, s);
    gtk_label_set_markup(GTK_LABEL(app->label_time), markup);
    g_free(markup);
}
 
/* Called every second while the timer is running */
static gboolean on_tick(gpointer user_data) {
    TimerApp *app = (TimerApp *)user_data;
    app->elapsed_seconds++;
    update_label(app);
    return G_SOURCE_CONTINUE;
}
 
/* Start / Pause button handler */
static void on_start_pause(GtkButton *btn, gpointer user_data) {
    TimerApp *app = (TimerApp *)user_data;
 
    if (!app->running) {
        app->timer_id = g_timeout_add_seconds(1, on_tick, app);
        app->running  = TRUE;
        gtk_button_set_label(btn, "Pause");
    } else {
        g_source_remove(app->timer_id);
        app->timer_id = 0;
        app->running  = FALSE;
        gtk_button_set_label(btn, "Start");
    }
}
 
/* Reset button handler */
static void on_reset(GtkButton *btn, gpointer user_data) {
    (void)btn;
    TimerApp *app = (TimerApp *)user_data;
 
    if (app->running) {
        g_source_remove(app->timer_id);
        app->timer_id = 0;
        app->running  = FALSE;
        gtk_button_set_label(GTK_BUTTON(app->btn_start), "Start");
    }
 
    app->elapsed_seconds = 0;
    update_label(app);
}
 
/* Build the UI */
static void activate(GtkApplication *gapp, gpointer user_data) {
    TimerApp *app = (TimerApp *)user_data;
 
    /* Window */
    GtkWidget *window = gtk_application_window_new(gapp);
    gtk_window_set_title(GTK_WINDOW(window), "GTK Timer");
    gtk_window_set_default_size(GTK_WINDOW(window), 300, 160);
    gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
 
    /* Root vertical box */
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
    gtk_widget_set_margin_top(vbox, 24);
    gtk_widget_set_margin_bottom(vbox, 24);
    gtk_widget_set_margin_start(vbox, 24);
    gtk_widget_set_margin_end(vbox, 24);
    gtk_window_set_child(GTK_WINDOW(window), vbox);
 
    /* Time display label (big monospace font via Pango markup) */
    app->label_time = gtk_label_new(NULL);
    update_label(app);   /* set initial "00:00:00" markup */
    gtk_box_append(GTK_BOX(vbox), app->label_time);
 
    /* Button row */
    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_widget_set_halign(hbox, GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(vbox), hbox);
 
    app->btn_start = gtk_button_new_with_label("Start");
    gtk_widget_set_size_request(app->btn_start, 100, 36);
    g_signal_connect(app->btn_start, "clicked", G_CALLBACK(on_start_pause), app);
    gtk_box_append(GTK_BOX(hbox), app->btn_start);
 
    app->btn_reset = gtk_button_new_with_label("Reset");
    gtk_widget_set_size_request(app->btn_reset, 100, 36);
    g_signal_connect(app->btn_reset, "clicked", G_CALLBACK(on_reset), app);
    gtk_box_append(GTK_BOX(hbox), app->btn_reset);
 
    gtk_window_present(GTK_WINDOW(window));
}
 
int main(int argc, char *argv[]) {
    TimerApp app = {0};
 
    GtkApplication *gapp = gtk_application_new(
        "com.example.gtktimer", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(gapp, "activate", G_CALLBACK(activate), &app);
 
    int status = g_application_run(G_APPLICATION(gapp), argc, argv);
    g_object_unref(gapp);
    return status;
}
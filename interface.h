#ifndef __INTERFACE_H__
#define __INTERFACE_H__

struct widget_struct {
    GtkEventController *window_event_controller;
    GtkWidget *drawing_area;
    GtkWidget *labels[10];
}; 
extern struct widget_struct widgets;

void activate_cb(GtkApplication* app, gpointer user_data);
#endif 

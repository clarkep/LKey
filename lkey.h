#ifndef __LKEY_H__
#define __LKEY_H__

#define NUM_KEYS 17
#define MAX_CHORD_LEN 8

void debug(char *format, ...);

void 
key_pressed_gcb(GtkEventControllerKey *controller,
                        guint keyval, guint keycode,  GdkModifierType state,
                          gpointer user_data);

void 
key_released_gcb(GtkEventControllerKey *controller,
                        guint keyval, guint keycode,  GdkModifierType state,
                          gpointer user_data);

void 
click_on_chord_cb(GtkGestureClick *click, gint n_press,
    gdouble x, gdouble y, gpointer user_data);

void
edit_chord_cb(GtkGestureClick *click, gint n_press,
            gdouble x, gdouble y, gpointer user_data);

void 
volume_changed_cb(GtkRange *range, gpointer user_data);

struct key_state
{
    uint8_t chord;
    uint8_t pressed;
    uint8_t midi_updated; 
    uint8_t chord_behind;
    int chord_ar[MAX_CHORD_LEN];
};
extern struct key_state key_state_buffer[NUM_KEYS];

#endif

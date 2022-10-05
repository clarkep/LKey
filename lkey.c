#include <signal.h>
#include <stdint.h>
#include <stdarg.h>
#include <unistd.h>
#include <gtk/gtk.h>
#include <jack/jack.h>
#include <jack/midiport.h>

#include "lkey.h"
#include "interface.h"

/* Note and chord terminator: can't be zero because 
 * that represents the unison interval. */
#define CHTERM 255
#define BASE_NOTE 60
#define NUM_CHORDS 10
#define DEFAULT_CHORDS 5

#define VELOCITY 127

#define DEBUG 1

/* GLOBAL VARS */

jack_client_t *client;
jack_port_t *output_port;

int chord_unity[2] = {0, CHTERM};
int chord_major_triad[4] = {0, 4, 7, CHTERM};
int chord_minor_triad[4] = {0, 3, 7, CHTERM}; 
int chord_dim_triad[4] = {0, 3, 6, CHTERM};
int chord_aug_triad[10] = {0, 4, 8, CHTERM};
int *chords_array[NUM_CHORDS] = {chord_unity, chord_major_triad,
                            chord_minor_triad, chord_dim_triad, chord_aug_triad, 
                            NULL, NULL, NULL, NULL, NULL};
int current_chord = 0;
int volume = VELOCITY;
int base_note = BASE_NOTE;
int editing = 0;
int editing_i = 0;
int num_chords = DEFAULT_CHORDS;

uint8_t note_keycodes[NUM_KEYS] = { 52, 39, 53, 40, 54, 55, 42, 56, 43, 57, 44, 58, 59, 46, 60, 47, 61}; 
uint8_t keypad_keycodes[10] = {90, 87, 88, 89, 83, 84, 85, 79, 80, 81};

void debug(char *format, ...) {
#if DEBUG
    va_list args;
    va_start(args, format );
    vfprintf(stderr, format, args );
    va_end(args);
#endif
}

struct key_state key_state_buffer[NUM_KEYS];

void init_key_state_buffer() 
{
    for (int i=0; i<NUM_KEYS; i++ ) {
        key_state_buffer[i].pressed = 0;
        key_state_buffer[i].midi_updated = 1; }
}

void copy_chord(int *dest, int *source);
void update_key_state_buffer_new_chord()
{
    for (int i=0; i<NUM_KEYS; i++) {
        if (!key_state_buffer[i].pressed) {
            copy_chord(key_state_buffer[i].chord_ar, chords_array[current_chord]);
        }
    }
}

/* We want dynamically allocated chords in our chords_array. */
void init_chords_array() 
{
    for (int i=0; i<DEFAULT_CHORDS; i++) {
        int *new_chord = malloc(sizeof(int)*(MAX_CHORD_LEN+1));
        copy_chord(new_chord, chords_array[i]); 
        chords_array[i] = new_chord;
    }
}

/* NOTE MANIPULATION */
void invert_down(int *chord) 
{
    int highest=0;
    int highest_i=255;
    int *cur_note = chord;
    int i = 0;
    while (*cur_note != CHTERM) {
        if (*cur_note > highest)  {
            highest = *cur_note; 
            highest_i = i;
        } 
        i++;
        cur_note++;
    }
    chord[highest_i] -= 12;
}

void invert_up(int *chord) 
{
    int lowest=255;
    int lowest_i=255;
    int *cur_note = chord;
    int i = 0;
    while (*cur_note != CHTERM) {
        if (*cur_note < lowest)  {
            lowest = *cur_note; 
            lowest_i = i;
        } 
        i++;
        cur_note++;
    }
    chord[lowest_i] += 12;
}

void copy_chord(int *dest, int *source) 
{
    while ((*dest++=*source++)!=CHTERM) {} 
}


/* USER INTERACTION CALLBACKS */

uint8_t get_pkey_by_keycode(guint keycode) 
{
    for (int i=0; i<NUM_KEYS; i++) {
        if (keycode == note_keycodes[i]) {
            return i;
        }
    }
    return 255;
}

uint8_t get_keypad_num_by_keycode(guint keycode)
{
    for (int i=0; i<10; i++) {
        if (keycode == keypad_keycodes[i]) {
            return i;
        }
    }
    return 255; 
}

static void 
handle_keypress_non_editing_mode(guint keyval, guint keycode, gpointer user_data)
{
    uint8_t pkey = get_pkey_by_keycode(keycode);
    //debug("keyval: %d, keycode: %d\n", keyval, keycode);
    if (pkey != 255 && key_state_buffer[pkey].pressed==0) {
        key_state_buffer[pkey].chord = current_chord; 
        key_state_buffer[pkey].pressed = 1;
        key_state_buffer[pkey].midi_updated = 0;
    } else {
        uint8_t keypad_num = get_keypad_num_by_keycode(keycode);
        int new_chord = 255;
        if (keypad_num != 255) {
            new_chord = keypad_num; 
        } else if (keyval >= 48 && keyval < 58) {
            new_chord = keyval - 48;
        } else {
            switch (keycode) { 
                case 116: //arrow keys
                new_chord = current_chord<4 ? 0 : current_chord-3;
                break;
                case 111:
                new_chord = current_chord==0 ? 1 : 
                    (current_chord<7 ? current_chord+3 : current_chord);
                break;
                case 113:
                new_chord = (current_chord%3!=1 && current_chord) ? current_chord-1 
                    : current_chord;
                break;
                case 114:
                new_chord = current_chord%3!=0 ? current_chord+1 : current_chord;
                break;
                case 34: // open bracket
                invert_down(chords_array[current_chord]); 
                break;
                case 35: // close bracket
                invert_up(chords_array[current_chord]);
                break;
                case 20: // minus
                base_note -= 12;
                break;
                case 21: // plus
                base_note += 12;
                break;
            }
        }
        if (chords_array[new_chord]) {
            gtk_widget_remove_css_class(widgets.labels[current_chord], "highlighted");
            gtk_widget_add_css_class(widgets.labels[new_chord], "highlighted");
            current_chord = new_chord;
            update_key_state_buffer_new_chord();
        } else {
        }
    }
    gtk_widget_queue_draw (GTK_WIDGET(user_data));
}

void
changed_cb (GObject* self, GParamSpec* pspec,
    gpointer user_data)
{
    //printf("editing: %d LEditing: %d\n", editing, gtk_editable_label_get_editing(GTK_EDITABLE_LABEL(self)));
    if (!gtk_editable_label_get_editing(GTK_EDITABLE_LABEL(self))) {
        //printf("All done!\n");
        gtk_event_controller_set_propagation_phase(widgets.window_event_controller,
        GTK_PHASE_CAPTURE);
        editing = 0;
        gtk_editable_set_editable(GTK_EDITABLE(self), 0);
        gtk_widget_remove_css_class(GTK_WIDGET(self), "inactive");
        gtk_widget_add_css_class(GTK_WIDGET(self), "highlighted");
        update_key_state_buffer_new_chord();
    }
}


static void 
leave_editing_mode()
{
    chords_array[current_chord][editing_i] = CHTERM;
    editing = 2;
    gtk_event_controller_set_propagation_phase(widgets.window_event_controller,
    GTK_PHASE_TARGET);
    GtkWidget *chord_label = widgets.labels[current_chord];
    g_signal_connect(chord_label, "notify::editing", G_CALLBACK(changed_cb), NULL);
    gtk_editable_set_editable(GTK_EDITABLE(chord_label), 1);
    gtk_editable_label_start_editing(GTK_EDITABLE_LABEL(chord_label)); 
}

static void 
handle_keypress_editing_mode(guint keyval, guint keycode, gpointer user_data)
{
    uint8_t pkey = get_pkey_by_keycode(keycode);
    if (pkey != 255 && editing==1 && key_state_buffer[pkey].pressed==0) {
        if (editing_i < MAX_CHORD_LEN) {
            chords_array[current_chord][editing_i] = pkey;      
            editing_i++;
            key_state_buffer[pkey].pressed = 1;
            // Don't send midi, because we just want to highlight the key.
            key_state_buffer[pkey].midi_updated = 1;
            gtk_widget_queue_draw (widgets.drawing_area);
            debug("%d\n", editing_i);
        }
    } else {
        if (keycode == 36 || keycode == 104 || keycode==24) {
            if (editing==1) {
                leave_editing_mode();
            }
        }
    }
}

void 
key_pressed_gcb(GtkEventControllerKey *controller,
                        guint keyval, guint keycode,  GdkModifierType state,
                          gpointer user_data)
{
    if (!editing) {
        handle_keypress_non_editing_mode(keyval, keycode, user_data);
    } else {
        handle_keypress_editing_mode(keyval, keycode, user_data);
    }
}

void 
key_released_gcb(GtkEventControllerKey *controller,
                        guint keyval, guint keycode,  GdkModifierType state,
                          gpointer user_data)
{
    uint8_t pkey = get_pkey_by_keycode(keycode);
    if (pkey != 255 && !editing) {
        /* don't update .chord: we want to release the same chord we pressed */
        key_state_buffer[pkey].pressed = 0;
        key_state_buffer[pkey].midi_updated = 0;
        key_state_buffer[pkey].chord_behind = 1;
    } else if (pkey != 255) {
        key_state_buffer[pkey].pressed = 0;
        key_state_buffer[pkey].midi_updated = 1;
    }
    gtk_widget_queue_draw (GTK_WIDGET(user_data));
}

void 
click_on_chord_cb(GtkGestureClick *click, gint n_press,
    gdouble x, gdouble y, gpointer user_data)
{
    GtkWidget **label_pointer = (GtkWidget **) user_data;
    ptrdiff_t new_chord = label_pointer-widgets.labels;
    if (chords_array[new_chord]) {
        gtk_widget_remove_css_class(widgets.labels[current_chord], "highlighted");
        gtk_widget_add_css_class(widgets.labels[new_chord], "highlighted");
        current_chord = new_chord;
        update_key_state_buffer_new_chord();
    }
}

void 
edit_chord_cb(GtkGestureClick *click, gint n_press,
            gdouble x, gdouble y, gpointer user_data)
{
    printf("Play chord then press Enter.\n");
    GtkWidget **label_pointer = (GtkWidget **) user_data;
    ptrdiff_t chord_n = label_pointer-widgets.labels;
    gtk_widget_remove_css_class(widgets.labels[current_chord], "highlighted");
    current_chord = chord_n;
    editing = 1;
    editing_i = 0;
    int *new_chord = malloc(sizeof(int)*(MAX_CHORD_LEN+1));
    free(chords_array[chord_n]);
    chords_array[chord_n] = new_chord;
    /*
    gtk_editable_set_editable(GTK_EDITABLE(*label_pointer), 1);
    gtk_editable_label_start_editing(GTK_EDITABLE_LABEL(*label_pointer)); 
    */
}

static void
close_window_cb (void)
{
  if (client)
    jack_client_close(client);
}

void 
volume_changed_cb(GtkRange *range, gpointer user_data)
{
    volume = gtk_range_get_value(range);
    printf("volume: %d\n", volume);
}

void my_getsize(GtkWidget *widget, GtkAllocation *allocation, void *data) {
    printf("width = %d, height = %d\n", allocation->width, allocation->height);
}

/* JACK CALLBACK */

static int process_cb(jack_nframes_t nframes, void *arg)
{
    void *port_buf = jack_port_get_buffer(output_port, nframes);
    unsigned char* buffer;
    jack_midi_clear_buffer(port_buf);
    int chords_written = 0;
    for (int pkey=0; pkey<NUM_KEYS; pkey++) {
        if (key_state_buffer[pkey].midi_updated==0) {
            //printf("playing pkey:%d\n", pkey);
            uint8_t type = key_state_buffer[pkey].pressed ? 0x90 : 0x80;
            int *nt = key_state_buffer[pkey].chord_ar;
            while (*nt != CHTERM) {
                buffer = jack_midi_event_reserve(port_buf, chords_written*3, 3);
                buffer[0] = type;
                buffer[1] = (unsigned char) base_note + pkey + *nt;
                buffer[2] = volume;
                nt++;
                //printf("type=%d, freq=%d, vel=%d\n", buffer[0], buffer[1], buffer[2]);
            }
            key_state_buffer[pkey].midi_updated = 1;
            // If this was a release, copy the new current chord to the key state buffer.
            if (key_state_buffer[pkey].chord_behind) {
                copy_chord(key_state_buffer[pkey].chord_ar, chords_array[current_chord]);
                key_state_buffer[pkey].chord_behind = 0;
            }
            chords_written++;
        }
    }
    return 0;
}

/* start_app: register the Gtk activate callback */
int start_app(int argc, char **argv)
{
    int status; GtkApplication *app;

    app = gtk_application_new("org.gtk.example", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK (activate_cb), NULL);
    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}

/* 
 * setup_jack:  Create the jack client, and register a "process callback" that can optionally
 * send midi events to the output port. Then activate the client.
 * */
int setup_jack()
{
    if((client = jack_client_open("lkey", JackNullOption, NULL))==0) {
        fprintf(stderr, "JACK server not running?\n");
    }
    jack_set_process_callback(client, process_cb, 0);
    output_port = jack_port_register(client, "out", JACK_DEFAULT_MIDI_TYPE, 
                                     JackPortIsOutput, 0);
    if (jack_activate(client)) {
        fprintf(stderr, "cannot activate client");
        return 1;
    }
    return 0;
}

int main (int argc, char **argv)
{
    int status;
    init_chords_array();
    init_key_state_buffer();
    setup_jack();
    update_key_state_buffer_new_chord();
    status = start_app(argc, argv);
    return status;
}


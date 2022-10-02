#include <gtk/gtk.h>

#include "interface.h"
#include "lkey.h"

/* UI SETUP CALLBACKS */

#define KB_WIDGET_WIDTH 1100 
#define KB_WIDGET_HEIGHT 300
#define TOP_ROW_OFFSET 50 
#define FIRST_KEY_OFFSET 50
#define VERT_GAP_BETWEEN_ROWS 100
#define HOR_GAP_BETWEEN_KEYS 100
#define WHITE_KEY_WIDTH 85
#define WHITE_KEY_HEIGHT 85
#define BLACK_KEY_WIDTH 85
#define BLACK_KEY_HEIGHT 85
#define HOR_OFFSET_BETWEEN_ROWS 50

#define SCALE 0.60

int key_colors[17] = {0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0};
struct widget_struct widgets;
static cairo_surface_t *surface = NULL;

static void
draw_shapes()
{
    cairo_t *cr;

    cr = cairo_create (surface);
    cairo_scale(cr, SCALE, SCALE);
    cairo_set_source_rgb(cr, 0, 0, 0);

    int x = FIRST_KEY_OFFSET - HOR_GAP_BETWEEN_KEYS;  // Start one key length behind.
    int y = TOP_ROW_OFFSET + VERT_GAP_BETWEEN_ROWS;
    for (int i=0; i<17; i++) {
        int on = key_state_buffer[i].pressed;
        if (key_colors[i]) { // black key
            cairo_rectangle (cr, x + HOR_OFFSET_BETWEEN_ROWS,
                             y - VERT_GAP_BETWEEN_ROWS, BLACK_KEY_WIDTH, BLACK_KEY_HEIGHT);
            if (on) {
                cairo_set_source_rgb(cr, 100, 0, 0);
            } else {
                cairo_set_source_rgb(cr, 0, 0, 0);
            }
            cairo_fill(cr);
        } else { // white key
            x += HOR_GAP_BETWEEN_KEYS;
            cairo_rectangle(cr, x, y, WHITE_KEY_WIDTH, WHITE_KEY_HEIGHT);
            if (on) {
                cairo_set_source_rgb(cr, 100, 0, 0);
                cairo_fill(cr);
            } else {
                cairo_set_source_rgb(cr, 255, 255, 255);
                cairo_fill_preserve(cr);
                cairo_set_source_rgb(cr, 0, 0, 0);
                cairo_stroke(cr);
            }
        }
    }
    cairo_fill (cr);
    cairo_destroy (cr);
}

static void
clear_surface (void)
{
    cairo_t *cr;

    cr = cairo_create (surface);

    /* Top row, i.e. black keys */
    cairo_set_source_rgb (cr, 1, 1, 1);
    cairo_paint (cr);

    cairo_destroy (cr);
}

static void
resize_cb (GtkWidget *widget,
        int        width,
        int        height,
        gpointer   data)
{
    if (surface)
    {
        cairo_surface_destroy (surface);
        surface = NULL;
    }

    if (gtk_native_get_surface (gtk_widget_get_native (widget)))
    {
        surface = gdk_surface_create_similar_surface (gtk_native_get_surface (gtk_widget_get_native (widget)),
                CAIRO_CONTENT_COLOR,
                gtk_widget_get_width (widget),
                gtk_widget_get_height (widget));

        /* Initialize the surface to white */
        clear_surface ();
    }
}

static void
draw_cb (GtkDrawingArea *drawing_area,
         cairo_t        *cr,
         int             width,
         int             height,
         gpointer        data)
{
    draw_shapes();
    cairo_set_source_surface (cr, surface, 0, 0);
    cairo_paint (cr);
}

char *chord_labels[10];
void init_chord_labels() 
{
    chord_labels[0] = "No Chord";
    chord_labels[1] = "Major";
    chord_labels[2] = "Minor";
    chord_labels[3] = "Diminished";
    chord_labels[4] = "Augmented";
}

static void label_add_callbacks(GtkWidget *label, int index)
{
    gtk_editable_set_editable(GTK_EDITABLE(label), 0);
    gtk_editable_set_alignment(GTK_EDITABLE(label), 0.5);
    GtkGesture *label_controller1 = gtk_gesture_click_new();
    gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(label_controller1), 1);
    gtk_widget_add_controller(label, GTK_EVENT_CONTROLLER(label_controller1));
    g_signal_connect_after(label_controller1, "pressed", G_CALLBACK(click_on_chord_cb), &widgets.labels[index]);
    GtkGesture *label_controller2 = gtk_gesture_click_new();
    gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(label_controller2), 3);
    gtk_widget_add_controller(label, GTK_EVENT_CONTROLLER(label_controller2));
    g_signal_connect_after(label_controller2, "pressed", G_CALLBACK(edit_chord_cb), &widgets.labels[index]);
}

static void 
add_chord_labels(GtkWidget *box)
{
    init_chord_labels();

    GtkWidget *vertical_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_widget_set_size_request(vertical_box, 300, 300);
    gtk_widget_set_margin_end(vertical_box, 5);
    gtk_widget_set_halign(vertical_box, GTK_ALIGN_END);

    GtkWidget *zero_label = gtk_editable_label_new(chord_labels[0]);
    gtk_widget_set_size_request(zero_label, 150, 60);
    gtk_widget_add_css_class(zero_label, "highlighted");
    gtk_box_prepend(GTK_BOX(vertical_box), zero_label);
    label_add_callbacks(zero_label, 0);
    //g_signal_connect(zero_label, "changed", G_CALLBACK(edit_chord_cb), NULL);
    widgets.labels[0] = zero_label;
    int index = 1;
    for (int row=0; row<3; row++) {
        GtkWidget *row_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5); 
        int active = 1;
        for (int col=0; col<3; col++) {
            char *chord_name;
            if (chord_labels[index]) { 
                chord_name = chord_labels[index]; 
            } else {
                chord_name = "Empty";
                active = 0;
            }
            GtkWidget *chord_label = gtk_editable_label_new(chord_name);
            if (!active) {
                gtk_widget_add_css_class(chord_label, "inactive");
            }
            gtk_widget_set_size_request(chord_label, 100, 80);
            gtk_box_append(GTK_BOX(row_box), chord_label);
            label_add_callbacks(chord_label, index);
            widgets.labels[index] = chord_label;
            index += 1;
        }
        gtk_box_prepend(GTK_BOX(vertical_box), row_box);
    }
    gtk_box_append(GTK_BOX(box), vertical_box);
}

/* SETUP UI AND REGISTER CALLBACKS */

void
activate_cb (GtkApplication* app,
        gpointer        user_data)
{
    GtkWidget *velocity_box;
    GtkWidget *label_title;
    GtkWidget *velocity_label;
    GtkWidget *kb_picture;
    GtkEventController *keypress;

/*
    GtkBuilder *builder = gtk_builder_new ();
    gtk_builder_add_from_file (builder, "interface.ui", NULL);
*/
    GtkBuilder *builder = gtk_builder_new_from_resource("/lkey/interface.ui");
    GObject *window = gtk_builder_get_object (builder, "window");
    gtk_window_set_application (GTK_WINDOW (window), app);

    GdkDisplay* display = gtk_widget_get_display(GTK_WIDGET(window));
    GtkCssProvider *style_provider = gtk_css_provider_new();
    GFile *css_file = g_file_new_for_path("styles.css");
    gtk_css_provider_load_from_file(style_provider, css_file);
    gtk_style_context_add_provider_for_display(display, GTK_STYLE_PROVIDER(style_provider),  
            GTK_STYLE_PROVIDER_PRIORITY_USER);

    GObject *hor_box = gtk_builder_get_object(builder, "upper_box");
    GObject *velocity_scale = gtk_builder_get_object(builder, "vel_scale");
    gtk_range_set_value(GTK_RANGE(velocity_scale), 127);
    g_signal_connect(velocity_scale, "value-changed", G_CALLBACK(volume_changed_cb), NULL);

    add_chord_labels(GTK_WIDGET(hor_box));
    GObject *vert_box = gtk_builder_get_object(builder, "content_box"); 

    GtkWidget *drawing_area = gtk_drawing_area_new();
    gtk_widget_set_size_request (drawing_area, SCALE*KB_WIDGET_WIDTH, SCALE*KB_WIDGET_HEIGHT);
    widgets.drawing_area = GTK_WIDGET(drawing_area);
    gtk_box_append(GTK_BOX(vert_box), drawing_area);

    gtk_drawing_area_set_draw_func (GTK_DRAWING_AREA(drawing_area), draw_cb, NULL, NULL);
    g_signal_connect_after (GTK_WIDGET(drawing_area), "resize", G_CALLBACK (resize_cb), NULL);

    keypress = gtk_event_controller_key_new();
    // Capture events before they filter down and get consumed by other widgets. This
    // is necessary to get arrow keys, which I think are consumed by containers.
    gtk_event_controller_set_propagation_phase(keypress, GTK_PHASE_CAPTURE); 
    gtk_widget_add_controller(GTK_WIDGET(window), GTK_EVENT_CONTROLLER(keypress));
    g_signal_connect(keypress, "key-pressed", G_CALLBACK(key_pressed_gcb),GTK_WIDGET(drawing_area));
    g_signal_connect(keypress, "key-released", G_CALLBACK(key_released_gcb),GTK_WIDGET(drawing_area));
    widgets.window_event_controller = keypress;

    gtk_widget_show(GTK_WIDGET(window));
}

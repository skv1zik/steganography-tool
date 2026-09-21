#include <gtk/gtk.h>
#include "steganography.h"
#include <string>
struct AppData {
    GtkWidget* window;
    GtkWidget* image_widget;
    GtkWidget* message_view;
    GtkWidget* status_label;
    GdkPixbuf* pixbuf;
};
static GdkPixbuf* scale_for_display(GdkPixbuf* src) {
    int width = gdk_pixbuf_get_width(src);
    int height = gdk_pixbuf_get_height(src);
    int maxDim = 400;
    if (width <= maxDim && height <= maxDim) return static_cast<GdkPixbuf*>(g_object_ref(src));
    double scale = static_cast<double>(maxDim) / static_cast<double>(width > height ? width : height);
    int newWidth = static_cast<int>(width * scale);
    int newHeight = static_cast<int>(height * scale);
    if (newWidth < 1) newWidth = 1;
    if (newHeight < 1) newHeight = 1;
    return gdk_pixbuf_scale_simple(src, newWidth, newHeight, GDK_INTERP_BILINEAR);
}
static void update_display(AppData* app) {
    if (app->pixbuf == nullptr) return;
    GdkPixbuf* scaled = scale_for_display(app->pixbuf);
    gtk_image_set_from_pixbuf(GTK_IMAGE(app->image_widget), scaled);
    g_object_unref(scaled);
}
static void on_load_clicked(GtkButton* button, gpointer user_data) {
    AppData* app = static_cast<AppData*>(user_data);
    GtkWidget* dialog = gtk_file_chooser_dialog_new("Load Image", GTK_WINDOW(app->window), GTK_FILE_CHOOSER_ACTION_OPEN, "_Cancel", GTK_RESPONSE_CANCEL, "_Open", GTK_RESPONSE_ACCEPT, NULL);
    GtkFileFilter* filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "Images");
    gtk_file_filter_add_pattern(filter, "*.png");
    gtk_file_filter_add_pattern(filter, "*.bmp");
    gtk_file_filter_add_pattern(filter, "*.PNG");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char* filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        GError* error = nullptr;
        GdkPixbuf* loaded = gdk_pixbuf_new_from_file(filename, &error);
        if (loaded != nullptr) {
            if (app->pixbuf != nullptr) g_object_unref(app->pixbuf);
            app->pixbuf = loaded;
            update_display(app);
            long capacity = getMaxMessageCapacity(app->pixbuf);
            std::string status = "Loaded image. Capacity: " + std::to_string(capacity) + " bytes";
            gtk_label_set_text(GTK_LABEL(app->status_label), status.c_str());
        } else {
            gtk_label_set_text(GTK_LABEL(app->status_label), "Failed to load image");
            if (error != nullptr) g_error_free(error);
        }
        g_free(filename);
    }
    gtk_widget_destroy(dialog);
}
static void on_encode_clicked(GtkButton* button, gpointer user_data) {
    AppData* app = static_cast<AppData*>(user_data);
    if (app->pixbuf == nullptr) {
        gtk_label_set_text(GTK_LABEL(app->status_label), "No image loaded");
        return;
    }
    GtkTextBuffer* buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(app->message_view));
    GtkTextIter start, end;
    gtk_text_buffer_get_start_iter(buffer, &start);
    gtk_text_buffer_get_end_iter(buffer, &end);
    char* text = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
    std::string message(text);
    g_free(text);
    bool success = encodeMessage(app->pixbuf, message);
    if (!success) {
        gtk_label_set_text(GTK_LABEL(app->status_label), "Message too large for this image");
        return;
    }
    GtkWidget* dialog = gtk_file_chooser_dialog_new("Save Image", GTK_WINDOW(app->window), GTK_FILE_CHOOSER_ACTION_SAVE, "_Cancel", GTK_RESPONSE_CANCEL, "_Save", GTK_RESPONSE_ACCEPT, NULL);
    gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), "stego_output.png");
    gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE);
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char* filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        std::string path(filename);
        if (path.size() < 4 || path.substr(path.size() - 4) != ".png") path += ".png";
        GError* error = nullptr;
        gboolean saved = gdk_pixbuf_save(app->pixbuf, path.c_str(), "png", &error, NULL);
        if (saved) {
            gtk_label_set_text(GTK_LABEL(app->status_label), "Message encoded and saved successfully");
        } else {
            gtk_label_set_text(GTK_LABEL(app->status_label), "Failed to save image");
            if (error != nullptr) g_error_free(error);
        }
        g_free(filename);
    }
    gtk_widget_destroy(dialog);
    update_display(app);
}
static void on_decode_clicked(GtkButton* button, gpointer user_data) {
    AppData* app = static_cast<AppData*>(user_data);
    if (app->pixbuf == nullptr) {
        gtk_label_set_text(GTK_LABEL(app->status_label), "No image loaded");
        return;
    }
    std::string message = decodeMessage(app->pixbuf);
    GtkTextBuffer* buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(app->message_view));
    if (message.empty()) {
        gtk_label_set_text(GTK_LABEL(app->status_label), "No hidden message found");
    } else {
        gtk_text_buffer_set_text(buffer, message.c_str(), -1);
        gtk_label_set_text(GTK_LABEL(app->status_label), "Message decoded successfully");
    }
}
static void activate(GtkApplication* gtkApp, gpointer user_data) {
    AppData* app = static_cast<AppData*>(user_data);
    app->window = gtk_application_window_new(gtkApp);
    gtk_window_set_title(GTK_WINDOW(app->window), "Steganography Tool");
    gtk_window_set_default_size(GTK_WINDOW(app->window), 600, 700);
    GtkWidget* mainBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_container_set_border_width(GTK_CONTAINER(mainBox), 10);
    gtk_container_add(GTK_CONTAINER(app->window), mainBox);
    GtkWidget* buttonBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    GtkWidget* loadButton = gtk_button_new_with_label("Load Image");
    GtkWidget* encodeButton = gtk_button_new_with_label("Encode & Save");
    GtkWidget* decodeButton = gtk_button_new_with_label("Decode");
    gtk_box_pack_start(GTK_BOX(buttonBox), loadButton, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(buttonBox), encodeButton, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(buttonBox), decodeButton, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(mainBox), buttonBox, FALSE, FALSE, 0);
    GtkWidget* imageScroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_size_request(imageScroll, 400, 400);
    app->image_widget = gtk_image_new();
    gtk_container_add(GTK_CONTAINER(imageScroll), app->image_widget);
    gtk_box_pack_start(GTK_BOX(mainBox), imageScroll, TRUE, TRUE, 0);
    GtkWidget* messageLabel = gtk_label_new("Message:");
    gtk_widget_set_halign(messageLabel, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(mainBox), messageLabel, FALSE, FALSE, 0);
    GtkWidget* messageScroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_size_request(messageScroll, -1, 120);
    app->message_view = gtk_text_view_new();
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(app->message_view), GTK_WRAP_WORD);
    gtk_container_add(GTK_CONTAINER(messageScroll), app->message_view);
    gtk_box_pack_start(GTK_BOX(mainBox), messageScroll, FALSE, FALSE, 0);
    app->status_label = gtk_label_new("No image loaded");
    gtk_widget_set_halign(app->status_label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(mainBox), app->status_label, FALSE, FALSE, 0);
    g_signal_connect(loadButton, "clicked", G_CALLBACK(on_load_clicked), app);
    g_signal_connect(encodeButton, "clicked", G_CALLBACK(on_encode_clicked), app);
    g_signal_connect(decodeButton, "clicked", G_CALLBACK(on_decode_clicked), app);
    gtk_widget_show_all(app->window);
}
int main(int argc, char** argv) {
    AppData app;
    app.pixbuf = nullptr;
    GtkApplication* gtkApp = gtk_application_new("com.example.steganography", G_APPLICATION_FLAGS_NONE);
    g_signal_connect(gtkApp, "activate", G_CALLBACK(activate), &app);
    int status = g_application_run(G_APPLICATION(gtkApp), argc, argv);
    if (app.pixbuf != nullptr) g_object_unref(app.pixbuf);
    g_object_unref(gtkApp);
    return status;
}

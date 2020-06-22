#include <string>
#include <gtk/gtk.h>

int ShowWarning(std::string message, GtkMessageType type_gtk = GTK_MESSAGE_WARNING) {
    // We don't have a pointer to the parent window, so nullptr should suffice
    GtkWidget *dialog = gtk_message_dialog_new(
        nullptr, GTK_DIALOG_DESTROY_WITH_PARENT,
        type_gtk, GTK_BUTTONS_CLOSE, message.c_str());

    int signal = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    return signal;  // return the received signal
}
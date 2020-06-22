#include "sep-helpers.hh"

int Show(std::string message, GtkMessageType type_gtk) {
    // We don't have a pointer to the parent window, so nullptr should suffice
    GtkWidget *dialog = gtk_message_dialog_new(
        nullptr, GTK_DIALOG_DESTROY_WITH_PARENT,
        type_gtk, GTK_BUTTONS_CLOSE, message.c_str());

    int signal = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    return signal;  // return the received signal
}

int ShowWarning(std::string message) {
    return Show(message, GTK_MESSAGE_WARNING);
}

/**
 * Add two pipette color map values of the same key.
 */
PipetteLayerOperations::PipetteColor sumPipetteColors(const PipetteLayerOperations::PipetteColor& lhs, const PipetteLayerOperations::PipetteColor& rhs)
{
  PipetteLayerOperations::PipetteColor result = lhs;
  for(auto const& elem : rhs)
  {
    result[elem.first] += elem.second;
  }
  return result;
}

/**
 * Divides each element inside elements by by a constant divisor.
 */
PipetteLayerOperations::PipetteColor dividePipetteColors(PipetteLayerOperations::PipetteColor elements, const int divisor)
{
  for(auto& elem : elements)
  {
    elem.second /= divisor;
  }
  return elements;
}

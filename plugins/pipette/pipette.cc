#include "pipette.hh"

#include <gdk/gdk.h>

////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////

Pipette::Pipette(){
}

Pipette::~Pipette(){
}

Pipette::Ptr Pipette::create(){
	return Ptr(new Pipette());
}

std::string Pipette::getPluginName(){
	return "Pipette";
}

std::string Pipette::getPluginVersion(){
	return "0.0";
}

void Pipette::registerCapabilities(ScroomPluginInterface::Ptr host){
	host->registerViewObserver("Pipette", shared_from_this<Pipette>());
	host->registerPresentationObserver("Pipette", shared_from_this<Pipette>());
}

static void on_toggled(GtkToggleButton* button, gpointer data){
	ViewInterface* view = static_cast<ViewInterface*>(data);
	view->unsetPanning();
}

Scroom::Bookkeeping::Token Pipette::viewAdded(ViewInterface::Ptr view){
	printf("View added\n");

	gdk_threads_enter();


	std::stringstream s;
	s << "_p";

	GtkToolItem* button = gtk_tool_item_new();
	GtkWidget* toggleButton = gtk_toggle_button_new_with_mnemonic(s.str().c_str());
	gtk_widget_set_visible(toggleButton, true);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(toggleButton), true);

	//Copy constructor to prevent view from being freed before it is used in the button handler
	ViewInterface::Ptr callback_ptr(view);
	gtk_container_add(GTK_CONTAINER(button), toggleButton);
	g_signal_connect(static_cast<gpointer>(toggleButton), "toggled", G_CALLBACK(on_toggled), callback_ptr.get());


	view->addToToolbar(button);



	gdk_threads_leave();

	return Scroom::Bookkeeping::Token();
	//return dunno what a token is or is supposed to do
}

void Pipette::presentationAdded(PresentationInterface::Ptr p){
	printf("Added cookie\n");


}

void Pipette::presentationDeleted(){
	printf("Deleted cookie\n");
}


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
}

Scroom::Bookkeeping::Token Pipette::viewAdded(ViewInterface::Ptr v){
	printf("View added 1-2-3-4-5-6-7-8-9");
}


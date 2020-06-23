#pragma once

#include <gtk/gtk.h>
#include <scroom/layeroperations.hh>
#include <string>

int Show(std::string message, GtkMessageType type_gtk);
int ShowWarning(std::string message);
PipetteLayerOperations::PipetteColor
sumPipetteColors(const PipetteLayerOperations::PipetteColor &lhs,
                 const PipetteLayerOperations::PipetteColor &rhs);
PipetteLayerOperations::PipetteColor
dividePipetteColors(PipetteLayerOperations::PipetteColor elements,
                    const int divisor);

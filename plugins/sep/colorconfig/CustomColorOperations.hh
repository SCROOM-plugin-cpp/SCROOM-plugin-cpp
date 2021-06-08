//
// Created by jelle on 02/06/2021.
//

#ifndef SCROOMCPPPLUGINS_CUSTOMCOLOROPERATIONS_HH
#define SCROOMCPPPLUGINS_CUSTOMCOLOROPERATIONS_HH

#include <boost/shared_ptr.hpp>
#include <scroom/pipettelayeroperations.hh>
#include <scroom/layeroperations.hh>
#include "CustomColor.hh"


class PipetteCommonOperationsCustomColor
        : public PipetteLayerOperations
                , public CommonOperations
{
protected:
    int bps;

public:
    using Ptr = boost::shared_ptr<PipetteCommonOperationsCustomColor>;

public:
    PipetteCommonOperationsCustomColor(int bps_)
            : bps(bps_){};

    PipetteLayerOperations::PipetteColor sumPixelValues(Scroom::Utils::Rectangle<int> area, const ConstTile::Ptr tile) override;
};


class OperationsCustomColors : public PipetteCommonOperationsCustomColor{
public:
    static Ptr create();
    OperationsCustomColors();

    int                  getBpp() override;
    Scroom::Utils::Stuff cache(const ConstTile::Ptr tile) override;
    void                 reduce(Tile::Ptr target, const ConstTile::Ptr source, int x, int y) override;
};



#endif //SCROOMCPPPLUGINS_CUSTOMCOLOROPERATIONS_HH

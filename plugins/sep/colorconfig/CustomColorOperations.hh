//
// Created by jelle on 02/06/2021.
//

#pragma once

#include "CustomColor.hh"
#include <boost/shared_ptr.hpp>
#include <scroom/layeroperations.hh>
#include <scroom/pipettelayeroperations.hh>

class PipetteCommonOperationsCustomColor : public PipetteLayerOperations,
                                           public CommonOperations {
protected:
  uint16_t bps;
  uint16_t spp;
  std::vector<CustomColor::Ptr> colors;

public:
  using Ptr = boost::shared_ptr<PipetteCommonOperationsCustomColor>;

public:
  PipetteCommonOperationsCustomColor(int bps_, int spp_ ) : bps(bps_) { spp = spp_; };
  void setSpp(int samplesPerPixel);
  void setColors(std::vector<CustomColor::Ptr> colors_);
  PipetteLayerOperations::PipetteColor
  sumPixelValues(Scroom::Utils::Rectangle<int> area,
                 const ConstTile::Ptr tile) override;
};

class OperationsCustomColors : public PipetteCommonOperationsCustomColor {
public:
  static Ptr create(int spp);
  OperationsCustomColors(int spp_);

  int getBpp() override;
  Scroom::Utils::Stuff cache(const ConstTile::Ptr tile) override;
  void reduce(Tile::Ptr target, const ConstTile::Ptr source, int x,
              int y) override;
};



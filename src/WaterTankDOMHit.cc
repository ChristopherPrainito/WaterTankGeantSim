/// \file WaterTankDOMHit.cc
/// \brief Implementation of the WaterTankDOMHit class

#include "WaterTankDOMHit.hh"

#include "G4SystemOfUnits.hh"

G4ThreadLocal G4Allocator<WaterTankDOMHit>* WaterTankDOMHitAllocator = nullptr;

WaterTankDOMHit::WaterTankDOMHit()
: G4VHit(),
  fTime(0.),
  fPosition(),
  fDirection(),
  fPhotonEnergy(0.),
  fWavelength(0.),
  fTrackID(-1),
  fParentID(-1)
{}

WaterTankDOMHit::WaterTankDOMHit(const WaterTankDOMHit& rhs)
: G4VHit()
{
  // Explicitly copy every member so that hits can be stored in STL containers
  // and safely cloned across threads if required.
  fTime         = rhs.fTime;
  fPosition     = rhs.fPosition;
  fDirection    = rhs.fDirection;
  fPhotonEnergy = rhs.fPhotonEnergy;
  fWavelength   = rhs.fWavelength;
  fTrackID      = rhs.fTrackID;
  fParentID     = rhs.fParentID;
}

WaterTankDOMHit& WaterTankDOMHit::operator=(const WaterTankDOMHit& rhs)
{
  if (this != &rhs) {
    fTime         = rhs.fTime;
    fPosition     = rhs.fPosition;
    fDirection    = rhs.fDirection;
    fPhotonEnergy = rhs.fPhotonEnergy;
    fWavelength   = rhs.fWavelength;
    fTrackID      = rhs.fTrackID;
    fParentID     = rhs.fParentID;
  }
  return *this;
}

WaterTankDOMHit::~WaterTankDOMHit() = default;

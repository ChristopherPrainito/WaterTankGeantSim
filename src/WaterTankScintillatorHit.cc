/// \file WaterTankScintillatorHit.cc
/// \brief Implementation of the WaterTankScintillatorHit class

#include "WaterTankScintillatorHit.hh"

#include "G4SystemOfUnits.hh"

G4ThreadLocal G4Allocator<WaterTankScintillatorHit>* WaterTankScintillatorHitAllocator = nullptr;

WaterTankScintillatorHit::WaterTankScintillatorHit()
: G4VHit(),
  fTime(0.),
  fPosition(),
  fEdep(0.),
  fLayer(-1),
  fBarIndex(-1),
  fTrackID(-1),
  fPDGCode(0)
{}

WaterTankScintillatorHit::WaterTankScintillatorHit(const WaterTankScintillatorHit& rhs)
: G4VHit()
{
  fTime     = rhs.fTime;
  fPosition = rhs.fPosition;
  fEdep     = rhs.fEdep;
  fLayer    = rhs.fLayer;
  fBarIndex = rhs.fBarIndex;
  fTrackID  = rhs.fTrackID;
  fPDGCode  = rhs.fPDGCode;
}

WaterTankScintillatorHit& WaterTankScintillatorHit::operator=(const WaterTankScintillatorHit& rhs)
{
  if (this != &rhs) {
    fTime     = rhs.fTime;
    fPosition = rhs.fPosition;
    fEdep     = rhs.fEdep;
    fLayer    = rhs.fLayer;
    fBarIndex = rhs.fBarIndex;
    fTrackID  = rhs.fTrackID;
    fPDGCode  = rhs.fPDGCode;
  }
  return *this;
}

WaterTankScintillatorHit::~WaterTankScintillatorHit() = default;

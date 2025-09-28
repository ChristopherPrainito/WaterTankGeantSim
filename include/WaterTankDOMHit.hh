/// \file WaterTankDOMHit.hh
/// \brief Definition of the WaterTankDOMHit class

#ifndef WaterTankDOMHit_h
#define WaterTankDOMHit_h 1

#include "G4VHit.hh"
#include "G4THitsCollection.hh"
#include "G4Allocator.hh"
#include "G4ThreeVector.hh"
#include "globals.hh"

class WaterTankDOMHit : public G4VHit
{
  public:
    WaterTankDOMHit();
    WaterTankDOMHit(const WaterTankDOMHit& rhs);
    WaterTankDOMHit& operator=(const WaterTankDOMHit& rhs);
    virtual ~WaterTankDOMHit();

    inline G4bool operator==(const WaterTankDOMHit& rhs) const
    {
      return (this == &rhs);
    }

    inline void* operator new(size_t);
    inline void operator delete(void* hit);

    void SetTime(G4double time) { fTime = time; }
    void SetPosition(const G4ThreeVector& pos) { fPosition = pos; }
    void SetDirection(const G4ThreeVector& dir) { fDirection = dir; }
    void SetPhotonEnergy(G4double energy) { fPhotonEnergy = energy; }
    void SetWavelength(G4double wavelength) { fWavelength = wavelength; }
    void SetTrackID(G4int id) { fTrackID = id; }
    void SetParentID(G4int id) { fParentID = id; }

    G4double        GetTime() const { return fTime; }
    const G4ThreeVector& GetPosition() const { return fPosition; }
    const G4ThreeVector& GetDirection() const { return fDirection; }
    G4double        GetPhotonEnergy() const { return fPhotonEnergy; }
    G4double        GetWavelength() const { return fWavelength; }
    G4int           GetTrackID() const { return fTrackID; }
    G4int           GetParentID() const { return fParentID; }

  private:
    G4double      fTime;
    G4ThreeVector fPosition;
    G4ThreeVector fDirection;
    G4double      fPhotonEnergy;
    G4double      fWavelength;
    G4int         fTrackID;
    G4int         fParentID;
};

typedef G4THitsCollection<WaterTankDOMHit> WaterTankDOMHitsCollection;

extern G4ThreadLocal G4Allocator<WaterTankDOMHit>* WaterTankDOMHitAllocator;

inline void* WaterTankDOMHit::operator new(size_t)
{
  if (!WaterTankDOMHitAllocator) {
    WaterTankDOMHitAllocator = new G4Allocator<WaterTankDOMHit>;
  }
  return (void*)WaterTankDOMHitAllocator->MallocSingle();
}

inline void WaterTankDOMHit::operator delete(void* hit)
{
  WaterTankDOMHitAllocator->FreeSingle(static_cast<WaterTankDOMHit*>(hit));
}

#endif

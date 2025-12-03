/// \file WaterTankScintillatorHit.hh
/// \brief Definition of the WaterTankScintillatorHit class

#ifndef WaterTankScintillatorHit_h
#define WaterTankScintillatorHit_h 1

#include "G4VHit.hh"
#include "G4THitsCollection.hh"
#include "G4Allocator.hh"
#include "G4ThreeVector.hh"
#include "globals.hh"

/// Lightweight record of a charged particle detection in a scintillator bar.
///
/// The scintillator sensitive detector creates one hit per charged particle
/// step that deposits energy above threshold. Each hit stores the layer/bar
/// identity, arrival time, position, energy deposit, and particle info.

class WaterTankScintillatorHit : public G4VHit
{
  public:
    WaterTankScintillatorHit();
    WaterTankScintillatorHit(const WaterTankScintillatorHit& rhs);
    WaterTankScintillatorHit& operator=(const WaterTankScintillatorHit& rhs);
    virtual ~WaterTankScintillatorHit();

    inline G4bool operator==(const WaterTankScintillatorHit& rhs) const
    {
      return (this == &rhs);
    }

    inline void* operator new(size_t);
    inline void operator delete(void* hit);

    // Setters
    void SetTime(G4double time) { fTime = time; }
    void SetPosition(const G4ThreeVector& pos) { fPosition = pos; }
    void SetEdep(G4double edep) { fEdep = edep; }
    void SetLayer(G4int layer) { fLayer = layer; }
    void SetBarIndex(G4int barIndex) { fBarIndex = barIndex; }
    void SetTrackID(G4int id) { fTrackID = id; }
    void SetPDGCode(G4int pdg) { fPDGCode = pdg; }

    // Getters
    G4double GetTime() const { return fTime; }
    const G4ThreeVector& GetPosition() const { return fPosition; }
    G4double GetEdep() const { return fEdep; }
    G4int GetLayer() const { return fLayer; }
    G4int GetBarIndex() const { return fBarIndex; }
    G4int GetTrackID() const { return fTrackID; }
    G4int GetPDGCode() const { return fPDGCode; }

  private:
    /// Global time of the hit (earliest step in bar for this track).
    G4double      fTime;
    /// Position of the energy deposit in world coordinates.
    G4ThreeVector fPosition;
    /// Energy deposited in this step/bar.
    G4double      fEdep;
    /// Scintillator layer index (0 = bottom layer, 1 = top layer).
    G4int         fLayer;
    /// Bar index within the layer.
    G4int         fBarIndex;
    /// Track ID of the particle.
    G4int         fTrackID;
    /// PDG code of the particle.
    G4int         fPDGCode;
};

typedef G4THitsCollection<WaterTankScintillatorHit> WaterTankScintillatorHitsCollection;

extern G4ThreadLocal G4Allocator<WaterTankScintillatorHit>* WaterTankScintillatorHitAllocator;

inline void* WaterTankScintillatorHit::operator new(size_t)
{
  if (!WaterTankScintillatorHitAllocator) {
    WaterTankScintillatorHitAllocator = new G4Allocator<WaterTankScintillatorHit>;
  }
  return (void*)WaterTankScintillatorHitAllocator->MallocSingle();
}

inline void WaterTankScintillatorHit::operator delete(void* hit)
{
  WaterTankScintillatorHitAllocator->FreeSingle(static_cast<WaterTankScintillatorHit*>(hit));
}

#endif

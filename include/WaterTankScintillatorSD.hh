/// \file WaterTankScintillatorSD.hh
/// \brief Definition of the WaterTankScintillatorSD class

#ifndef WaterTankScintillatorSD_h
#define WaterTankScintillatorSD_h 1

#include "G4VSensitiveDetector.hh"
#include "G4HCofThisEvent.hh"
#include "G4Step.hh"
#include "G4ThreeVector.hh"
#include "G4SDManager.hh"
#include "G4ios.hh"

#include "WaterTankScintillatorHit.hh"

#include <map>

class G4Step;
class G4HCofThisEvent;

/// Sensitive detector for scintillator bars that records charged particle hits.
///
/// This SD triggers on charged particles depositing energy in scintillator bars.
/// It records the earliest hit time per track per bar to simulate the leading
/// edge discriminator behavior of real scintillator readouts.

class WaterTankScintillatorSD : public G4VSensitiveDetector
{
  public:
    WaterTankScintillatorSD(const G4String& name, const G4String& hitsCollectionName);
    virtual ~WaterTankScintillatorSD();
  
    // methods from base class
    virtual void Initialize(G4HCofThisEvent* hitCollection);
    virtual G4bool ProcessHits(G4Step* step, G4TouchableHistory* history);
    virtual void EndOfEvent(G4HCofThisEvent* hitCollection);

    /// Set the minimum energy deposit threshold (default 0.1 MeV).
    void SetEnergyThreshold(G4double threshold) { fEnergyThreshold = threshold; }
    G4double GetEnergyThreshold() const { return fEnergyThreshold; }

  private:
    /// Per-event hits collection.
    WaterTankScintillatorHitsCollection* fHitsCollection;
    /// Cached hits collection ID.
    G4int fHitsCollectionID;
    /// Minimum energy deposit to record a hit.
    G4double fEnergyThreshold;
    
    /// Track earliest hit per (trackID, layer, barIndex) to avoid duplicates.
    /// Key: (trackID * 10000 + layer * 1000 + barIndex)
    std::map<G4int, G4double> fTrackBarHits;
};

#endif

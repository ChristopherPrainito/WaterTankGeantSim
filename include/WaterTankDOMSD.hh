/// \file WaterTankDOMSD.hh
/// \brief Definition of the WaterTankDOMSD class

#ifndef WaterTankDOMSD_h
#define WaterTankDOMSD_h 1

#include "G4VSensitiveDetector.hh"
#include "G4HCofThisEvent.hh"
#include "G4Step.hh"
#include "G4ThreeVector.hh"
#include "G4SDManager.hh"
#include "G4ios.hh"

#include "WaterTankDOMHit.hh"

class G4Step;
class G4HCofThisEvent;
class G4VPhysicalVolume;

/// Sensitive detector that turns optical photons into DOM hits.
///
/// The detector watches the water-to-DOM boundary and, whenever an optical
/// photon crosses into the DOM, evaluates the optical surface acceptance and
/// records a `WaterTankDOMHit` with the photon's kinematics. The owning code
/// provides references to the relevant physical volumes and optical surface.

class WaterTankDOMSD : public G4VSensitiveDetector
{
  public:
    WaterTankDOMSD(const G4String& name, const G4String& hitsCollectionName);
    virtual ~WaterTankDOMSD();
  
    // methods from base class
    virtual void Initialize(G4HCofThisEvent* hitCollection);
    virtual G4bool ProcessHits(G4Step* step, G4TouchableHistory* history);
    virtual void EndOfEvent(G4HCofThisEvent* hitCollection);

    /// Bind the DOM placement so we can recognize boundary crossings.
    void SetDOMPhysicalVolume(const G4VPhysicalVolume* domPhys) { fDOMPhysicalVolume = domPhys; }
    /// Bind the water placement, complementing the DOM volume above.
    void SetWaterPhysicalVolume(const G4VPhysicalVolume* waterPhys) { fWaterPhysicalVolume = waterPhys; }
    /// Provide the optical surface name whose efficiency curve we should sample.
    void SetDOMOpticalSurfaceName(const G4String& surfaceName) { fDOMOpticalSurfaceName = surfaceName; }

  private:
  /// Per-event hits collection pushed into the event at initialization.
  WaterTankDOMHitsCollection* fHitsCollection;
  /// Cached ID used to register the hits collection with the event.
  G4int                       fHitsCollectionID;
  /// Physical placement of the DOM glass sphere.
  const G4VPhysicalVolume*    fDOMPhysicalVolume = nullptr;
  /// Physical placement of the enclosing water volume.
  const G4VPhysicalVolume*    fWaterPhysicalVolume = nullptr;
  /// Name of the logical border surface modeling DOM efficiency.
  G4String                    fDOMOpticalSurfaceName;
};

#endif
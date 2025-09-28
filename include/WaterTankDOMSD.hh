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

/// DOM sensitive detector class
///
/// This class defines a sensitive detector for the DOM (Digital Optical Module).
/// It records hits when particles interact with the DOM glass sphere.

class WaterTankDOMSD : public G4VSensitiveDetector
{
  public:
    WaterTankDOMSD(const G4String& name, const G4String& hitsCollectionName);
    virtual ~WaterTankDOMSD();
  
    // methods from base class
    virtual void Initialize(G4HCofThisEvent* hitCollection);
    virtual G4bool ProcessHits(G4Step* step, G4TouchableHistory* history);
    virtual void EndOfEvent(G4HCofThisEvent* hitCollection);

    void SetDOMPhysicalVolume(const G4VPhysicalVolume* domPhys) { fDOMPhysicalVolume = domPhys; }
    void SetWaterPhysicalVolume(const G4VPhysicalVolume* waterPhys) { fWaterPhysicalVolume = waterPhys; }
    void SetDOMOpticalSurfaceName(const G4String& surfaceName) { fDOMOpticalSurfaceName = surfaceName; }

  private:
  WaterTankDOMHitsCollection* fHitsCollection;
  G4int                       fHitsCollectionID;
  const G4VPhysicalVolume*    fDOMPhysicalVolume = nullptr;
  const G4VPhysicalVolume*    fWaterPhysicalVolume = nullptr;
  G4String                    fDOMOpticalSurfaceName;
};

#endif
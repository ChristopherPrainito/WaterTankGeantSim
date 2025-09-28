/// \file WaterTankDetectorConstruction.hh
/// \brief Definition of the WaterTankDetectorConstruction class

#ifndef WaterTankDetectorConstruction_h
#define WaterTankDetectorConstruction_h 1

#include "G4VUserDetectorConstruction.hh"
#include "globals.hh"

class G4VPhysicalVolume;
class G4LogicalVolume;

/// Detector construction class to define materials and geometry.

class WaterTankDetectorConstruction : public G4VUserDetectorConstruction
{
  public:
    WaterTankDetectorConstruction();
    virtual ~WaterTankDetectorConstruction();

    virtual G4VPhysicalVolume* Construct();
    virtual void ConstructSDandField();
    
    G4LogicalVolume* GetScoringVolume() const { return fScoringVolume; }

  protected:
  G4LogicalVolume*   fScoringVolume;
  G4LogicalVolume*   fDOMLogicalVolume;
  G4LogicalVolume*   fWaterLogicalVolume;
  G4VPhysicalVolume* fWaterPhysicalVolume;
  G4VPhysicalVolume* fDOMPhysicalVolume;
};

#endif

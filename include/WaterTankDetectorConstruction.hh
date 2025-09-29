/// \file WaterTankDetectorConstruction.hh
/// \brief Definition of the WaterTankDetectorConstruction class

#ifndef WaterTankDetectorConstruction_h
#define WaterTankDetectorConstruction_h 1

#include "G4VUserDetectorConstruction.hh"
#include "globals.hh"

class G4VPhysicalVolume;
class G4LogicalVolume;

/// Detector construction that defines the full IceCube-in-a-tank setup.
///
/// This class creates the polypropylene tank, fills it with ultrapure
/// water, suspends the glass DOM sphere, and wires up optical surfaces.
/// It also exposes handles to the water volume used for scoring and the
/// DOM volume so that sensitive detectors can be attached at runtime.

class WaterTankDetectorConstruction : public G4VUserDetectorConstruction
{
  public:
    WaterTankDetectorConstruction();
    virtual ~WaterTankDetectorConstruction();

    virtual G4VPhysicalVolume* Construct();
    virtual void ConstructSDandField();

    /// Accessor to the volume in which energy deposition is tallied.
    G4LogicalVolume* GetScoringVolume() const { return fScoringVolume; }

  protected:
  /// Water volume we use to compute calorimetric observables.
  G4LogicalVolume*   fScoringVolume;
  /// Logical representation of the DOM glass sphere.
  G4LogicalVolume*   fDOMLogicalVolume;
  /// Logical volume for the bulk tank water used for SD binding.
  G4LogicalVolume*   fWaterLogicalVolume;
  /// Physical placement of the water volume (needed to configure surfaces).
  G4VPhysicalVolume* fWaterPhysicalVolume;
  /// Physical placement of the DOM sphere (needed for the sensitive detector).
  G4VPhysicalVolume* fDOMPhysicalVolume;
};

#endif

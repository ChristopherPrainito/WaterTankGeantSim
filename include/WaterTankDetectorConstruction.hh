/// \file WaterTankDetectorConstruction.hh
/// \brief Definition of the WaterTankDetectorConstruction class

#ifndef WaterTankDetectorConstruction_h
#define WaterTankDetectorConstruction_h 1

#include "G4VUserDetectorConstruction.hh"
#include "globals.hh"
#include <vector>

class G4VPhysicalVolume;
class G4LogicalVolume;

/// Detector construction that defines the full IceCube-in-a-tank setup.
///
/// This class creates the polypropylene tank, fills it with ultrapure
/// water, suspends the glass DOM sphere, and wires up optical surfaces.
/// A two-layer scintillator bar array is placed above the tank for
/// cosmic ray muon triggering and time-of-flight measurements.
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

    // Scintillator array configuration accessors
    G4int GetScintBarsPerLayer() const { return fScintBarsPerLayer; }
    G4double GetScintBarLength() const { return fScintBarLength; }
    G4double GetScintBarWidth() const { return fScintBarWidth; }
    G4double GetScintBarThickness() const { return fScintBarThickness; }

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
    
    // Scintillator bar array configuration
    /// Number of bars per layer (forms a square grid coverage).
    G4int fScintBarsPerLayer;
    /// Length of each scintillator bar (along its long axis).
    G4double fScintBarLength;
    /// Width of each scintillator bar (cross-section dimension).
    G4double fScintBarWidth;
    /// Thickness of each scintillator bar (cross-section dimension).
    G4double fScintBarThickness;
    /// Gap between adjacent bars.
    G4double fScintBarGap;
    /// Height above tank top surface for the lower scintillator layer.
    G4double fScintLayerOffset;
    /// Spacing between the two scintillator layers.
    G4double fScintLayerSpacing;
    
    /// Logical volumes for scintillator bars (one per layer).
    std::vector<G4LogicalVolume*> fScintBarLogical;
};

#endif

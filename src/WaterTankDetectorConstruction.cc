/// \file WaterTankDetectorConstruction.cc
/// \brief Implementation of the WaterTankDetectorConstruction class

#include "WaterTankDetectorConstruction.hh"

#include "G4RunManager.hh"
#include "G4NistManager.hh"
#include "G4Box.hh"
#include "G4Cons.hh"
#include "G4Orb.hh"
#include "G4Sphere.hh"
#include "G4Trd.hh"
#include "G4Tubs.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4SystemOfUnits.hh"
#include "G4VisAttributes.hh" // For visualization attributes
#include "G4MaterialPropertiesTable.hh"
#include "G4OpticalSurface.hh"
#include "G4LogicalBorderSurface.hh"
#include "G4PhysicalConstants.hh"
#include "WaterTankDOMSD.hh"
#include "G4SDManager.hh"

WaterTankDetectorConstruction::WaterTankDetectorConstruction()
: G4VUserDetectorConstruction(),
  fScoringVolume(nullptr),
  fDOMLogicalVolume(nullptr),
  fWaterLogicalVolume(nullptr),
  fWaterPhysicalVolume(nullptr),
  fDOMPhysicalVolume(nullptr)
{ }

WaterTankDetectorConstruction::~WaterTankDetectorConstruction()
{ }

G4VPhysicalVolume* WaterTankDetectorConstruction::Construct()
{  
  // Pull materials primarily from the NIST database. This keeps definitions of
  // common materials (polypropylene, water, glass, air) centralized and avoids
  // custom manual compositions unless necessary.
  G4NistManager* nist = G4NistManager::Instance();
  
  // Option to switch on/off checking of volumes overlaps
  G4bool checkOverlaps = true;
 
  // --------------------------------------------------------------
  // Polypropylene cylindrical tank (shell) with ultrapure water fill
  // Specs approximate the IceCube calibration test tank used on surface.
  // --------------------------------------------------------------
  G4double in = 2.54*cm; // inch in Geant4 units

  // Dimensions
  G4double innerRadius = 0.5 * 71.0 * in;   // 35.5" -> 90.17 cm
  G4double wall        = 0.5 * in;          // 0.5"   -> 1.27 cm
  G4double outerRadius = innerRadius + wall; 
  G4double fullHeight  = 36.0 * in;         // 36"    -> 91.44 cm
  G4double halfHeight  = 0.5 * fullHeight;

  // Materials
  G4Material* matPolypropylene = nist->FindOrBuildMaterial("G4_POLYPROPYLENE");
  G4Material* matWater         = nist->FindOrBuildMaterial("G4_WATER"); // Ultrapure water

  // Optical properties for water and DOM glass
  const G4int nOptPhotons = 6;
  G4double photonEnergy[nOptPhotons] =
  {
    2.00*eV,
    2.48*eV,
    3.10*eV,
    3.54*eV,
    3.88*eV,
    4.13*eV
  }; // 620 nm to 300 nm

  G4double refractiveIndexWater[nOptPhotons] =
  {
    1.333,
    1.334,
    1.336,
    1.338,
    1.340,
    1.342
  };

  G4double absorptionWater[nOptPhotons] =
  {
    120.*m,
    110.*m,
    100.*m,
     90.*m,
     80.*m,
     70.*m
  };

  // Attach wavelength-dependent optical constants so Cherenkov photons are
  // refracted/absorbed realistically. Geant4 interpolates between these
  // sample points when propagating optical photons.
  auto waterMPT = new G4MaterialPropertiesTable();
  waterMPT->AddProperty("RINDEX", photonEnergy, refractiveIndexWater, nOptPhotons);
  waterMPT->AddProperty("ABSLENGTH", photonEnergy, absorptionWater, nOptPhotons);
  matWater->SetMaterialPropertiesTable(waterMPT);

  // --------------------------------------------------------------
  // World: air volume sized for cosmic ray simulation
  // --------------------------------------------------------------
  G4double worldHalfXY = 3*m;  // 3m half-width provides adequate simulation volume
  G4double worldHalfZ  = 3*m;  // 3m half-height allows full particle trajectories
  G4Material* world_mat = nist->FindOrBuildMaterial("G4_AIR");

  // The world volume provides sufficient space for particle generation
  // and propagation while avoiding boundary effects.
  G4Box* solidWorld = new G4Box("World", worldHalfXY, worldHalfXY, worldHalfZ);
  G4LogicalVolume* logicWorld = new G4LogicalVolume(solidWorld, world_mat, "World");
  G4VPhysicalVolume* physWorld = new G4PVPlacement(
      0, G4ThreeVector(), logicWorld, "World", 0, false, 0, checkOverlaps);

  // Tank shell (ring)
  G4Tubs* solidTankShell = new G4Tubs(
    "TankShell",
    innerRadius,         // Rmin
    outerRadius,         // Rmax
    halfHeight,          // half length in Z
    0.*deg, 360.*deg
  );
  G4LogicalVolume* logicTankShell = new G4LogicalVolume(
    solidTankShell, matPolypropylene, "TankShell");
  new G4PVPlacement(
    0, G4ThreeVector(), logicTankShell, "TankShell",
    logicWorld, false, 0, checkOverlaps
  );

  // Water volume inside the tank. We shrink the radius/height ever so slightly
  // to eliminate coincident surfaces, which otherwise produce navigation
  // ambiguities for optical photons.
  G4double gap = 0.1*mm; // small tolerance
  G4Tubs* solidTankWater = new G4Tubs(
    "TankWater",
    0.,                   // Rmin
    innerRadius - gap,    // Rmax
    halfHeight - gap,     // half length in Z
    0.*deg, 360.*deg
  );
  G4LogicalVolume* logicTankWater = new G4LogicalVolume(
    solidTankWater, matWater, "TankWater");
  fWaterPhysicalVolume = new G4PVPlacement(
    0, G4ThreeVector(), logicTankWater, "TankWater",
    logicWorld, false, 0, checkOverlaps
  );
  fWaterLogicalVolume = logicTankWater;

  // Visualization attributes: distinct translucent colors
  auto visShell = new G4VisAttributes(G4Colour(0.95, 0.85, 0.1, 0.3)); // translucent yellow
  visShell->SetForceSolid(true);
  logicTankShell->SetVisAttributes(visShell);

  auto visWater = new G4VisAttributes(G4Colour(0.1, 0.3, 0.95, 0.3)); // translucent blue
  visWater->SetForceSolid(true);
  logicTankWater->SetVisAttributes(visWater);

  // Hide world for clarity
  auto visInvisible = new G4VisAttributes(false);
  logicWorld->SetVisAttributes(visInvisible);

  // --------------------------------------------------------------
  // IceCube DOM (Digital Optical Module) in center of tank
  // Simple model: glass sphere (PMT) suspended in water
  // --------------------------------------------------------------
  
  // DOM dimensions (approximate IceCube DOM specs)
  G4double domRadius = 16.5*cm;  // ~13" diameter glass sphere
  
  // Materials
  G4Material* matGlass = nist->FindOrBuildMaterial("G4_Pyrex_Glass");

  G4double refractiveIndexGlass[nOptPhotons] =
  {
    1.470,
    1.471,
    1.473,
    1.475,
    1.476,
    1.478
  };

  G4double absorptionGlass[nOptPhotons] =
  {
     15.*m,
     15.*m,
     15.*m,
     15.*m,
     15.*m,
     15.*m
  };

  auto glassMPT = new G4MaterialPropertiesTable();
  glassMPT->AddProperty("RINDEX", photonEnergy, refractiveIndexGlass, nOptPhotons);
  glassMPT->AddProperty("ABSLENGTH", photonEnergy, absorptionGlass, nOptPhotons);
  matGlass->SetMaterialPropertiesTable(glassMPT);
  
  // DOM glass sphere (PMT housing) - suspended at center
  G4Sphere* solidDOMSphere = new G4Sphere(
    "DOMSphere",
    0.,                 // inner radius
    domRadius,          // outer radius  
    0.*deg, 360.*deg,   // phi range
    0.*deg, 180.*deg    // theta range (full sphere)
  );
  fDOMLogicalVolume = new G4LogicalVolume(
    solidDOMSphere, matGlass, "DOMSphere");
  fDOMPhysicalVolume = new G4PVPlacement(
    0, G4ThreeVector(0, 0, 0), fDOMLogicalVolume, "DOMSphere",
    logicTankWater, false, 0, checkOverlaps
  );
  
  // Visualization for DOM
  auto visDOMGlass = new G4VisAttributes(G4Colour(0.8, 0.9, 1.0, 0.4)); // light blue glass
  visDOMGlass->SetForceSolid(true);
  fDOMLogicalVolume->SetVisAttributes(visDOMGlass);

  // Optical surface to model DOM detection efficiency
  auto domOpticalSurface = new G4OpticalSurface("DOMOpticalSurface");
  domOpticalSurface->SetType(dielectric_metal);
  domOpticalSurface->SetModel(unified);
  domOpticalSurface->SetFinish(polished);

  G4double domEfficiency[nOptPhotons] =
  {
    0.22,
    0.24,
    0.25,
    0.25,
    0.23,
    0.20
  };

  G4double domReflectivity[nOptPhotons] =
  {
    0.05,
    0.05,
    0.05,
    0.05,
    0.05,
    0.05
  };

  // The optical surface encodes an effective detection efficiency curve. We
  // treat it as a "metal" surface so that every photon either gets absorbed
  // (triggering a hit) or reflected based on this curve.
  auto domSurfaceMPT = new G4MaterialPropertiesTable();
  domSurfaceMPT->AddProperty("EFFICIENCY", photonEnergy, domEfficiency, nOptPhotons);
  domSurfaceMPT->AddProperty("REFLECTIVITY", photonEnergy, domReflectivity, nOptPhotons);
  domOpticalSurface->SetMaterialPropertiesTable(domSurfaceMPT);

  // Bind the optical surface to the physical interface bordering water and the
  // DOM. The sensitive detector will later query this same surface to decide
  // whether an incident photon is recorded as a hit.
  if (fWaterPhysicalVolume && fDOMPhysicalVolume) {
    new G4LogicalBorderSurface("DOMOpticalSurfaceBorder",
                               fWaterPhysicalVolume,
                               fDOMPhysicalVolume,
                               domOpticalSurface);
  }

  // Steps inside the water tank drive the energy deposition bookkeeping.
  fScoringVolume = logicTankWater;

  //
  //always return the physical World
  //
  return physWorld;
}

void WaterTankDetectorConstruction::ConstructSDandField()
{
  // Create sensitive detector for DOM. This converts optical photons that
  // enter the DOM into hits and records their kinematics.
  G4String DOMSDname = "WaterTank/DOMSD";
  WaterTankDOMSD* domSD = new WaterTankDOMSD(DOMSDname, "DOMHitsCollection");
  domSD->SetDOMPhysicalVolume(fDOMPhysicalVolume);
  domSD->SetWaterPhysicalVolume(fWaterPhysicalVolume);
  domSD->SetDOMOpticalSurfaceName("DOMOpticalSurfaceBorder");
  G4SDManager::GetSDMpointer()->AddNewDetector(domSD);
  
  // Attach sensitive detector to DOM logical volume
  if (fDOMLogicalVolume) {
    SetSensitiveDetector(fDOMLogicalVolume, domSD);
  }
  if (fWaterLogicalVolume) {
    SetSensitiveDetector(fWaterLogicalVolume, domSD);
  }
}

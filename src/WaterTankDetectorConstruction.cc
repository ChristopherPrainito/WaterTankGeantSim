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
#include "WaterTankScintillatorSD.hh"
#include "G4SDManager.hh"

WaterTankDetectorConstruction::WaterTankDetectorConstruction()
: G4VUserDetectorConstruction(),
  fScoringVolume(nullptr),
  fDOMLogicalVolume(nullptr),
  fWaterLogicalVolume(nullptr),
  fWaterPhysicalVolume(nullptr),
  fDOMPhysicalVolume(nullptr),
  // Scintillator array default configuration
  // Eljen EJ-200 style bars, sized to cover the tank
  fScintBarsPerLayer(12),           // 12 bars per layer
  fScintBarLength(200.*cm),         // 2m long bars
  fScintBarWidth(10.*cm),           // 10cm wide
  fScintBarThickness(2.*cm),        // 2cm thick
  fScintBarGap(0.5*cm),             // 0.5cm gap between bars
  fScintLayerOffset(10.*cm),        // 10cm above tank top
  fScintLayerSpacing(5.*cm)         // 5cm between layers
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

  // Rayleigh scattering lengths in pure water (wavelength-dependent).
  // Rayleigh scattering scales as 1/lambda^4; shorter wavelengths scatter more.
  // Values are approximate for ultrapure water at these energies.
  G4double rayleighWater[nOptPhotons] =
  {
    300.*m,   // 620 nm - long wavelengths scatter less
    150.*m,   // 500 nm
     60.*m,   // 400 nm
     30.*m,   // 350 nm
     18.*m,   // 320 nm
     12.*m    // 300 nm - short wavelengths scatter more
  };

  // Attach wavelength-dependent optical constants so Cherenkov photons are
  // refracted/absorbed/scattered realistically. Geant4 interpolates between these
  // sample points when propagating optical photons.
  auto waterMPT = new G4MaterialPropertiesTable();
  waterMPT->AddProperty("RINDEX", photonEnergy, refractiveIndexWater, nOptPhotons);
  waterMPT->AddProperty("ABSLENGTH", photonEnergy, absorptionWater, nOptPhotons);
  waterMPT->AddProperty("RAYLEIGH", photonEnergy, rayleighWater, nOptPhotons);
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

  // --------------------------------------------------------------
  // Two-layer Scintillator Bar Array (Eljen EJ-200 style)
  // Placed above the tank for cosmic ray muon triggering and TOF
  // Layer 0: bars oriented along X-axis
  // Layer 1: bars oriented along Y-axis (perpendicular lattice)
  // --------------------------------------------------------------
  
  // Scintillator material: polyvinyltoluene base (EJ-200 approximation)
  G4Material* matScintillator = nist->FindOrBuildMaterial("G4_PLASTIC_SC_VINYLTOLUENE");
  
  // Position of the scintillator array above the tank
  G4double tankTopZ = halfHeight;  // Top of tank in world coordinates
  
  G4cout << "\n====== Scintillator Geometry Debug ======" << G4endl;
  G4cout << "Tank halfHeight = " << halfHeight/CLHEP::cm << " cm" << G4endl;
  G4cout << "Tank top Z = +" << tankTopZ/CLHEP::cm << " cm" << G4endl;
  G4cout << "Tank bottom Z = " << -halfHeight/CLHEP::cm << " cm" << G4endl;
  
  // Calculate the total width of one layer of bars
  G4double layerWidth = fScintBarsPerLayer * fScintBarWidth + (fScintBarsPerLayer - 1) * fScintBarGap;
  
  // Visualization attributes for scintillator bars
  auto visScintL0 = new G4VisAttributes(G4Colour(0.0, 0.8, 0.2, 0.5)); // green, translucent
  visScintL0->SetForceSolid(true);
  auto visScintL1 = new G4VisAttributes(G4Colour(0.8, 0.2, 0.8, 0.5)); // purple, translucent
  visScintL1->SetForceSolid(true);
  
  // Layer 0: bars along X-axis (long dimension in X, arrayed in Y)
  G4double layer0Z = tankTopZ + fScintLayerOffset + fScintBarThickness/2.0;
  
  G4cout << "Layer offset from tank top = " << fScintLayerOffset/CLHEP::cm << " cm" << G4endl;
  G4cout << "Layer 0 Z position = +" << layer0Z/CLHEP::cm << " cm (ABOVE tank top)" << G4endl;
  
  G4Box* solidScintBarL0 = new G4Box("ScintBarL0",
    fScintBarLength/2.0,       // half-length in X
    fScintBarWidth/2.0,        // half-width in Y
    fScintBarThickness/2.0);   // half-thickness in Z
  
  G4LogicalVolume* logicScintBarL0 = new G4LogicalVolume(
    solidScintBarL0, matScintillator, "ScintBarL0");
  logicScintBarL0->SetVisAttributes(visScintL0);
  fScintBarLogical.push_back(logicScintBarL0);
  
  // Place Layer 0 bars
  for (G4int iBar = 0; iBar < fScintBarsPerLayer; ++iBar) {
    G4double yPos = -layerWidth/2.0 + fScintBarWidth/2.0 + iBar * (fScintBarWidth + fScintBarGap);
    G4ThreeVector barPos(0, yPos, layer0Z);
    new G4PVPlacement(
      0,                          // no rotation
      barPos,                     // position
      logicScintBarL0,            // logical volume
      "ScintBarL0",               // name
      logicWorld,                 // mother volume
      false,                      // no boolean operation
      iBar,                       // copy number = bar index
      checkOverlaps);
  }
  
  // Layer 1: bars along Y-axis (long dimension in Y, arrayed in X)
  G4double layer1Z = layer0Z + fScintBarThickness/2.0 + fScintLayerSpacing + fScintBarThickness/2.0;
  
  G4cout << "Layer 1 Z position = +" << layer1Z/CLHEP::cm << " cm (ABOVE layer 0)" << G4endl;
  G4cout << "==========================================\n" << G4endl;
  
  G4Box* solidScintBarL1 = new G4Box("ScintBarL1",
    fScintBarWidth/2.0,        // half-width in X
    fScintBarLength/2.0,       // half-length in Y
    fScintBarThickness/2.0);   // half-thickness in Z
  
  G4LogicalVolume* logicScintBarL1 = new G4LogicalVolume(
    solidScintBarL1, matScintillator, "ScintBarL1");
  logicScintBarL1->SetVisAttributes(visScintL1);
  fScintBarLogical.push_back(logicScintBarL1);
  
  // Place Layer 1 bars
  for (G4int iBar = 0; iBar < fScintBarsPerLayer; ++iBar) {
    G4double xPos = -layerWidth/2.0 + fScintBarWidth/2.0 + iBar * (fScintBarWidth + fScintBarGap);
    G4ThreeVector barPos(xPos, 0, layer1Z);
    new G4PVPlacement(
      0,                          // no rotation
      barPos,                     // position
      logicScintBarL1,            // logical volume
      "ScintBarL1",               // name
      logicWorld,                 // mother volume
      false,                      // no boolean operation
      iBar,                       // copy number = bar index
      checkOverlaps);
  }
  
  G4cout << "Scintillator array constructed:" << G4endl;
  G4cout << "  Bars per layer: " << fScintBarsPerLayer << G4endl;
  G4cout << "  Bar dimensions: " << fScintBarLength/cm << " x " << fScintBarWidth/cm << " x " << fScintBarThickness/cm << " cm" << G4endl;
  G4cout << "  Layer 0 (X-oriented) Z: " << layer0Z/cm << " cm" << G4endl;
  G4cout << "  Layer 1 (Y-oriented) Z: " << layer1Z/cm << " cm" << G4endl;

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
  
  // Attach sensitive detector to both DOM and water logical volumes.
  // The SD needs to be on the water volume to catch optical photons as they
  // cross from water into the DOM boundary. The SD logic filters for only
  // boundary-crossing optical photons from water→DOM.
  if (fDOMLogicalVolume) {
    SetSensitiveDetector(fDOMLogicalVolume, domSD);
  }
  if (fWaterLogicalVolume) {
    SetSensitiveDetector(fWaterLogicalVolume, domSD);
  }
  
  // Create sensitive detector for scintillator bars. This records charged
  // particle hits for triggering and time-of-flight measurements.
  G4String ScintSDname = "WaterTank/ScintillatorSD";
  WaterTankScintillatorSD* scintSD = new WaterTankScintillatorSD(ScintSDname, "ScintHitsCollection");
  scintSD->SetEnergyThreshold(0.1*MeV);  // 0.1 MeV threshold
  G4SDManager::GetSDMpointer()->AddNewDetector(scintSD);
  
  // Attach scintillator SD to both layer logical volumes
  for (auto* logVol : fScintBarLogical) {
    if (logVol) {
      SetSensitiveDetector(logVol, scintSD);
    }
  }
}

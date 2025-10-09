/// \file exampleWaterTank.cc
/// \brief Main program of the Water Tank example

#include "WaterTankDetectorConstruction.hh"
#include "WaterTankActionInitialization.hh"
#include "QBBC.hh"

#include "G4RunManagerFactory.hh"
#include "G4SteppingVerbose.hh"
#include "G4UIExecutive.hh"
#include "G4UImanager.hh"
#include "G4VisExecutive.hh"
#include "G4OpticalPhysics.hh"
#include "G4OpticalParameters.hh"
#include <iostream>
#include "G4AnalysisManager.hh"

/// Entry point that configures the Geant4 run manager, physics list, and
// visualization stack before either running in batch mode or opening an
// interactive UI session.
int main(int argc,char** argv)
{
  // Detect interactive mode (if no macro file arguments were provided) and
  // spin up the appropriate UI driver. In interactive mode we keep a pointer
  // around so we can start the session later.
  G4UIExecutive* ui = 0;
  if ( argc == 1 ) {
    ui = new G4UIExecutive(argc, argv);
  }

  // Use the verbose stepping helper that prints coordinates with units to aid
  // in geometry validation during development runs.
  G4int precision = 4;
  G4SteppingVerbose::UseBestUnit(precision);

  // Ensure that individual worker threads merge their ntuples before writing
  // to disk. This keeps output in a single ROOT file even in MT mode.
  G4AnalysisManager::Instance()->SetNtupleMerging(true);

  // Construct the default run manager which owns the detector geometry and
  // orchestrates event processing.
  auto runManager = G4RunManagerFactory::CreateRunManager(G4RunManagerType::Default);

  // Plug in the detector construction which describes the water tank and DOM
  // geometry as well as the material optical properties.
  runManager->SetUserInitialization(new WaterTankDetectorConstruction());

  // Base physics list: QBBC is a standard option tuned for EM + hadronic
  // interactions. We extend it with Geant4's optical physics to model
  // Cherenkov light and WLS processes inside the tank.
  auto physicsList = new QBBC;
  physicsList->SetVerboseLevel(1);

  auto opticalPhysics = new G4OpticalPhysics();
  physicsList->RegisterPhysics(opticalPhysics);

  // Tune the optical physics to produce a realistic Cherenkov photon yield and
  // ensure secondary photons are tracked promptly for accurate timing at the DOM.
  auto opticalParameters = G4OpticalParameters::Instance();
  opticalParameters->SetWLSTimeProfile("delta");
  opticalParameters->SetCerenkovStackPhotons(true);
  opticalParameters->SetCerenkovTrackSecondariesFirst(true);
  opticalParameters->SetCerenkovMaxPhotonsPerStep(300);
  opticalParameters->SetCerenkovMaxBetaChange(10.0);

  runManager->SetUserInitialization(physicsList);
    
  // Register all user actions (primary generator, run/event/stepping hooks).
  runManager->SetUserInitialization(new WaterTankActionInitialization());
  
  // Initialize visualization with the default graphics system so detector
  // geometry and tracks can be rendered if the session is interactive.
  auto visManager = new G4VisExecutive(argc, argv);
  // Constructors can also take optional arguments:
  // - a graphics system of choice, eg. "OGL"
  // - and a verbosity argument - see /vis/verbose guidance.
  // auto visManager = new G4VisExecutive(argc, argv, "OGL", "Quiet");
  // auto visManager = new G4VisExecutive("Quiet");
  visManager->Initialize();

  // Get the pointer to the User Interface manager
  G4UImanager* UImanager = G4UImanager::GetUIpointer();

  // Process macro or start UI session
  if ( ! ui ) { 
    // batch mode
    G4String command = "/control/execute ";
    G4String fileName = argv[1];
    UImanager->ApplyCommand(command+fileName);
  }
  else { 
    // interactive mode
    UImanager->ApplyCommand("/control/execute init_vis.mac");
    ui->SessionStart();
    delete ui;
  }

  // For cleanup we release visualization first; the run manager owns the
  // physics list, detector, and action classes so we do not delete them here.
  delete visManager;
  delete runManager;
}

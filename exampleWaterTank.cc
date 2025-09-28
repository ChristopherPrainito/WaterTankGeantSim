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
#include <iostream>  // Add this line
#include "G4AnalysisManager.hh"

//#include "Randomize.hh"

int main(int argc,char** argv)
{
  // Detect interactive mode (if no arguments) and define UI session
  //
  G4UIExecutive* ui = 0;
  if ( argc == 1 ) {
    ui = new G4UIExecutive(argc, argv);
  }

  // Optionally: choose a different Random engine...
  // G4Random::setTheEngine(new CLHEP::MTwistEngine);
  
  // use G4SteppingVerboseWithUnits
  G4int precision = 4;
  G4SteppingVerbose::UseBestUnit(precision);

  G4AnalysisManager::Instance()->SetNtupleMerging(true);

  // Construct the default run manager
  //
  auto runManager = G4RunManagerFactory::CreateRunManager(G4RunManagerType::Default);

  // Set mandatory initialization classes
  //
  // Detector construction
  runManager->SetUserInitialization(new WaterTankDetectorConstruction());

  // Physics list
  auto physicsList = new QBBC;
  physicsList->SetVerboseLevel(1);

  auto opticalPhysics = new G4OpticalPhysics();
  physicsList->RegisterPhysics(opticalPhysics);

  auto opticalParameters = G4OpticalParameters::Instance();
  opticalParameters->SetWLSTimeProfile("delta");
  opticalParameters->SetCerenkovStackPhotons(true);
  opticalParameters->SetCerenkovTrackSecondariesFirst(true);
  opticalParameters->SetCerenkovMaxPhotonsPerStep(300);
  opticalParameters->SetCerenkovMaxBetaChange(10.0);

  runManager->SetUserInitialization(physicsList);
    
  // User action initialization
  runManager->SetUserInitialization(new WaterTankActionInitialization());
  
  // Initialize visualization with the default graphics system
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
  //
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

  // Job termination
  // Free the store: user actions, physics_list and detector_description are
  // owned and deleted by the run manager, so they should not be deleted 
  // in the main() program !
  
  delete visManager;
  delete runManager;
}

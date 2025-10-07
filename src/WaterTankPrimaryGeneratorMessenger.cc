/// \file WaterTankPrimaryGeneratorMessenger.cc
/// \brief Implementation of the WaterTankPrimaryGeneratorMessenger class

#include "WaterTankPrimaryGeneratorMessenger.hh"
#include "WaterTankPrimaryGeneratorAction.hh"

#include "G4UIdirectory.hh"
#include "G4UIcmdWithABool.hh"
#include "G4UIcmdWithAString.hh"

WaterTankPrimaryGeneratorMessenger::WaterTankPrimaryGeneratorMessenger(WaterTankPrimaryGeneratorAction* generatorAction)
: G4UImessenger(),
  fGeneratorAction(generatorAction)
{
  // Create directory for WaterTank commands
  fWaterTankDirectory = new G4UIdirectory("/watertank/");
  fWaterTankDirectory->SetGuidance("WaterTank detector simulation commands");
  
  // Create directory for generator commands
  fGeneratorDirectory = new G4UIdirectory("/watertank/generator/");
  fGeneratorDirectory->SetGuidance("Primary generator configuration commands");
  
  // Command to switch between single muon and CRY modes
  fUseCRYCmd = new G4UIcmdWithABool("/watertank/generator/useCRY", this);
  fUseCRYCmd->SetGuidance("Enable/disable CRY cosmic ray shower generation");
  fUseCRYCmd->SetGuidance("  true  = Use CRY cosmic ray showers");
  fUseCRYCmd->SetGuidance("  false = Use single muon (default)");
  fUseCRYCmd->SetParameterName("useCRY", false);
  fUseCRYCmd->SetDefaultValue(false);
  fUseCRYCmd->AvailableForStates(G4State_PreInit, G4State_Idle);
  
  // Command to set CRY setup file
  fCRYSetupFileCmd = new G4UIcmdWithAString("/watertank/generator/crySetupFile", this);
  fCRYSetupFileCmd->SetGuidance("Set the CRY setup file path");
  fCRYSetupFileCmd->SetGuidance("The setup file contains CRY configuration parameters");
  fCRYSetupFileCmd->SetParameterName("filename", false);
  fCRYSetupFileCmd->SetDefaultValue("cry_setup.file");
  fCRYSetupFileCmd->AvailableForStates(G4State_PreInit, G4State_Idle);
}

WaterTankPrimaryGeneratorMessenger::~WaterTankPrimaryGeneratorMessenger()
{
  delete fUseCRYCmd;
  delete fCRYSetupFileCmd;
  delete fGeneratorDirectory;
  delete fWaterTankDirectory;
}

void WaterTankPrimaryGeneratorMessenger::SetNewValue(G4UIcommand* command, G4String newValue)
{
  if (command == fUseCRYCmd) {
    G4bool useCRY = fUseCRYCmd->GetNewBoolValue(newValue);
    fGeneratorAction->SetUseCRY(useCRY);
  }
  else if (command == fCRYSetupFileCmd) {
    fGeneratorAction->SetCRYSetupFile(newValue);
  }
}
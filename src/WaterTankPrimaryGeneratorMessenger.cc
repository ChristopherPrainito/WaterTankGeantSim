/// \file WaterTankPrimaryGeneratorMessenger.cc
/// \brief Implementation of the WaterTankPrimaryGeneratorMessenger class

#include "WaterTankPrimaryGeneratorMessenger.hh"
#include "WaterTankPrimaryGeneratorAction.hh"

#include "G4UIdirectory.hh"
#include "G4UIcmdWithABool.hh"
#include "G4UIcmdWithAString.hh"
#include "G4UIcmdWithADoubleAndUnit.hh"
#include "G4UIcmdWith3Vector.hh"
#include "G4UIcmdWith3VectorAndUnit.hh"

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
  
  // Create directory for muon configuration
  fMuonDirectory = new G4UIdirectory("/watertank/generator/muon/");
  fMuonDirectory->SetGuidance("Single muon generator configuration");
  
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
  
  // Muon energy command
  fMuonEnergyCmd = new G4UIcmdWithADoubleAndUnit("/watertank/generator/muon/energy", this);
  fMuonEnergyCmd->SetGuidance("Set kinetic energy of single muon");
  fMuonEnergyCmd->SetParameterName("energy", false);
  fMuonEnergyCmd->SetDefaultValue(4.0);
  fMuonEnergyCmd->SetDefaultUnit("GeV");
  fMuonEnergyCmd->SetUnitCategory("Energy");
  fMuonEnergyCmd->AvailableForStates(G4State_PreInit, G4State_Idle);
  
  // Muon direction command (unit vector)
  fMuonDirectionCmd = new G4UIcmdWith3Vector("/watertank/generator/muon/direction", this);
  fMuonDirectionCmd->SetGuidance("Set momentum direction of single muon (will be normalized)");
  fMuonDirectionCmd->SetGuidance("Example: /watertank/generator/muon/direction 0 0 -1");
  fMuonDirectionCmd->SetParameterName("dx", "dy", "dz", false);
  fMuonDirectionCmd->AvailableForStates(G4State_PreInit, G4State_Idle);
  
  // Muon position command
  fMuonPositionCmd = new G4UIcmdWith3VectorAndUnit("/watertank/generator/muon/position", this);
  fMuonPositionCmd->SetGuidance("Set starting position of single muon");
  fMuonPositionCmd->SetGuidance("Example: /watertank/generator/muon/position 0 0 50 cm");
  fMuonPositionCmd->SetParameterName("x", "y", "z", false);
  fMuonPositionCmd->SetDefaultUnit("cm");
  fMuonPositionCmd->SetUnitCategory("Length");
  fMuonPositionCmd->AvailableForStates(G4State_PreInit, G4State_Idle);
}

WaterTankPrimaryGeneratorMessenger::~WaterTankPrimaryGeneratorMessenger()
{
  delete fMuonEnergyCmd;
  delete fMuonDirectionCmd;
  delete fMuonPositionCmd;
  delete fUseCRYCmd;
  delete fCRYSetupFileCmd;
  delete fMuonDirectory;
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
  else if (command == fMuonEnergyCmd) {
    fGeneratorAction->SetMuonEnergy(fMuonEnergyCmd->GetNewDoubleValue(newValue));
  }
  else if (command == fMuonDirectionCmd) {
    fGeneratorAction->SetMuonDirection(fMuonDirectionCmd->GetNew3VectorValue(newValue));
  }
  else if (command == fMuonPositionCmd) {
    fGeneratorAction->SetMuonPosition(fMuonPositionCmd->GetNew3VectorValue(newValue));
  }
}
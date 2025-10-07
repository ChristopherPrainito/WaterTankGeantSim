/// \file WaterTankPrimaryGeneratorMessenger.hh
/// \brief Definition of the WaterTankPrimaryGeneratorMessenger class

#ifndef WaterTankPrimaryGeneratorMessenger_h
#define WaterTankPrimaryGeneratorMessenger_h 1

#include "G4UImessenger.hh"
#include "globals.hh"

class WaterTankPrimaryGeneratorAction;
class G4UIdirectory;
class G4UIcmdWithABool;
class G4UIcmdWithAString;

/// Messenger class for WaterTankPrimaryGeneratorAction
///
/// This class provides UI commands to control the primary generator:
/// - Switch between single muon and CRY cosmic ray shower modes
/// - Set CRY setup file path
/// - Configure generator parameters

class WaterTankPrimaryGeneratorMessenger : public G4UImessenger
{
  public:
    WaterTankPrimaryGeneratorMessenger(WaterTankPrimaryGeneratorAction* generatorAction);
    virtual ~WaterTankPrimaryGeneratorMessenger();
    
    virtual void SetNewValue(G4UIcommand* command, G4String newValue);
    
  private:
    WaterTankPrimaryGeneratorAction* fGeneratorAction;
    
    G4UIdirectory* fWaterTankDirectory;
    G4UIdirectory* fGeneratorDirectory;
    
    G4UIcmdWithABool* fUseCRYCmd;
    G4UIcmdWithAString* fCRYSetupFileCmd;
};

#endif
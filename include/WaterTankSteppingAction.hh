/// \file WaterTankSteppingAction.hh
/// \brief Definition of the WaterTankSteppingAction class

#ifndef WaterTankSteppingAction_h
#define WaterTankSteppingAction_h 1

#include "G4UserSteppingAction.hh"
#include "globals.hh"

class WaterTankEventAction;

class G4LogicalVolume;

/// Stepping action class
/// 

class WaterTankSteppingAction : public G4UserSteppingAction
{
  public:
    WaterTankSteppingAction(WaterTankEventAction* eventAction);
    virtual ~WaterTankSteppingAction();

    // method from the base class
    virtual void UserSteppingAction(const G4Step*);

  private:
    WaterTankEventAction*  fEventAction;
    G4LogicalVolume* fScoringVolume;
};

#endif

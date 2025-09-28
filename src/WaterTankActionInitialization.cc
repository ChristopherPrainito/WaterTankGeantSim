/// \file WaterTankActionInitialization.cc
/// \brief Implementation of the WaterTankActionInitialization class

#include "WaterTankActionInitialization.hh"
#include "WaterTankPrimaryGeneratorAction.hh"
#include "WaterTankRunAction.hh"
#include "WaterTankEventAction.hh"
#include "WaterTankSteppingAction.hh"

WaterTankActionInitialization::WaterTankActionInitialization()
 : G4VUserActionInitialization()
{}

WaterTankActionInitialization::~WaterTankActionInitialization()
{}

void WaterTankActionInitialization::BuildForMaster() const
{
  WaterTankRunAction* runAction = new WaterTankRunAction;
  SetUserAction(runAction);
}

void WaterTankActionInitialization::Build() const
{
  SetUserAction(new WaterTankPrimaryGeneratorAction);

  WaterTankRunAction* runAction = new WaterTankRunAction;
  SetUserAction(runAction);
  
  WaterTankEventAction* eventAction = new WaterTankEventAction(runAction);
  SetUserAction(eventAction);
  
  SetUserAction(new WaterTankSteppingAction(eventAction));
}

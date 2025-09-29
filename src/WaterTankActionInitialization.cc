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
  // In multi-threaded mode the master thread only needs bookkeeping objects
  // that aggregate results. It does not shoot primaries or step through the
  // geometry, so we only register the run action here.
  WaterTankRunAction* runAction = new WaterTankRunAction;
  SetUserAction(runAction);
}

void WaterTankActionInitialization::Build() const
{
  // Worker threads (or single-threaded runs) need the full suite of actions.
  // Order matters: the primary generator must be in place before events are
  // initialized, and the run action must outlive event/stepping actions that
  // forward data to it.
  SetUserAction(new WaterTankPrimaryGeneratorAction);

  WaterTankRunAction* runAction = new WaterTankRunAction;
  SetUserAction(runAction);
  
  WaterTankEventAction* eventAction = new WaterTankEventAction(runAction);
  SetUserAction(eventAction);
  
  // The stepping action depends on the event action to stash energy deposits.
  SetUserAction(new WaterTankSteppingAction(eventAction));
}

/// \file WaterTankSteppingAction.hh
/// \brief Definition of the WaterTankSteppingAction class

#ifndef WaterTankSteppingAction_h
#define WaterTankSteppingAction_h 1

#include "G4UserSteppingAction.hh"
#include "globals.hh"

class WaterTankEventAction;

class G4LogicalVolume;

/// Collects step-level energy deposition inside the scoring volume.
///
/// Every step, the action checks whether we are inside the water volume used
/// for calorimetry. Non-optical tracks contribute their deposited energy to the
/// event action, while optical photons are ignored to avoid double-counting
/// energy carried by Cherenkov light.

class WaterTankSteppingAction : public G4UserSteppingAction
{
  public:
    WaterTankSteppingAction(WaterTankEventAction* eventAction);
    virtual ~WaterTankSteppingAction();

    // method from the base class
    virtual void UserSteppingAction(const G4Step*);

  private:
    /// Event action that aggregates per-event energy totals.
    WaterTankEventAction*  fEventAction;
    /// Cached pointer to the water scoring volume for quick comparisons.
    G4LogicalVolume* fScoringVolume;
};

#endif

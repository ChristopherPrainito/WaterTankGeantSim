/// \file WaterTankSteppingAction.cc
/// \brief Implementation of the WaterTankSteppingAction class

#include "WaterTankSteppingAction.hh"
#include "WaterTankEventAction.hh"
#include "WaterTankDetectorConstruction.hh"

#include "G4Step.hh"
#include "G4Event.hh"
#include "G4RunManager.hh"
#include "G4LogicalVolume.hh"
#include "G4OpticalPhoton.hh"

WaterTankSteppingAction::WaterTankSteppingAction(WaterTankEventAction* eventAction)
: G4UserSteppingAction(),
  fEventAction(eventAction),
  fScoringVolume(0)
{}

WaterTankSteppingAction::~WaterTankSteppingAction()
{}

void WaterTankSteppingAction::UserSteppingAction(const G4Step* step)
{
  if (!fScoringVolume) { 
    // Lazy-fetch the scoring volume from the detector construction. Doing this
    // once avoids querying the geometry store on every step.
    const WaterTankDetectorConstruction* detectorConstruction
      = static_cast<const WaterTankDetectorConstruction*>
        (G4RunManager::GetRunManager()->GetUserDetectorConstruction());
    fScoringVolume = detectorConstruction->GetScoringVolume();   
  }

  // get volume of the current step
  G4LogicalVolume* volume 
    = step->GetPreStepPoint()->GetTouchableHandle()
      ->GetVolume()->GetLogicalVolume();
      
  // check if we are in scoring volume
  if (volume != fScoringVolume) return;

  // collect energy deposited in this step
  // Do not count optical photons in calorimetry
  if (step->GetTrack()->GetDefinition() == G4OpticalPhoton::OpticalPhotonDefinition()) {
    return;
  }
  // Feed the energy deposit to the event action which will forward it to the
  // run action at the end of the event. This supports both ST and MT modes.
  G4double edepStep = step->GetTotalEnergyDeposit();
  fEventAction->AddEdep(edepStep);
}

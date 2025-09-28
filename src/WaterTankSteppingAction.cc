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
  G4double edepStep = step->GetTotalEnergyDeposit();
  fEventAction->AddEdep(edepStep);  

  G4ThreeVector endPoint = step->GetPostStepPoint()->GetPosition();
  //fEventAction->FillHistograms(edepStep, endPoint.getZ(), endPoint.getX(), endPoint.getY());
}

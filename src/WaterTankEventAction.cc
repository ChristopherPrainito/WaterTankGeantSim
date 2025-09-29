/// \file WaterTankEventAction.cc
/// \brief Implementation of the WaterTankEventAction class

#include "WaterTankEventAction.hh"
#include "WaterTankRunAction.hh"
#include "WaterTankAnalysis.hh"
#include "WaterTankDOMHit.hh"

#include "G4Event.hh"
#include "G4RunManager.hh"
#include "G4ParticleGun.hh"
#include "G4SystemOfUnits.hh"
#include "G4AnalysisManager.hh"
#include "G4SDManager.hh"
#include "G4HCofThisEvent.hh"

WaterTankEventAction::WaterTankEventAction(WaterTankRunAction* runAction)
: G4UserEventAction(),
  fRunAction(runAction),
  fEdep(0.),
  fDetectionCount(0),
  fDOMHCID(-1)
{
}

WaterTankEventAction::~WaterTankEventAction()
{}

void WaterTankEventAction::BeginOfEventAction(const G4Event*)
{    
  // Reset per-event accumulators. The stepping action will add deposited
  // energy, while the sensitive detector will populate hits which we count at
  // the end of the event.
  fEdep = 0.;
  fDetectionCount = 0;
}

void WaterTankEventAction::EndOfEventAction(const G4Event* event)
{   
  // accumulate statistics in run action
  fRunAction->AddEdep(fEdep);
  auto analysisManager = G4AnalysisManager::Instance();
  const G4int eventId = event->GetEventID();

  // Retrieve DOM hits collection and count detections. We cache the collection
  // ID after the first lookup to avoid repeated string-based searches.
  auto hce = event->GetHCofThisEvent();
  WaterTankDOMHitsCollection* domHits = nullptr;
  if (hce) {
    if (fDOMHCID < 0) {
      fDOMHCID = G4SDManager::GetSDMpointer()->GetCollectionID("DOMHitsCollection");
    }
    if (fDOMHCID >= 0 && fDOMHCID < hce->GetNumberOfCollections()) {
      domHits = static_cast<WaterTankDOMHitsCollection*>(hce->GetHC(fDOMHCID));
    }
  }

  fDetectionCount = (domHits) ? static_cast<G4int>(domHits->entries()) : 0;

  analysisManager->FillNtupleIColumn(0, 0, eventId);
  analysisManager->FillNtupleDColumn(0, 1, fEdep/GeV);
  analysisManager->FillNtupleIColumn(0, 2, fDetectionCount);
  analysisManager->AddNtupleRow(0);

  // Populate the hits ntuple with one row per DOM detection. Units are chosen
  // to be human-friendly (ns, eV, nm, cm) for downstream analysis in ROOT.
  if (domHits) {
    for (G4int ihit = 0; ihit < domHits->entries(); ++ihit) {
      auto hit = (*domHits)[ihit];
      if (!hit) continue;
      analysisManager->FillNtupleIColumn(1, 0, eventId);
      analysisManager->FillNtupleIColumn(1, 1, hit->GetTrackID());
      analysisManager->FillNtupleIColumn(1, 2, hit->GetParentID());
      analysisManager->FillNtupleDColumn(1, 3, hit->GetTime()/ns);
      analysisManager->FillNtupleDColumn(1, 4, hit->GetPhotonEnergy()/eV);
      analysisManager->FillNtupleDColumn(1, 5, hit->GetWavelength()/nm);
      const auto& pos = hit->GetPosition();
      analysisManager->FillNtupleDColumn(1, 6, pos.x()/cm);
      analysisManager->FillNtupleDColumn(1, 7, pos.y()/cm);
      analysisManager->FillNtupleDColumn(1, 8, pos.z()/cm);
      const auto& dir = hit->GetDirection();
      analysisManager->FillNtupleDColumn(1, 9, dir.x());
      analysisManager->FillNtupleDColumn(1, 10, dir.y());
      analysisManager->FillNtupleDColumn(1, 11, dir.z());
      analysisManager->AddNtupleRow(1);
    }
  }
}

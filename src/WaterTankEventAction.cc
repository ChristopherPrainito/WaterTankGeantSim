/// \file WaterTankEventAction.cc
/// \brief Implementation of the WaterTankEventAction class

#include "WaterTankEventAction.hh"
#include "WaterTankRunAction.hh"
#include "Analysis.hh"
#include "WaterTankDOMHit.hh"

#include "G4Event.hh"
#include "G4SystemOfUnits.hh"
#include "G4SDManager.hh"
#include "G4HCofThisEvent.hh"

#include <cmath>
#include <limits>

WaterTankEventAction::WaterTankEventAction(WaterTankRunAction* runAction)
: G4UserEventAction(),
  fRunAction(runAction),
  fEdep(0.),
  fDetectionCount(0),
  fDOMHCID(-1),
  fTScintNs(std::numeric_limits<G4double>::quiet_NaN())
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
  fTScintNs = std::numeric_limits<G4double>::quiet_NaN();
}

void WaterTankEventAction::EndOfEventAction(const G4Event* event)
{   
  // accumulate statistics in run action
  fRunAction->AddEdep(fEdep);
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

  G4double firstHitNs = std::numeric_limits<G4double>::quiet_NaN();
  if (domHits) {
    for (G4int ihit = 0; ihit < domHits->entries(); ++ihit) {
      const auto hit = (*domHits)[ihit];
      if (!hit) {
        continue;
      }
      const G4double timeNs = hit->GetTime() / ns;
      if (!std::isfinite(firstHitNs) || timeNs < firstHitNs) {
        firstHitNs = timeNs;
      }
    }
  }

  G4double dtNs = std::numeric_limits<G4double>::quiet_NaN();
  if (std::isfinite(firstHitNs) && std::isfinite(fTScintNs)) {
    dtNs = firstHitNs - fTScintNs;
  }

  Analysis::Instance().RecordEventSummary(fDetectionCount,
                                          firstHitNs,
                                          fTScintNs,
                                          dtNs);
}

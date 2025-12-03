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
#include "G4PrimaryVertex.hh"
#include "G4PrimaryParticle.hh"
#include "G4ThreeVector.hh"
#include <algorithm>
#include <vector>
#include <cmath>

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

  // Get primary particle information
  G4int primaryPDG = 0;
  G4double primaryEnergy = 0.0;
  G4ThreeVector primaryPos(0,0,0);
  G4ThreeVector primaryDir(0,0,0);
  
  auto primaryVertex = event->GetPrimaryVertex();
  if (primaryVertex && primaryVertex->GetNumberOfParticle() > 0) {
    auto primaryParticle = primaryVertex->GetPrimary();
    primaryPDG = primaryParticle->GetPDGcode();
    // Prefer kinetic energy for physics-normalized quantities (exclude rest mass)
    primaryEnergy = primaryParticle->GetKineticEnergy();
    primaryPos = primaryVertex->GetPosition();
    primaryDir = primaryParticle->GetMomentumDirection();
  }

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

  // Calculate physics analysis variables
  G4double photonYield = (primaryEnergy > 0) ? fDetectionCount / (primaryEnergy/GeV) : 0.0;
  G4double firstPhotonTime = 1e9;  // Initialize to large value
  G4double lastPhotonTime = -1e9;  // Initialize to small value
  G4double avgWavelength = 0.0;
  G4double timeRMS = 0.0;
  G4double timeMedian = 0.0;
  
  if (domHits && fDetectionCount > 0) {
    G4double sumWavelength = 0.0;
    G4double sumTime = 0.0;
    G4double sumTime2 = 0.0;
    std::vector<G4double> hitTimes;
    hitTimes.reserve(fDetectionCount);
    
    for (G4int ihit = 0; ihit < domHits->entries(); ++ihit) {
      auto hit = (*domHits)[ihit];
      if (!hit) continue;
      G4double hitTime = hit->GetTime();
      hitTimes.push_back(hitTime);
      sumTime += hitTime;
      sumTime2 += hitTime * hitTime;
      if (hitTime < firstPhotonTime) firstPhotonTime = hitTime;
      if (hitTime > lastPhotonTime) lastPhotonTime = hitTime;
      sumWavelength += hit->GetWavelength();
    }
    avgWavelength = sumWavelength / fDetectionCount;
    
    // Compute time statistics
    G4double meanTime = sumTime / fDetectionCount;
    G4double variance = (sumTime2 / fDetectionCount) - (meanTime * meanTime);
    timeRMS = (variance > 0) ? std::sqrt(variance) : 0.0;
    
    // Compute median time
    if (!hitTimes.empty()) {
      std::sort(hitTimes.begin(), hitTimes.end());
      size_t n = hitTimes.size();
      if (n % 2 == 0) {
        timeMedian = (hitTimes[n/2 - 1] + hitTimes[n/2]) / 2.0;
      } else {
        timeMedian = hitTimes[n/2];
      }
    }
  } else {
    firstPhotonTime = -1.0;  // No photons detected
    lastPhotonTime = -1.0;
  }

  // Fill event ntuple with enhanced data
  analysisManager->FillNtupleIColumn(0, 0, eventId);
  analysisManager->FillNtupleDColumn(0, 1, fEdep/GeV);
  analysisManager->FillNtupleIColumn(0, 2, fDetectionCount);
  analysisManager->FillNtupleIColumn(0, 3, primaryPDG);
  analysisManager->FillNtupleDColumn(0, 4, primaryEnergy/GeV);
  analysisManager->FillNtupleDColumn(0, 5, primaryPos.x()/cm);
  analysisManager->FillNtupleDColumn(0, 6, primaryPos.y()/cm);
  analysisManager->FillNtupleDColumn(0, 7, primaryPos.z()/cm);
  analysisManager->FillNtupleDColumn(0, 8, primaryDir.x());
  analysisManager->FillNtupleDColumn(0, 9, primaryDir.y());
  analysisManager->FillNtupleDColumn(0, 10, primaryDir.z());
  analysisManager->FillNtupleDColumn(0, 11, photonYield);
  analysisManager->FillNtupleDColumn(0, 12, firstPhotonTime/ns);
  analysisManager->FillNtupleDColumn(0, 13, lastPhotonTime/ns);
  analysisManager->FillNtupleDColumn(0, 14, avgWavelength/nm);
  analysisManager->FillNtupleDColumn(0, 15, timeRMS/ns);
  analysisManager->FillNtupleDColumn(0, 16, timeMedian/ns);
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

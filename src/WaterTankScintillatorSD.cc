/// \file WaterTankScintillatorSD.cc
/// \brief Implementation of the WaterTankScintillatorSD class

#include "WaterTankScintillatorSD.hh"

#include "WaterTankScintillatorHit.hh"

#include "G4HCofThisEvent.hh"
#include "G4Step.hh"
#include "G4ThreeVector.hh"
#include "G4SDManager.hh"
#include "G4ios.hh"
#include "G4SystemOfUnits.hh"
#include "G4UnitsTable.hh"
#include "G4OpticalPhoton.hh"
#include "G4VPhysicalVolume.hh"
#include "G4TouchableHandle.hh"

WaterTankScintillatorSD::WaterTankScintillatorSD(const G4String& name,
                                                   const G4String& hitsCollectionName) 
 : G4VSensitiveDetector(name),
   fHitsCollection(nullptr),
   fHitsCollectionID(-1),
   fEnergyThreshold(0.1*MeV)
{
  collectionName.insert(hitsCollectionName);
}

WaterTankScintillatorSD::~WaterTankScintillatorSD() 
{}

void WaterTankScintillatorSD::Initialize(G4HCofThisEvent* hce)
{
  // Allocate a fresh hits collection at the beginning of each event.
  fHitsCollection = new WaterTankScintillatorHitsCollection(SensitiveDetectorName, collectionName[0]);

  if (fHitsCollectionID < 0) {
    fHitsCollectionID = G4SDManager::GetSDMpointer()->GetCollectionID(collectionName[0]);
  }

  if (fHitsCollectionID >= 0) {
    hce->AddHitsCollection(fHitsCollectionID, fHitsCollection);
  }
  
  // Clear the per-event tracking map
  fTrackBarHits.clear();
}

G4bool WaterTankScintillatorSD::ProcessHits(G4Step* aStep, 
                                             G4TouchableHistory*)
{  
  // Ignore optical photons - we only care about charged particles
  auto track = aStep->GetTrack();
  if (track->GetDefinition() == G4OpticalPhoton::OpticalPhotonDefinition()) {
    return false;
  }

  // Get energy deposit
  G4double edep = aStep->GetTotalEnergyDeposit();
  if (edep < fEnergyThreshold) {
    return false;
  }

  // Get touchable to extract copy numbers for layer and bar identification
  auto touchable = aStep->GetPreStepPoint()->GetTouchableHandle();
  if (!touchable) {
    return false;
  }

  // Volume hierarchy: ScintBarL0/L1 (copy=barIndex) -> World
  // The bar copy number is at depth 0
  // Layer is determined from the volume name (ScintBarL0 or ScintBarL1)
  G4int barIndex = touchable->GetCopyNumber(0);  // bar copy number
  
  // Get layer from volume name
  G4String volumeName = touchable->GetVolume()->GetName();
  G4int layer = -1;
  if (volumeName == "ScintBarL0") {
    layer = 0;
  } else if (volumeName == "ScintBarL1") {
    layer = 1;
  } else {
    // Unknown volume, skip
    return false;
  }

  auto prePoint = aStep->GetPreStepPoint();
  G4double hitTime = prePoint->GetGlobalTime();
  G4ThreeVector hitPos = prePoint->GetPosition();
  G4int trackID = track->GetTrackID();
  G4int pdgCode = track->GetDefinition()->GetPDGEncoding();

  // Create unique key for this track in this bar
  G4int key = trackID * 10000 + layer * 1000 + barIndex;
  
  // Check if we already recorded a hit for this track in this bar
  auto it = fTrackBarHits.find(key);
  if (it != fTrackBarHits.end()) {
    // Already have a hit for this track/bar - accumulate energy but keep earliest time
    // For simplicity, we record only the first hit per track per bar
    return false;
  }
  
  // Record this as the earliest hit for this track in this bar
  fTrackBarHits[key] = hitTime;

  // Create hit and fill data
  auto hit = new WaterTankScintillatorHit();
  hit->SetTime(hitTime);
  hit->SetPosition(hitPos);
  hit->SetEdep(edep);
  hit->SetLayer(layer);
  hit->SetBarIndex(barIndex);
  hit->SetTrackID(trackID);
  hit->SetPDGCode(pdgCode);

  fHitsCollection->insert(hit);

  return true;
}

void WaterTankScintillatorSD::EndOfEvent(G4HCofThisEvent*)
{
  // Optional: summarize hits at end of event
}

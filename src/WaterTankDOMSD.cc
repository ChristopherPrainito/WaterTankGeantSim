/// \file WaterTankDOMSD.cc
/// \brief Implementation of the WaterTankDOMSD class

#include "WaterTankDOMSD.hh"

#include "WaterTankDOMHit.hh"

#include "G4HCofThisEvent.hh"
#include "G4Step.hh"
#include "G4ThreeVector.hh"
#include "G4SDManager.hh"
#include "G4ios.hh"
#include "G4SystemOfUnits.hh"
#include "G4UnitsTable.hh"
#include "G4OpticalPhoton.hh"
#include "G4PhysicalConstants.hh"
#include "G4LogicalBorderSurface.hh"
#include "G4MaterialPropertyVector.hh"
#include "G4OpticalSurface.hh"
#include "G4VPhysicalVolume.hh"
#include <Randomize.hh>
#include <algorithm>

WaterTankDOMSD::WaterTankDOMSD(const G4String& name,
                               const G4String& hitsCollectionName) 
 : G4VSensitiveDetector(name),
   fHitsCollection(nullptr),
   fHitsCollectionID(-1)
{
  collectionName.insert(hitsCollectionName);
}

WaterTankDOMSD::~WaterTankDOMSD() 
{}

void WaterTankDOMSD::Initialize(G4HCofThisEvent* hce)
{
  fHitsCollection = new WaterTankDOMHitsCollection(SensitiveDetectorName, collectionName[0]);

  if (fHitsCollectionID < 0) {
    fHitsCollectionID = G4SDManager::GetSDMpointer()->GetCollectionID(collectionName[0]);
  }

  if (fHitsCollectionID >= 0) {
    hce->AddHitsCollection(fHitsCollectionID, fHitsCollection);
  }
}

G4bool WaterTankDOMSD::ProcessHits(G4Step* aStep, 
                                   G4TouchableHistory*)
{  
  auto track = aStep->GetTrack();
  if (track->GetDefinition() != G4OpticalPhoton::OpticalPhotonDefinition()) {
    return false;
  }

  if (!fDOMPhysicalVolume || !fWaterPhysicalVolume) {
    return false;
  }

  auto prePoint = aStep->GetPreStepPoint();
  auto postPoint = aStep->GetPostStepPoint();
  if (!prePoint || !postPoint) {
    return false;
  }

  auto preVolume = prePoint->GetPhysicalVolume();
  auto postVolume = postPoint->GetPhysicalVolume();
  if (!preVolume || !postVolume) {
    return false;
  }

  // Require photon to move from water into the DOM
  if (preVolume != fWaterPhysicalVolume || postVolume != fDOMPhysicalVolume) {
    return false;
  }

  if (postPoint->GetStepStatus() != fGeomBoundary) {
    return false;
  }

  if (!fHitsCollection) {
    return false;
  }

  G4double photonEnergy = postPoint->GetKineticEnergy();
  if (photonEnergy <= 0.) {
    photonEnergy = track->GetKineticEnergy();
  }

  // Determine detection probability from optical surface, if available
  G4double detectionProbability = 1.0;
  if (!fDOMOpticalSurfaceName.empty()) {
    const G4LogicalBorderSurface* borderSurface =
      G4LogicalBorderSurface::GetSurface(preVolume, postVolume);
    if (!borderSurface) {
      borderSurface = G4LogicalBorderSurface::GetSurface(postVolume, preVolume);
    }
    if (borderSurface) {
      auto surfaceProperty = borderSurface->GetSurfaceProperty();
      auto opticalSurface = dynamic_cast<G4OpticalSurface*>(surfaceProperty);
      if (opticalSurface) {
        auto surfaceMPT = opticalSurface->GetMaterialPropertiesTable();
        if (surfaceMPT) {
          auto efficiency = surfaceMPT->GetProperty("EFFICIENCY");
          if (efficiency) {
            detectionProbability = efficiency->Value(photonEnergy);
          }
        }
      }
    }
  }

  if (detectionProbability <= 0.) {
    return false;
  }

  detectionProbability = std::min(1.0, std::max(0.0, detectionProbability));
  if (detectionProbability < 1.0 && G4UniformRand() > detectionProbability) {
    return false;
  }

  auto hit = new WaterTankDOMHit();
  hit->SetTime(postPoint->GetGlobalTime());
  hit->SetPosition(postPoint->GetPosition());
  hit->SetDirection(postPoint->GetMomentumDirection().unit());

  hit->SetPhotonEnergy(photonEnergy);

  G4double wavelength = 0.;
  if (photonEnergy > 0.) {
    wavelength = (h_Planck * c_light) / photonEnergy;
  }
  hit->SetWavelength(wavelength);
  hit->SetTrackID(track->GetTrackID());
  hit->SetParentID(track->GetParentID());

  fHitsCollection->insert(hit);

  G4cout << "DOM HIT: track=" << track->GetTrackID()
         << " parent=" << track->GetParentID()
         << " time=" << postPoint->GetGlobalTime()/ns << " ns"
         << " energy=" << photonEnergy/eV << " eV"
         << " wavelength=" << wavelength/nm << " nm"
         << G4endl;

  track->SetTrackStatus(fStopAndKill);

  return true;
}

void WaterTankDOMSD::EndOfEvent(G4HCofThisEvent*)
{
  // Optional: summarize hits at end of event
}
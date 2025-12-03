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
  // Allocate a fresh hits collection at the beginning of each event. The
  // sensitive detector base class ensures ownership is transferred to the
  // event when we add it via AddHitsCollection.
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
  // Only optical photons are relevant for DOM detections; all charged
  // particles are handled elsewhere (e.g., energy deposition in water).
  auto track = aStep->GetTrack();
  if (track->GetDefinition() != G4OpticalPhoton::OpticalPhotonDefinition()) {
    return false;
  }

  // Bail out early if the detector has not yet been wired with the DOM and
  // water physical volumes. This protects against partial construction states.
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

  // For dielectric_metal surface, the photon is absorbed at the boundary
  // while still "in" the water volume. Check if we're at a geometry boundary
  // with water as preVolume. The postVolume might still be water if the 
  // photon is absorbed/reflected at the surface.
  
  // First check: is this a boundary step from water?
  if (preVolume != fWaterPhysicalVolume) {
    return false;
  }
  
  if (postPoint->GetStepStatus() != fGeomBoundary) {
    return false;
  }
  
  // Check if postVolume is DOM, OR if we're at water boundary with DOM
  // For dielectric_metal, the photon may be absorbed without "entering" DOM
  bool enteringDOM = (postVolume == fDOMPhysicalVolume);
  
  // Also check if we're at the water-DOM boundary by position
  // The DOM is a sphere of radius 16.5cm at origin
  if (!enteringDOM) {
    G4ThreeVector pos = postPoint->GetPosition();
    G4double r = pos.mag();
    G4double domRadius = 16.5 * cm;  // From detector construction
    // If we're at ~DOM radius and step status is boundary, we're hitting DOM
    if (std::abs(r - domRadius) < 1.0*mm) {
      enteringDOM = true;
    }
  }
  
  if (!enteringDOM) {
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
    // Look up the logical border surface spanning water -> DOM. The optical
    // efficiency property captures the DOM quantum efficiency vs. wavelength.
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

  // At this point the photon is deemed detected. Build a hit object capturing
  // arrival time, position, direction, and provenance for downstream analysis.
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

  // Reduce console spam: per-photon printing was flooding logs during large
  // CRY runs. Keep the hit stored for analysis and avoid printing here.
  // If per-hit debugging is needed, re-enable the print below or add a
  // verbosity UI command to toggle it.
  // G4cout << "DOM HIT: track=" << track->GetTrackID()
  //        << " parent=" << track->GetParentID()
  //        << " time=" << postPoint->GetGlobalTime()/ns << " ns"
  //        << " energy=" << photonEnergy/eV << " eV"
  //        << " wavelength=" << wavelength/nm << " nm"
  //        << G4endl;

  // Terminate the optical photon track once it has triggered the DOM to avoid
  // double-counting or spurious reflections in later optical surfaces.
  track->SetTrackStatus(fStopAndKill);

  return true;
}

void WaterTankDOMSD::EndOfEvent(G4HCofThisEvent*)
{
  // Optional: summarize hits at end of event
}
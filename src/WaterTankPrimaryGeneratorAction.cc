/// \file WaterTankPrimaryGeneratorAction.cc
/// \brief Implementation of the WaterTankPrimaryGeneratorAction class

#include "WaterTankPrimaryGeneratorAction.hh"
#include "WaterTankCRYPrimaryGenerator.hh"
#include "WaterTankPrimaryGeneratorMessenger.hh"

#include "G4LogicalVolumeStore.hh"
#include "G4LogicalVolume.hh"
#include "G4Box.hh"
#include "G4RunManager.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4ParticleDefinition.hh"
#include "G4SystemOfUnits.hh"
#include "Randomize.hh"

WaterTankPrimaryGeneratorAction::WaterTankPrimaryGeneratorAction()
: G4VUserPrimaryGeneratorAction(),
  fMode(GeneratorMode::SingleMuon),
  fParticleGun(nullptr),
  fEnvelopeBox(nullptr),
  fMuonEnergy(4.*GeV),
  fMuonDirection(0., 0., 1.),
  fMuonPosition(0., 0., 0.),
  fUseCustomPosition(false),
  fCRYGenerator(nullptr),
  fCRYSetupFile("cry_setup.file"),
  fMessenger(nullptr)
{
  G4int n_particle = 1;
  fParticleGun = new G4ParticleGun(n_particle);

  // Default particle kinematics: single muon for detector calibration
  G4ParticleTable* particleTable = G4ParticleTable::GetParticleTable();
  G4String particleName;
  G4ParticleDefinition* particle
    = particleTable->FindParticle(particleName="mu-");
  fParticleGun->SetParticleDefinition(particle);
  fParticleGun->SetParticleMomentumDirection(fMuonDirection);
  fParticleGun->SetParticleEnergy(fMuonEnergy);
  
  // Create UI messenger
  fMessenger = new WaterTankPrimaryGeneratorMessenger(this);
  
  G4cout << "WaterTankPrimaryGeneratorAction initialized in SingleMuon mode" << G4endl;
  G4cout << "Use '/watertank/generator/useCRY true' to enable CRY cosmic ray showers" << G4endl;
  G4cout << "Configure muon with /watertank/generator/muon/energy, direction, position" << G4endl;
}

WaterTankPrimaryGeneratorAction::~WaterTankPrimaryGeneratorAction()
{
  delete fParticleGun;
  delete fCRYGenerator;
  delete fMessenger;
}

void WaterTankPrimaryGeneratorAction::GeneratePrimaries(G4Event* anEvent)
{
  // Choose generation method based on current mode
  switch (fMode) {
    case GeneratorMode::SingleMuon:
      GenerateSingleMuon(anEvent);
      break;
    case GeneratorMode::CRYShower:
      GenerateCRYShower(anEvent);
      break;
  }
}

void WaterTankPrimaryGeneratorAction::GenerateSingleMuon(G4Event* anEvent)
{
  // Called once per event to spawn the primary vertex.
  // Use user-configured energy, direction, and optionally position.

  // Apply user-configured muon parameters
  fParticleGun->SetParticleEnergy(fMuonEnergy);
  fParticleGun->SetParticleMomentumDirection(fMuonDirection.unit());

  if (fUseCustomPosition) {
    // Use the user-specified position directly
    fParticleGun->SetParticlePosition(fMuonPosition);
  } else {
    // Auto-position: start from bottom of world and go upward (+Z)
    // In order to avoid dependence of PrimaryGeneratorAction
    // on DetectorConstruction class we get World volume
    // from G4LogicalVolumeStore.
    G4double envSizeZ = 0;

    if (!fEnvelopeBox) {
      G4LogicalVolume* envLV
        = G4LogicalVolumeStore::GetInstance()->GetVolume("World");
      if ( envLV ) fEnvelopeBox = dynamic_cast<G4Box*>(envLV->GetSolid());
    }

    if ( fEnvelopeBox ) {
      envSizeZ = fEnvelopeBox->GetZHalfLength()*2.;
    } else {
      G4ExceptionDescription msg;
      msg << "World volume of box shape not found.\n"; 
      msg << "Perhaps you have changed geometry.\n";
      msg << "The gun will be placed at the center.";
      G4Exception("WaterTankPrimaryGeneratorAction::GenerateSingleMuon()",
        "MyCode0002",JustWarning,msg);
    }

    // Position single muons at the bottom of the world volume (for +Z direction)
    G4double x0 = 0.0;
    G4double y0 = 0.0;
    G4double z0 = -0.5 * envSizeZ + 1.*mm; // slightly inside world
    fParticleGun->SetParticlePosition(G4ThreeVector(x0,y0,z0));
  }

  fParticleGun->GeneratePrimaryVertex(anEvent);
}

void WaterTankPrimaryGeneratorAction::GenerateCRYShower(G4Event* anEvent)
{
  // Initialize CRY if not already done
  if (!fCRYGenerator) {
    InitializeCRY();
  }
  
  // Generate cosmic ray shower using CRY
  if (fCRYGenerator && fCRYGenerator->IsInitialized()) {
    fCRYGenerator->GeneratePrimaryVertex(anEvent);
  } else {
    G4ExceptionDescription msg;
    msg << "CRY generator not properly initialized. Falling back to single muon mode.";
    G4Exception("WaterTankPrimaryGeneratorAction::GenerateCRYShower()",
                "CRYMode001", JustWarning, msg);
    GenerateSingleMuon(anEvent);
  }
}

void WaterTankPrimaryGeneratorAction::SetUseCRY(G4bool useCRY)
{
  if (useCRY) {
    fMode = GeneratorMode::CRYShower;
    G4cout << "Switched to CRY cosmic ray shower mode" << G4endl;
    // Initialize CRY if not already done
    if (!fCRYGenerator) {
      InitializeCRY();
    }
  } else {
    fMode = GeneratorMode::SingleMuon;
    G4cout << "Switched to single muon mode" << G4endl;
  }
}

void WaterTankPrimaryGeneratorAction::SetCRYSetupFile(const G4String& filename)
{
  fCRYSetupFile = filename;
  G4cout << "CRY setup file set to: " << filename << G4endl;
  
  // Re-initialize CRY if it was already created
  if (fCRYGenerator) {
    delete fCRYGenerator;
    fCRYGenerator = nullptr;
    if (fMode == GeneratorMode::CRYShower) {
      InitializeCRY();
    }
  }
}

void WaterTankPrimaryGeneratorAction::InitializeCRY()
{
  try {
    fCRYGenerator = new WaterTankCRYPrimaryGenerator(fCRYSetupFile);
    G4cout << "CRY generator initialized with setup file: " << fCRYSetupFile << G4endl;
  } catch (const std::exception& e) {
    G4ExceptionDescription msg;
    msg << "Failed to initialize CRY generator: " << e.what();
    G4Exception("WaterTankPrimaryGeneratorAction::InitializeCRY()",
                "CRYInit001", JustWarning, msg);
    fCRYGenerator = nullptr;
  }
}

void WaterTankPrimaryGeneratorAction::SetMuonEnergy(G4double energy)
{
  fMuonEnergy = energy;
  G4cout << "Muon energy set to: " << energy/GeV << " GeV" << G4endl;
}

void WaterTankPrimaryGeneratorAction::SetMuonDirection(const G4ThreeVector& dir)
{
  fMuonDirection = dir.unit();
  G4cout << "Muon direction set to: (" << fMuonDirection.x() << ", " 
         << fMuonDirection.y() << ", " << fMuonDirection.z() << ")" << G4endl;
}

void WaterTankPrimaryGeneratorAction::SetMuonPosition(const G4ThreeVector& pos)
{
  fMuonPosition = pos;
  fUseCustomPosition = true;
  G4cout << "Muon position set to: (" << pos.x()/cm << ", " 
         << pos.y()/cm << ", " << pos.z()/cm << ") cm" << G4endl;
}

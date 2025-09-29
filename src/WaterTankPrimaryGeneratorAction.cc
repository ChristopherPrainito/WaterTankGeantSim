/// \file WaterTankPrimaryGeneratorAction.cc
/// \brief Implementation of the WaterTankPrimaryGeneratorAction class

#include "WaterTankPrimaryGeneratorAction.hh"

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
  fParticleGun(0), 
  fEnvelopeBox(0)
{
  G4int n_particle = 1;
  fParticleGun  = new G4ParticleGun(n_particle);

  // Default particle kinematics: a single downward-going muon that will
  // produce Cherenkov light as it traverses the water volume.
  G4ParticleTable* particleTable = G4ParticleTable::GetParticleTable();
  G4String particleName;
  G4ParticleDefinition* particle
    = particleTable->FindParticle(particleName="mu-");
  fParticleGun->SetParticleDefinition(particle);
  fParticleGun->SetParticleMomentumDirection(G4ThreeVector(0.,0.,1.));
  fParticleGun->SetParticleEnergy(4.*GeV);
}

WaterTankPrimaryGeneratorAction::~WaterTankPrimaryGeneratorAction()
{
  delete fParticleGun;
}

void WaterTankPrimaryGeneratorAction::GeneratePrimaries(G4Event* anEvent)
{
  // Called once per event to spawn the primary vertex. We orient the muon
  // along +Z and release it just inside the negative Z boundary of the world.

  // In order to avoid dependence of PrimaryGeneratorAction
  // on DetectorConstruction class we get World volume
  // from G4LogicalVolumeStore.

  G4double envSizeXY = 0;
  G4double envSizeZ = 0;

  if (!fEnvelopeBox)
  {
    G4LogicalVolume* envLV
      = G4LogicalVolumeStore::GetInstance()->GetVolume("World");
    if ( envLV ) fEnvelopeBox = dynamic_cast<G4Box*>(envLV->GetSolid());
  }

  if ( fEnvelopeBox ) {
    envSizeXY = fEnvelopeBox->GetXHalfLength()*2.;
    envSizeZ = fEnvelopeBox->GetZHalfLength()*2.;
  }  
  else  {
    G4ExceptionDescription msg;
  msg << "World volume of box shape not found.\n"; 
  msg << "Perhaps you have changed geometry.\n";
  msg << "The gun will be placed at the center.";
    G4Exception("WaterTankPrimaryGeneratorAction::GeneratePrimaries()",
     "MyCode0002",JustWarning,msg);
  }

  // Start muons just outside the world boundary on -Z and shoot toward +Z.
  // Randomizing the starting point later would emulate an angular spread of
  // cosmic rays; for now we keep it deterministic for easier debugging.
  G4double x0 = 0.0;
  G4double y0 = 0.0;
  G4double z0 = -0.5 * envSizeZ + 1.*mm; // slightly inside world to avoid starting outside
  fParticleGun->SetParticleMomentumDirection(G4ThreeVector(0.,0.,1.));
  fParticleGun->SetParticlePosition(G4ThreeVector(x0,y0,z0));

  fParticleGun->GeneratePrimaryVertex(anEvent);
}

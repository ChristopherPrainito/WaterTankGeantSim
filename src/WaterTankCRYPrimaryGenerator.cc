/// \file WaterTankCRYPrimaryGenerator.cc
/// \brief Implementation of the WaterTankCRYPrimaryGenerator class

#include "WaterTankCRYPrimaryGenerator.hh"
#include "G4Event.hh"
#include "G4ParticleDefinition.hh"
#include "G4SystemOfUnits.hh"
#include "G4ThreeVector.hh"
#include "G4Exception.hh"
#include <fstream>
#include <iostream>
#include <sstream>

WaterTankCRYPrimaryGenerator::WaterTankCRYPrimaryGenerator()
: G4VPrimaryGenerator(),
  fParticleGun(nullptr),
  fParticleTable(nullptr),
  fCRYGenerator(nullptr),
  fParticleVector(nullptr),
  fInitialized(false)
{
  Initialize();
}

WaterTankCRYPrimaryGenerator::WaterTankCRYPrimaryGenerator(const G4String& setupFile)
: G4VPrimaryGenerator(),
  fParticleGun(nullptr),
  fParticleTable(nullptr),
  fCRYGenerator(nullptr),
  fParticleVector(nullptr),
  fInitialized(false)
{
  Initialize();
  SetupCRY(setupFile);
}

WaterTankCRYPrimaryGenerator::~WaterTankCRYPrimaryGenerator()
{
  delete fParticleGun;
  delete fCRYGenerator;
  if (fParticleVector) {
    for (auto particle : *fParticleVector) {
      delete particle;
    }
    delete fParticleVector;
  }
}

void WaterTankCRYPrimaryGenerator::Initialize()
{
  // Create particle gun
  fParticleGun = new G4ParticleGun(1);
  
  // Get particle table
  fParticleTable = G4ParticleTable::GetParticleTable();
  
  // Create vector to store CRY particles
  fParticleVector = new std::vector<CRYParticle*>;
}

void WaterTankCRYPrimaryGenerator::SetupCRY(const G4String& setupFile)
{
  // Read the CRY setup file
  std::ifstream inputFile(setupFile);
  if (!inputFile.is_open()) {
    G4ExceptionDescription msg;
    msg << "Failed to open CRY setup file: " << setupFile;
    G4Exception("WaterTankCRYPrimaryGenerator::SetupCRY()",
                "CRYSetup001", FatalException, msg);
    return;
  }
  
  // Read entire file into a string
  std::stringstream buffer;
  buffer << inputFile.rdbuf();
  inputFile.close();
  
  // Setup CRY with the configuration string
  G4String dataPath = CRY_DATA;
  SetupCRY(buffer.str(), dataPath);
}

void WaterTankCRYPrimaryGenerator::SetupCRY(const G4String& setupString, const G4String& dataPath)
{
  try {
    // Create CRY setup
    CRYSetup* setup = new CRYSetup(setupString, dataPath);
    
    // Create CRY generator
    fCRYGenerator = new CRYGenerator(setup);
    
    // Set up random number generator
    RNGWrapper<CLHEP::HepRandomEngine>::set(CLHEP::HepRandom::getTheEngine(), 
                                            &CLHEP::HepRandomEngine::flat);
    setup->setRandomFunction(RNGWrapper<CLHEP::HepRandomEngine>::rng);
    
    fInitialized = true;
    
    G4cout << "CRY generator initialized successfully" << G4endl;
    G4cout << "Data path: " << dataPath << G4endl;
    G4cout << "Setup: " << setupString << G4endl;
    
  } catch (const std::exception& e) {
    G4ExceptionDescription msg;
    msg << "Failed to initialize CRY generator: " << e.what();
    G4Exception("WaterTankCRYPrimaryGenerator::SetupCRY()",
                "CRYSetup002", FatalException, msg);
  }
}

void WaterTankCRYPrimaryGenerator::GeneratePrimaryVertex(G4Event* anEvent)
{
  if (!fInitialized) {
    G4ExceptionDescription msg;
    msg << "CRY generator not initialized. Call SetupCRY() first.";
    G4Exception("WaterTankCRYPrimaryGenerator::GeneratePrimaryVertex()",
                "CRYGenerate001", FatalException, msg);
    return;
  }
  
  // Clear previous particles
  for (auto particle : *fParticleVector) {
    delete particle;
  }
  fParticleVector->clear();
  
  // Generate CRY event
  fCRYGenerator->genEvent(fParticleVector);
  
  G4cout << "Event " << anEvent->GetEventID() 
         << ": CRY generated " << fParticleVector->size() 
         << " particles" << G4endl;
  
  // Convert CRY particles to Geant4 primaries
  for (unsigned int i = 0; i < fParticleVector->size(); i++) {
    CRYParticle* cryParticle = (*fParticleVector)[i];
    
    // Get particle definition
    G4ParticleDefinition* particleDefn = 
      fParticleTable->FindParticle(cryParticle->PDGid());
    
    if (!particleDefn) {
      G4cout << "Warning: Unknown particle PDG ID " << cryParticle->PDGid() 
             << " - skipping" << G4endl;
      continue;
    }
    
    // Set particle properties
    fParticleGun->SetParticleDefinition(particleDefn);
    fParticleGun->SetParticleEnergy(cryParticle->ke() * MeV);
    
    // Use CRY's natural position coordinates within the simulation volume
    // CRY generates particles with realistic spatial and angular distributions
    G4double x_pos = cryParticle->x() * m;
    G4double y_pos = cryParticle->y() * m;
    G4double z_pos = cryParticle->z() * m;
    
    fParticleGun->SetParticlePosition(G4ThreeVector(x_pos, y_pos, z_pos));
    fParticleGun->SetParticleMomentumDirection(G4ThreeVector(cryParticle->u(), 
                                                             cryParticle->v(), 
                                                             cryParticle->w()));
  // CRY returns time in seconds; convert to Geant4 internal units
  fParticleGun->SetParticleTime(cryParticle->t() * s);
    
    // Generate primary vertex
    fParticleGun->GeneratePrimaryVertex(anEvent);
    
    // Verbose output for particle details
    G4cout << "  " << CRYUtils::partName(cryParticle->id()) 
           << " (PDG=" << cryParticle->PDGid() << ")"
           << " E=" << cryParticle->ke() << " MeV"
           << " pos=(" << cryParticle->x() << ", " << cryParticle->y() << ", " << cryParticle->z() << ") m"
           << " dir=(" << cryParticle->u() << ", " << cryParticle->v() << ", " << cryParticle->w() << ")"
           << G4endl;
  }
}
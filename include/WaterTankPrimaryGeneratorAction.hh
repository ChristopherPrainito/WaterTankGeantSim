/// \file WaterTankPrimaryGeneratorAction.hh
/// \brief Definition of the WaterTankPrimaryGeneratorAction class

#ifndef WaterTankPrimaryGeneratorAction_h
#define WaterTankPrimaryGeneratorAction_h 1

#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4ParticleGun.hh"
#include "globals.hh"

class G4ParticleGun;
class G4Event;
class G4Box;
class WaterTankCRYPrimaryGenerator;
class WaterTankPrimaryGeneratorMessenger;

/// Configures the primary particle gun that seeds each event.
///
/// This class supports two modes:
/// 1. Single muon mode: launches a single down-going 4 GeV muon from just
///    outside the world boundary and aims it straight through the tank.
/// 2. CRY mode: uses the CRY cosmic ray shower library to generate realistic
///    cosmic ray showers at sea level for Cambridge, MA conditions.
///
/// The mode can be switched using SetUseCRY() method or via macro commands.

enum class GeneratorMode {
  SingleMuon,
  CRYShower
};

class WaterTankPrimaryGeneratorAction : public G4VUserPrimaryGeneratorAction
{
  public:
    WaterTankPrimaryGeneratorAction();    
    virtual ~WaterTankPrimaryGeneratorAction();

    // method from the base class
    virtual void GeneratePrimaries(G4Event*);         
  
    // Configuration methods
    void SetUseCRY(G4bool useCRY);
    void SetCRYSetupFile(const G4String& filename);
    G4bool GetUseCRY() const { return fMode == GeneratorMode::CRYShower; }
    
    // method to access particle gun
    const G4ParticleGun* GetParticleGun() const { return fParticleGun; }
  
  private:
    /// Mode selection
    GeneratorMode fMode;
    
    /// Single muon mode components
    G4ParticleGun* fParticleGun; ///< Particle gun for single muon mode
    G4Box* fEnvelopeBox; ///< Cached world box for positioning
    
    /// CRY mode components  
    WaterTankCRYPrimaryGenerator* fCRYGenerator; ///< CRY cosmic ray generator
    G4String fCRYSetupFile; ///< Path to CRY setup file
    
    /// UI messenger
    WaterTankPrimaryGeneratorMessenger* fMessenger; ///< UI command messenger
    
    /// Helper methods
    void GenerateSingleMuon(G4Event* anEvent);
    void GenerateCRYShower(G4Event* anEvent);
    void InitializeCRY();
};

#endif

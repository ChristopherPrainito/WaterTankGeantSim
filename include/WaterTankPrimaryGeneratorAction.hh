/// \file WaterTankPrimaryGeneratorAction.hh
/// \brief Definition of the WaterTankPrimaryGeneratorAction class

#ifndef WaterTankPrimaryGeneratorAction_h
#define WaterTankPrimaryGeneratorAction_h 1

#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4ParticleGun.hh"
#include "G4ThreeVector.hh"
#include "globals.hh"

class G4ParticleGun;
class G4Event;
class G4Box;
class WaterTankCRYPrimaryGenerator;
class WaterTankPrimaryGeneratorMessenger;

/// Configures the primary particle gun that seeds each event.
///
/// This class supports two modes:
/// 1. Single muon mode: launches a configurable muon from a specified
///    position and direction through the tank.
/// 2. CRY mode: uses the CRY cosmic ray shower library to generate realistic
///    cosmic ray showers at sea level for Cambridge, MA conditions.
///
/// The mode can be switched using SetUseCRY() method or via macro commands.
/// Single muon parameters can be configured via /watertank/generator/muon/* commands.

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
    
    // Single muon configuration
    void SetMuonEnergy(G4double energy);
    void SetMuonDirection(const G4ThreeVector& dir);
    void SetMuonPosition(const G4ThreeVector& pos);
    
    // method to access particle gun
    const G4ParticleGun* GetParticleGun() const { return fParticleGun; }
  
  private:
    /// Mode selection
    GeneratorMode fMode;
    
    /// Single muon mode components
    G4ParticleGun* fParticleGun; ///< Particle gun for single muon mode
    G4Box* fEnvelopeBox; ///< Cached world box for positioning
    
    /// Single muon configuration (user-settable)
    G4double fMuonEnergy;         ///< Muon kinetic energy
    G4ThreeVector fMuonDirection; ///< Muon momentum direction
    G4ThreeVector fMuonPosition;  ///< Muon starting position
    G4bool fUseCustomPosition;    ///< Whether to use custom position vs auto
    
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

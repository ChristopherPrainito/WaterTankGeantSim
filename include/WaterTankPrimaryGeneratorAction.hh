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

/// Configures the primary particle gun that seeds each event.
///
/// The current setup launches a single down-going 4 GeV muon from just
/// outside the world boundary and aims it straight through the tank.
/// This mimics a cosmic-ray muon traversing the water volume and producing
/// secondary Cherenkov photons that can trigger the DOM.

class WaterTankPrimaryGeneratorAction : public G4VUserPrimaryGeneratorAction
{
  public:
    WaterTankPrimaryGeneratorAction();    
    virtual ~WaterTankPrimaryGeneratorAction();

    // method from the base class
    virtual void GeneratePrimaries(G4Event*);         
  
    // method to access particle gun
    const G4ParticleGun* GetParticleGun() const { return fParticleGun; }
  
  private:
  /// Owned particle gun instance shared across events.
  G4ParticleGun*  fParticleGun; // pointer a to G4 gun class
  /// Cached world box so we can position the muon source relative to it.
  G4Box* fEnvelopeBox;
};

#endif

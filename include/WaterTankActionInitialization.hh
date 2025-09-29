/// \file WaterTankActionInitialization.hh
/// \brief Definition of the WaterTankActionInitialization class

#ifndef WaterTankActionInitialization_h
#define WaterTankActionInitialization_h 1

#include "G4VUserActionInitialization.hh"

/// Bootstraps per-run and per-thread user actions.
///
/// Geant4 asks this object to provide the concrete primary generator,
/// run, event, and stepping actions both for the master thread and worker
/// threads. This is where the simulation wiring between components lives.

class WaterTankActionInitialization : public G4VUserActionInitialization
{
  public:
    WaterTankActionInitialization();
    virtual ~WaterTankActionInitialization();

    virtual void BuildForMaster() const;
    virtual void Build() const;
};

#endif

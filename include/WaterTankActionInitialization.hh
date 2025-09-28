/// \file WaterTankActionInitialization.hh
/// \brief Definition of the WaterTankActionInitialization class

#ifndef WaterTankActionInitialization_h
#define WaterTankActionInitialization_h 1

#include "G4VUserActionInitialization.hh"

/// Action initialization class.

class WaterTankActionInitialization : public G4VUserActionInitialization
{
  public:
    WaterTankActionInitialization();
    virtual ~WaterTankActionInitialization();

    virtual void BuildForMaster() const;
    virtual void Build() const;
};

#endif

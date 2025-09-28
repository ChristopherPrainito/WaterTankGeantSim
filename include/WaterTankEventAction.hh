/// \file WaterTankEventAction.hh
/// \brief Definition of the WaterTankEventAction class

#ifndef WaterTankEventAction_h
#define WaterTankEventAction_h 1

#include "G4UserEventAction.hh"
#include "globals.hh"

class WaterTankRunAction;

/// Event action class
///

class WaterTankEventAction : public G4UserEventAction
{
  public:
    WaterTankEventAction(WaterTankRunAction* runAction);
    virtual ~WaterTankEventAction();

    virtual void BeginOfEventAction(const G4Event* event);
    virtual void EndOfEventAction(const G4Event* event);

    void AddEdep(G4double edep) { fEdep += edep; }
  private:
    WaterTankRunAction* fRunAction;
    G4double     fEdep;
    G4int        fDetectionCount;
    G4int        fDOMHCID;
};

#endif

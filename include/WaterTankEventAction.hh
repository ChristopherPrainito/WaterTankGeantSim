/// \file WaterTankEventAction.hh
/// \brief Definition of the WaterTankEventAction class

#ifndef WaterTankEventAction_h
#define WaterTankEventAction_h 1

#include "G4UserEventAction.hh"
#include "globals.hh"

class WaterTankRunAction;

/// Handles per-event bookkeeping, including DOM and scintillator hit extraction.
///
/// For every event we reset the running totals, collect the total energy
/// deposited in the water scoring volume, and extract hits produced by
/// the DOM and scintillator sensitive detectors. The run action receives the
/// accumulated energy and the analysis manager records event summaries,
/// per-hit information, and time-of-flight calculations.

class WaterTankEventAction : public G4UserEventAction
{
  public:
    WaterTankEventAction(WaterTankRunAction* runAction);
    virtual ~WaterTankEventAction();

    virtual void BeginOfEventAction(const G4Event* event);
    virtual void EndOfEventAction(const G4Event* event);

    /// Accumulate step-level energy deposition into the event total.
    void AddEdep(G4double edep) { fEdep += edep; }
  private:
    /// Back-pointer used to flush event totals into run-level accumulators.
    WaterTankRunAction* fRunAction;
    /// Energy deposited during the current event.
    G4double     fEdep;
    /// How many DOM photon hits were recorded this event.
    G4int        fDetectionCount;
    /// Cached DOM hits collection ID to avoid repeated lookups.
    G4int        fDOMHCID;
    /// Cached scintillator hits collection ID to avoid repeated lookups.
    G4int        fScintHCID;
};

#endif

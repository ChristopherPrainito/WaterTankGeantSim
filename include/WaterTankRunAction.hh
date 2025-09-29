/// \file WaterTankRunAction.hh
/// \brief Definition of the WaterTankRunAction class

#ifndef WaterTankRunAction_h
#define WaterTankRunAction_h 1

#include "G4UserRunAction.hh"
#include "G4Accumulable.hh"
#include "globals.hh"

class G4Run;

/// Collects run-wide observables and manages persistent output.
///
/// The run action owns Geant4 accumulables that receive energy deposition
/// contributions from the stepping action. It opens the ROOT output file,
/// defines ntuples for event and DOM hit summaries, and at the end of the run
/// computes statistics before writing results to disk.

class WaterTankRunAction : public G4UserRunAction
{
  public:
    WaterTankRunAction();
    virtual ~WaterTankRunAction();

  // virtual G4Run* GenerateRun();
  virtual void BeginOfRunAction(const G4Run*);
  virtual void   EndOfRunAction(const G4Run*);

  /// Thread-safe way to accumulate deposited energy.
  void AddEdep (G4double edep);

  private:
  /// Sum of deposited energy across the run (uses Geant4 accumulables).
  G4Accumulable<G4double> fEdep;
  /// Sum of squared deposited energy to compute RMS.
  G4Accumulable<G4double> fEdep2;
  /// Histogram bin width (kept for potential calorimeter maps).
  G4float m_segment;
};

#endif

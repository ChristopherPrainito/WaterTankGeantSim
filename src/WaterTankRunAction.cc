/// \file WaterTankRunAction.cc
/// \brief Implementation of the WaterTankRunAction class

#include "WaterTankRunAction.hh"
#include "WaterTankPrimaryGeneratorAction.hh"
#include "WaterTankDetectorConstruction.hh"
#include "WaterTankAnalysis.hh"
// #include "WaterTankRun.hh"

#include "G4RunManager.hh"
#include "G4Run.hh"
#include "G4AccumulableManager.hh"
#include "G4LogicalVolumeStore.hh"
#include "G4LogicalVolume.hh"
#include "G4UnitsTable.hh"
#include "G4SystemOfUnits.hh"
#include "G4AnalysisManager.hh"
#include "globals.hh"
#include "G4Run.hh"

WaterTankRunAction::WaterTankRunAction()
: G4UserRunAction(),
  fEdep(0.),
  fEdep2(0.)
{ 
  // Register accumulable to the accumulable manager so that thread-local
  // contributions automatically merge at the end of the run.
  G4AccumulableManager* accumulableManager = G4AccumulableManager::Instance();
  accumulableManager->Register(fEdep);
  accumulableManager->Register(fEdep2);

  // Hook up the Geant4 analysis manager. The header WaterTankAnalysis.hh can be
  // used to swap out the backend if we ever want CSV or XML instead of ROOT.
  auto analysisManager = G4AnalysisManager::Instance();
  G4cout << "Using " << analysisManager->GetType() << G4endl;

  // Create directories 
  analysisManager->SetVerboseLevel(1);
  if ( G4Threading::IsMultithreadedApplication() ) analysisManager->SetNtupleMerging(true);

  // Event-level summary ntuple: one row per event capturing how much energy
  // was deposited in the water and how many DOM hits were recorded.
  // Ntuple 0: event
  analysisManager->CreateNtuple("event", "Event summary");
  analysisManager->CreateNtupleIColumn("EventID");           // 0
  analysisManager->CreateNtupleDColumn("Edep_GeV");          // 1
  analysisManager->CreateNtupleIColumn("DOMHitCount");       // 2
  // Primary particle information
  analysisManager->CreateNtupleIColumn("PrimaryPDG");        // 3
  analysisManager->CreateNtupleDColumn("PrimaryEnergy_GeV"); // 4
  analysisManager->CreateNtupleDColumn("PrimaryX_cm");       // 5
  analysisManager->CreateNtupleDColumn("PrimaryY_cm");       // 6
  analysisManager->CreateNtupleDColumn("PrimaryZ_cm");       // 7
  analysisManager->CreateNtupleDColumn("PrimaryDirX");       // 8
  analysisManager->CreateNtupleDColumn("PrimaryDirY");       // 9
  analysisManager->CreateNtupleDColumn("PrimaryDirZ");       // 10
  // Physics analysis variables
  analysisManager->CreateNtupleDColumn("PhotonYield_per_GeV"); // 11
  analysisManager->CreateNtupleDColumn("FirstPhotonTime_ns");  // 12
  analysisManager->CreateNtupleDColumn("LastPhotonTime_ns");   // 13
  analysisManager->CreateNtupleDColumn("AvgPhotonWavelength_nm"); // 14
  // Extended timing statistics for physics validation
  analysisManager->CreateNtupleDColumn("TimeRMS_ns");        // 15
  analysisManager->CreateNtupleDColumn("TimeMedian_ns");     // 16
  // Scintillator trigger information
  analysisManager->CreateNtupleIColumn("ScintHitCount");     // 17
  analysisManager->CreateNtupleIColumn("ScintL0HitCount");   // 18
  analysisManager->CreateNtupleIColumn("ScintL1HitCount");   // 19
  analysisManager->CreateNtupleDColumn("ScintFirstTime_ns"); // 20
  analysisManager->CreateNtupleDColumn("ScintL0FirstTime_ns"); // 21
  analysisManager->CreateNtupleDColumn("ScintL1FirstTime_ns"); // 22
  analysisManager->CreateNtupleIColumn("ScintL0FirstBar");   // 23
  analysisManager->CreateNtupleIColumn("ScintL1FirstBar");   // 24
  analysisManager->CreateNtupleDColumn("ScintTotalEdep_MeV"); // 25
  // Time-of-flight from scintillator to DOM
  analysisManager->CreateNtupleDColumn("TOF_ns");            // 26 (FirstPhotonTime - ScintFirstTime)
  analysisManager->CreateNtupleDColumn("TOF_L0_ns");         // 27 (FirstPhotonTime - ScintL0FirstTime)
  analysisManager->CreateNtupleDColumn("TOF_L1_ns");         // 28 (FirstPhotonTime - ScintL1FirstTime)
  analysisManager->CreateNtupleIColumn("ScintCoincidence");  // 29 (1 if both layers hit, 0 otherwise)
  analysisManager->FinishNtuple();

  // Detailed DOM hit ntuple: one row per detected photon with position,
  // direction, and provenance. This provides the raw material for timing and
  // angular studies when reviewing the simulation output in ROOT.
  // Ntuple 1: domhits
  analysisManager->CreateNtuple("domhits", "DOM photon hits");
  analysisManager->CreateNtupleIColumn("EventID");
  analysisManager->CreateNtupleIColumn("TrackID");
  analysisManager->CreateNtupleIColumn("ParentID");
  analysisManager->CreateNtupleDColumn("Time_ns");
  analysisManager->CreateNtupleDColumn("Energy_eV");
  analysisManager->CreateNtupleDColumn("Wavelength_nm");
  analysisManager->CreateNtupleDColumn("PosX_cm");
  analysisManager->CreateNtupleDColumn("PosY_cm");
  analysisManager->CreateNtupleDColumn("PosZ_cm");
  analysisManager->CreateNtupleDColumn("DirX");
  analysisManager->CreateNtupleDColumn("DirY");
  analysisManager->CreateNtupleDColumn("DirZ");
  analysisManager->FinishNtuple();

  // Ntuple 2: scinthits - detailed scintillator hit information
  analysisManager->CreateNtuple("scinthits", "Scintillator bar hits");
  analysisManager->CreateNtupleIColumn("EventID");           // 0
  analysisManager->CreateNtupleIColumn("Layer");             // 1
  analysisManager->CreateNtupleIColumn("BarIndex");          // 2
  analysisManager->CreateNtupleDColumn("Time_ns");           // 3
  analysisManager->CreateNtupleDColumn("Edep_MeV");          // 4
  analysisManager->CreateNtupleDColumn("PosX_cm");           // 5
  analysisManager->CreateNtupleDColumn("PosY_cm");           // 6
  analysisManager->CreateNtupleDColumn("PosZ_cm");           // 7
  analysisManager->CreateNtupleIColumn("TrackID");           // 8
  analysisManager->CreateNtupleIColumn("PDGCode");           // 9
  analysisManager->FinishNtuple();
}

WaterTankRunAction::~WaterTankRunAction()
{
  //delete G4AnalysisManager::Instance();  
}

void WaterTankRunAction::BeginOfRunAction(const G4Run*)
{ 
  // inform the runManager to save random number seed
  G4RunManager::GetRunManager()->SetRandomNumberStore(false);
  
  // Get analysis manager
  auto analysisManager = G4AnalysisManager::Instance();
  
  // Access detector construction for geometry info if needed.
  // (Previously printed radiation length which was calorimetry-specific.)

  // Write output to a deterministic filename unless changed via macro. ROOT
  // will append a thread suffix automatically when ntuple merging is disabled.
  G4String fileName = "output_default.root";
  analysisManager->OpenFile(fileName);



  // reset accumulables to their initial values
  G4AccumulableManager* accumulableManager = G4AccumulableManager::Instance();
  accumulableManager->Reset();
}

void WaterTankRunAction::EndOfRunAction(const G4Run* run)
{
  G4int nofEvents = run->GetNumberOfEvent();
  if (nofEvents == 0) return;
  
  // Merge accumulables 
  G4AccumulableManager* accumulableManager = G4AccumulableManager::Instance();
  accumulableManager->Merge();

  // Compute average energy deposition and RMS
  //
  G4double edep  = fEdep.GetValue() / nofEvents;
  G4double edep2 = fEdep2.GetValue() / nofEvents;
  
  //G4double rms = edep2 - edep*edep/nofEvents;
  //if (rms > 0.) rms = std::sqrt(rms); else rms = 0.;  
  G4double rms = edep2 - edep*edep;
  if (rms > 0.) rms = std::sqrt(rms); else rms = 0.;  

  // Summarize run conditions for the log so we can cross-check energy and
  // particle species when reviewing outputs. Note: in MT mode the master does
  // not have a primary generator, so we guard against null pointers.
  //  note: There is no primary generator action object for "master"
  //        run manager for multi-threaded mode.
  const WaterTankPrimaryGeneratorAction* generatorAction
   = static_cast<const WaterTankPrimaryGeneratorAction*>
     (G4RunManager::GetRunManager()->GetUserPrimaryGeneratorAction());
  G4String runCondition;
  if (generatorAction)
  {
    if (generatorAction->GetUseCRY()) {
      runCondition += "CRY cosmic ray shower events";
    } else {
      const G4ParticleGun* particleGun = generatorAction->GetParticleGun();
      runCondition += particleGun->GetParticleDefinition()->GetParticleName();
      runCondition += " of ";
      G4double particleEnergy = particleGun->GetParticleEnergy();
      runCondition += G4BestUnit(particleEnergy,"Energy");
    }
  }
        
  // Print
  //  
  if (IsMaster()) {
    G4cout
     << G4endl
     << "--------------------End of Global Run-----------------------";
  }
  else {
    G4cout
     << G4endl
     << "--------------------End of Local Run------------------------";
  }
  
  G4cout
     << G4endl
     << " The run consists of " << nofEvents << " "<< runCondition
     << G4endl
     << " Average energy deposition per particle : " 
     << G4BestUnit(edep,"Energy") << " +/- " << G4BestUnit(rms,"Energy")
     << G4endl
     << "------------------------------------------------------------"
     << G4endl
     << G4endl;
  // Persist histograms and ntuples. The analysis manager owns the file handle,
  // so CloseFile() also triggers writing any buffered data to disk.
  auto analysisManager = G4AnalysisManager::Instance();
  analysisManager->Write();
  analysisManager->CloseFile();
}

void WaterTankRunAction::AddEdep(G4double edep)
{
  // Geant4 accumulables act like thread-local reduction variables. Each call
  // simply adds to the running sum and the merge happens automatically later.
  fEdep  += edep;
  fEdep2 += edep*edep;
}

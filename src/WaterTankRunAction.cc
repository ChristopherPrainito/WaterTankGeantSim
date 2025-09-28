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
  // Register accumulable to the accumulable manager
  G4AccumulableManager* accumulableManager = G4AccumulableManager::Instance();
  accumulableManager->Register(fEdep);
  accumulableManager->Register(fEdep2);

  // Create analysis manager
  // The choice of analysis technology is done via selection of a namespace
  // in WaterTankAnalysis.hh
  auto analysisManager = G4AnalysisManager::Instance();
  G4cout << "Using " << analysisManager->GetType() << G4endl;

  // Create directories 
  analysisManager->SetVerboseLevel(1);
  if ( G4Threading::IsMultithreadedApplication() ) analysisManager->SetNtupleMerging(true);

  analysisManager->CreateNtuple("event", "Event summary");
  analysisManager->CreateNtupleIColumn("EventID");
  analysisManager->CreateNtupleDColumn("Edep_GeV");
  analysisManager->CreateNtupleIColumn("DOMHitCount");
  analysisManager->FinishNtuple();

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
  
  const WaterTankDetectorConstruction* detectorConstruction
      = static_cast<const WaterTankDetectorConstruction*>
        (G4RunManager::GetRunManager()->GetUserDetectorConstruction());
  G4LogicalVolume* scoringVolume = detectorConstruction->GetScoringVolume();
  G4String material = scoringVolume->GetMaterial()->GetName();
  G4cout << "RADIATION LENGTH: " << scoringVolume->GetMaterial()->GetRadlen() << G4endl;

  G4String fileName = "output_default.root";
  analysisManager->OpenFile(fileName);

  //if (!IsMaster()) {
  //  const WaterTankPrimaryGeneratorAction* generatorAction =
  //      static_cast<const WaterTankPrimaryGeneratorAction*>(
  //          G4RunManager::GetRunManager()->GetUserPrimaryGeneratorAction());

  //  if (generatorAction) {
  //    const G4ParticleGun* particleGun = generatorAction->GetParticleGun();
  //    G4String name = particleGun->GetParticleDefinition()->GetParticleName();
  //    G4double energy = particleGun->GetParticleEnergy();
  //    fileName = "ntuple_" + name + "_" + std::to_string((int)energy) + ".root";
  //  }


  //  //// Creating histograms
  //  //analysisManager->CreateNtuple("showerEDep", "Energy Deposition in Volume");
  //  //analysisManager->CreateNtupleDColumn("E");
  //  //analysisManager->CreateNtupleDColumn("z");
  //  //analysisManager->CreateNtupleDColumn("r");
  //  //analysisManager->CreateNtupleDColumn("x");
  //  //analysisManager->CreateNtupleDColumn("y");
  //  //m_segment = 0.050*m;
  //  //m_segment = 0.025*m;
  //  //G4float offset = m_segment / 2;
  //  //G4int nSegments = 1.9*m / m_segment;
  //  //G4int histo2= analysisManager->CreateH2("EdepKTeV", "",
  //  //      nSegments, -0.95*m-offset, 0.95*m-offset,
  //  //      nSegments, -0.95*m-offset, 0.95*m-offset); // This doesn't appear to be filled?
  //  //analysisManager->SetH2Activation(histo2, true);
  //  //analysisManager->FinishNtuple();
  //}
  // Creating histograms
  //analysisManager->OpenFile(fileName);
  //analysisManager->CreateNtuple("showerEDep", "Energy Deposition in Volume");
  //analysisManager->CreateNtupleDColumn("E");
  //analysisManager->CreateNtupleDColumn("z");
  //analysisManager->CreateNtupleDColumn("r");
  //analysisManager->CreateNtupleDColumn("x");
  //analysisManager->CreateNtupleDColumn("y");
  //std::cout << "here1" << std::endl;
  //m_segment = 0.050*m;
  //m_segment = 0.025*m;
  //G4float offset = m_segment / 2;
  //G4int nSegments = 1.9*m / m_segment;
  //G4int histo2= analysisManager->CreateH2("EdepKTeV", "",
  //      nSegments, -0.95*m-offset, 0.95*m-offset,
  //      nSegments, -0.95*m-offset, 0.95*m-offset); // This doesn't appear to be filled?
  //analysisManager->SetH2Activation(histo2, true);
  //analysisManager->FinishNtuple();

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

  // Run conditions
  //  note: There is no primary generator action object for "master"
  //        run manager for multi-threaded mode.
  const WaterTankPrimaryGeneratorAction* generatorAction
   = static_cast<const WaterTankPrimaryGeneratorAction*>
     (G4RunManager::GetRunManager()->GetUserPrimaryGeneratorAction());
  G4String runCondition;
  if (generatorAction)
  {
    const G4ParticleGun* particleGun = generatorAction->GetParticleGun();
    runCondition += particleGun->GetParticleDefinition()->GetParticleName();
    runCondition += " of ";
    G4double particleEnergy = particleGun->GetParticleEnergy();
    runCondition += G4BestUnit(particleEnergy,"Energy");
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
  // save histograms & ntuple
  //
  auto analysisManager = G4AnalysisManager::Instance();
  analysisManager->Write();
  analysisManager->CloseFile();
}

void WaterTankRunAction::AddEdep(G4double edep)
{
  fEdep  += edep;
  fEdep2 += edep*edep;
}

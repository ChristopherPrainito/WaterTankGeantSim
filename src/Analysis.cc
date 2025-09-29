/// \file Analysis.cc
/// \brief Implementation of the project-specific analysis singleton.

#include "Analysis.hh"

#include "G4AnalysisManager.hh"
#include "G4AutoLock.hh"
#include "G4EventManager.hh"
#include "G4Event.hh"
#include "G4GenericMessenger.hh"
#include "G4RunManager.hh"
#include "G4StateManager.hh"
#include "G4SystemOfUnits.hh"
#include "G4Threading.hh"
#include "G4ios.hh"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <limits>
#include <sstream>
#include <string>

namespace
{
  const char* kFileEnvVar = "WATERTANK_ANALYSIS_FILE";
}

G4Mutex Analysis::fMutex = G4MUTEX_INITIALIZER;

Analysis& Analysis::Instance()
{
  static Analysis singleton;
  return singleton;
}

Analysis::Analysis()
{
  if (auto* manager = G4AnalysisManager::Instance()) {
    manager->SetVerboseLevel(1);
    if (G4Threading::IsMultithreadedApplication()) {
      manager->SetNtupleMerging(true);
    }
  }

  const char* envPrefix = std::getenv(kFileEnvVar);
  if (envPrefix && *envPrefix) {
    fFileNamePrefix = envPrefix;
  } else {
    fFileNamePrefix = "run";
  }

  ConfigureMessenger();
}

Analysis::~Analysis()
{
  delete fMessenger;
  fMessenger = nullptr;
}

void Analysis::ConfigureMessenger()
{
  fMessenger = new G4GenericMessenger(this, "/my/analysis/", "Analysis configuration");

  auto& enableCmd = fMessenger->DeclareProperty("enable", fEnabled, "Enable or disable analysis output");
  enableCmd.SetStates(G4State_PreInit, G4State_Idle);

  auto& fileCmd = fMessenger->DeclareProperty("fileName", fFileNamePrefix, "Output file name prefix or pattern");
  fileCmd.SetStates(G4State_PreInit, G4State_Idle);

  auto& hitsCmd = fMessenger->DeclareProperty("saveHits", fSaveHits, "Toggle writing the per-hit DOM ntuple");
  hitsCmd.SetStates(G4State_PreInit, G4State_Idle);

  auto& planarCmd = fMessenger->DeclareProperty("planarFace", fPlanarFace,
                                                "Assume a planar DOM face for XY occupancy histograms");
  planarCmd.SetStates(G4State_PreInit, G4State_Idle);
}

void Analysis::BookObjects()
{
  auto* manager = G4AnalysisManager::Instance();
  if (!manager) {
    return;
  }

  static G4ThreadLocal G4bool booked = false;
  if (booked) {
    return;
  }

  G4AutoLock lock(&fMutex);
  if (booked) {
    return;
  }

  manager->SetVerboseLevel(1);
  if (G4Threading::IsMultithreadedApplication()) {
    manager->SetNtupleMerging(true);
  }

  fH1NPE = manager->CreateH1("h_nPE", "Number of DOM photoelectrons per event", 100, 0., 500.);
  fH1HitTime = manager->CreateH1("h_hitTime_ns", "DOM hit times;time [ns];counts", 200, 0., 500.);
  fH1FirstHit = manager->CreateH1("h_firstHitTime_ns", "Earliest DOM hit per event;time [ns];events", 200, 0., 500.);
  fH1DeltaT = manager->CreateH1("h_dt_scint_dom_ns", "DOM earliest minus scint trigger;#Delta t [ns];events", 400, -200., 800.);
  fH1Wavelength = manager->CreateH1("h_wavelength_nm", "Wavelength of detected photons;wavelength [nm];counts", 200, 250., 650.);
  fH1CosTheta = manager->CreateH1("h_cosTheta", "Cosine of incidence angle at photocathode;cos#theta;counts", 100, -1., 1.);
  fH2XY = manager->CreateH2("h2_xy_hits_mm", "DOM surface occupancy;x [mm];y [mm]", 100, -200., 200., 100, -200., 200.);

  fNtupleEventsId = manager->CreateNtuple("events", "Per-event summary");
  manager->CreateNtupleIColumn("run");
  manager->CreateNtupleIColumn("event");
  manager->CreateNtupleIColumn("nPE");
  manager->CreateNtupleDColumn("t_first_ns");
  manager->CreateNtupleDColumn("t_scint_ns");
  manager->CreateNtupleDColumn("dt_ns");
  manager->FinishNtuple();

  fNtupleHitsId = manager->CreateNtuple("hits", "Per-hit DOM observables");
  manager->CreateNtupleIColumn("event");
  manager->CreateNtupleDColumn("t_ns");
  manager->CreateNtupleDColumn("lambda_nm");
  manager->CreateNtupleDColumn("cosTheta");
  manager->CreateNtupleDColumn("x_mm");
  manager->CreateNtupleDColumn("y_mm");
  manager->FinishNtuple();

  booked = true;
}

void Analysis::ResetRunAccumulators()
{
  G4AutoLock lock(&fMutex);
  fSumNPE = 0.;
  fSumFirstHit = 0.;
  fEventsWithHits = 0;
  fEventsProcessed = 0;
}

void Analysis::BeginRun(G4int runID, const G4String& fileNamePrefix)
{
  auto* manager = G4AnalysisManager::Instance();
  if (!manager) {
    return;
  }

  if (!fEnabled) {
    G4cout << "[Analysis] Disabled, no output will be produced." << G4endl;
    fRunActive = false;
    return;
  }

  if (!fileNamePrefix.empty()) {
    fFileNamePrefix = fileNamePrefix;
  }

  BookObjects();

  if (G4Threading::IsMasterThread()) {
    ResetRunAccumulators();
    fRunID = runID;

    const G4String fileName = BuildOutputFileName(runID);
    manager->OpenFile(fileName);

    G4cout << "[Analysis] Writing run " << runID << " to " << fileName
           << " using backend " << manager->GetType() << G4endl;
  }

  fRunActive = true;
}

void Analysis::EndRun()
{
  if (!fRunActive) {
    return;
  }

  auto* manager = G4AnalysisManager::Instance();
  if (!manager) {
    return;
  }

  if (!G4Threading::IsMasterThread()) {
    return;
  }

  manager->Write();
  manager->CloseFile();
  fRunActive = false;

  const G4double meanNPE = (fEventsProcessed > 0)
                               ? fSumNPE / static_cast<G4double>(fEventsProcessed)
                               : 0.;
  const G4double meanFirstHit = (fEventsWithHits > 0)
                                    ? fSumFirstHit / static_cast<G4double>(fEventsWithHits)
                                    : std::numeric_limits<G4double>::quiet_NaN();

  std::ostringstream summary;
  summary.setf(std::ios::fixed);
  summary << std::setprecision(3);
  summary << "[Analysis] Run " << fRunID << " summary: <NPE>=" << meanNPE;
  if (std::isfinite(meanFirstHit)) {
    summary << " <t_first>=" << meanFirstHit << " ns";
  } else {
    summary << " <t_first>=N/A";
  }

  G4cout << summary.str() << G4endl;
}

void Analysis::CountPE(G4double time_ns,
                       G4double wavelength_nm,
                       G4double cosTheta,
                       G4double x_mm,
                       G4double y_mm)
{
  auto* manager = G4AnalysisManager::Instance();
  if (!manager || !fRunActive || !fEnabled) {
    return;
  }

  if (fH1HitTime >= 0 && std::isfinite(time_ns)) {
    manager->FillH1(fH1HitTime, time_ns);
  }
  if (fH1Wavelength >= 0 && std::isfinite(wavelength_nm)) {
    manager->FillH1(fH1Wavelength, wavelength_nm);
  }
  if (fH1CosTheta >= 0 && std::isfinite(cosTheta)) {
    manager->FillH1(fH1CosTheta, cosTheta);
  }
  if (fH2XY >= 0 && std::isfinite(x_mm) && std::isfinite(y_mm) && fPlanarFace) {
    manager->FillH2(fH2XY, x_mm, y_mm);
  }

  if (fSaveHits && fNtupleHitsId >= 0) {
    const G4int eventID = CurrentEventID();
    manager->FillNtupleIColumn(fNtupleHitsId, 0, eventID);
    manager->FillNtupleDColumn(fNtupleHitsId, 1, time_ns);
    manager->FillNtupleDColumn(fNtupleHitsId, 2, wavelength_nm);
    manager->FillNtupleDColumn(fNtupleHitsId, 3, cosTheta);
    manager->FillNtupleDColumn(fNtupleHitsId, 4, x_mm);
    manager->FillNtupleDColumn(fNtupleHitsId, 5, y_mm);
    manager->AddNtupleRow(fNtupleHitsId);
  }
}

void Analysis::RecordEventSummary(G4int nPE,
                                  G4double t_first_ns,
                                  G4double t_scint_ns,
                                  G4double dt_ns)
{
  auto* manager = G4AnalysisManager::Instance();
  if (!manager || !fRunActive || !fEnabled) {
    return;
  }

  if (fH1NPE >= 0) {
    manager->FillH1(fH1NPE, static_cast<G4double>(nPE));
  }
  if (fH1FirstHit >= 0 && std::isfinite(t_first_ns)) {
    manager->FillH1(fH1FirstHit, t_first_ns);
  }
  if (fH1DeltaT >= 0 && std::isfinite(dt_ns)) {
    manager->FillH1(fH1DeltaT, dt_ns);
  }

  {
    G4AutoLock lock(&fMutex);
    ++fEventsProcessed;
    fSumNPE += static_cast<G4double>(nPE);
    if (std::isfinite(t_first_ns)) {
      fSumFirstHit += t_first_ns;
      ++fEventsWithHits;
    }
  }

  if (fNtupleEventsId >= 0) {
    const G4int eventID = CurrentEventID();
    manager->FillNtupleIColumn(fNtupleEventsId, 0, fRunID);
    manager->FillNtupleIColumn(fNtupleEventsId, 1, eventID);
    manager->FillNtupleIColumn(fNtupleEventsId, 2, nPE);
    manager->FillNtupleDColumn(fNtupleEventsId, 3, t_first_ns);
    manager->FillNtupleDColumn(fNtupleEventsId, 4, t_scint_ns);
    manager->FillNtupleDColumn(fNtupleEventsId, 5, dt_ns);
    manager->AddNtupleRow(fNtupleEventsId);
  }
}

G4String Analysis::BuildOutputFileName(G4int runID) const
{
  G4String prefix = fFileNamePrefix;
  if (prefix.empty()) {
    prefix = "run";
  }

  // If user provides a placeholder, replace it with run number.
  if (prefix.find("{run}") != std::string::npos) {
    std::ostringstream runFmt;
    runFmt << std::setw(4) << std::setfill('0') << runID;
    std::string updated = prefix;
    updated.replace(updated.find("{run}"), 5, runFmt.str());
    return updated;
  }

  // If the user specified an explicit extension, respect it and skip suffixing.
  if (prefix.find('.') != std::string::npos) {
    return prefix;
  }

  std::ostringstream name;
  name << prefix << "_" << std::setw(4) << std::setfill('0') << runID << DetectDefaultExtension();
  return name.str();
}

G4String Analysis::DetectDefaultExtension() const
{
  auto* manager = G4AnalysisManager::Instance();
  if (!manager) {
    return ".dat";
  }

  const G4String backendType = manager->GetType();
  std::string backendLower = backendType;
  std::transform(backendLower.begin(), backendLower.end(), backendLower.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  if (backendLower.find("root") != std::string::npos) {
    return ".root";
  }
  if (backendLower.find("xml") != std::string::npos) {
    return ".xml";
  }
  if (backendLower.find("csv") != std::string::npos) {
    return ".csv";
  }
  return ".dat";
}

G4int Analysis::CurrentEventID() const
{
  auto eventManager = G4EventManager::GetEventManager();
  if (!eventManager) {
    return -1;
  }
  const G4Event* event = eventManager->GetConstCurrentEvent();
  return event ? event->GetEventID() : -1;
}

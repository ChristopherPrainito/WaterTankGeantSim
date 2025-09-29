/// \file Analysis.hh
/// \brief Singleton wrapper around G4AnalysisManager with project-specific helpers.

#ifndef WaterTank_Analysis_hh
#define WaterTank_Analysis_hh 1

#include "globals.hh"

#include "G4AnalysisManager.hh"
#include "G4Threading.hh"

#include <limits>

class G4GenericMessenger;

/// Central analysis service that books/records run and event observables.
///
/// The class wraps Geant4's analysis manager, providing a single point where
/// histograms and ntuples are booked and filled. It is implemented as a
/// process-wide singleton so that user actions can access it without managing
/// ownership or lifetimes explicitly.
class Analysis
{
  public:
    /// Retrieve the singleton instance (created on first use).
    static Analysis& Instance();

    /// Open an output file for the given run and (re-)book histograms/ntuples.
    void BeginRun(G4int runID, const G4String& fileNamePrefix = "");
    /// Flush buffered histograms/ntuples and close the output file.
    void EndRun();

    /// Record a detected photoelectron with per-hit observables.
    void CountPE(G4double time_ns,
                 G4double wavelength_nm,
                 G4double cosTheta,
                 G4double x_mm,
                 G4double y_mm);

    /// Store per-event summary observables (always called once per event).
    void RecordEventSummary(G4int nPE,
                             G4double t_first_ns,
                             G4double t_scint_ns,
                             G4double dt_ns);

    /// Optional hint to enable planar DOM occupancy (no-op for spherical DOMs).
    void SetPlanarFace(G4bool hasPlanarFace) { fPlanarFace = hasPlanarFace; }

    /// Allow other components to query current configuration flags.
    G4bool IsEnabled() const { return fEnabled; }
    G4bool SaveHits() const { return fSaveHits; }
    const G4String& GetFileNamePrefix() const { return fFileNamePrefix; }

  private:
    Analysis();
    ~Analysis();

    Analysis(const Analysis&) = delete;
    Analysis& operator=(const Analysis&) = delete;

    void ConfigureMessenger();
    void BookObjects();
    void ResetRunAccumulators();
    G4String BuildOutputFileName(G4int runID) const;
    G4String DetectDefaultExtension() const;
    G4int CurrentEventID() const;

  private:
    G4GenericMessenger* fMessenger = nullptr;

    G4bool fEnabled = true;
    G4bool fSaveHits = false;
    G4bool fPlanarFace = false;
    G4bool fRunActive = false;

    G4String fFileNamePrefix;
    G4int fRunID = -1;

    // Histogram identifiers (cached after booking).
    G4int fH1NPE = -1;
    G4int fH1HitTime = -1;
    G4int fH1FirstHit = -1;
    G4int fH1DeltaT = -1;
    G4int fH1Wavelength = -1;
    G4int fH1CosTheta = -1;
    G4int fH2XY = -1;

    // Ntuple identifiers.
    G4int fNtupleEventsId = -1;
    G4int fNtupleHitsId = -1;

    // Per-run summary stats (protected by a mutex in MT builds).
    G4double fSumNPE = 0.;
    G4double fSumFirstHit = 0.;
    G4int fEventsWithHits = 0;
    G4int fEventsProcessed = 0;

    static G4Mutex fMutex;
};

#endif

// ========================================================
// Water Tank Analysis ROOT Macro
// ========================================================
// Comprehensive analysis and plotting of water tank simulation data
// for IceCube DOM calibration studies with Cherenkov light detection
// and scintillator array time-of-flight measurements
// Run with: root -l analyze_watertank.C
// Or from ROOT prompt: .x analyze_watertank.C

#include <TFile.h>
#include <TTree.h>
#include <TH1F.h>
#include <TH2F.h>
#include <TCanvas.h>
#include <TGraph.h>
#include <TLegend.h>
#include <TStyle.h>
#include <TMath.h>
#include <TEllipse.h>
#include <TF1.h>
#include <TLine.h>
#include <TLatex.h>
#include <TBox.h>
#include <iostream>
#include <vector>
#include <cmath>

// Physical constants
const double c_light = 29.9792458; // cm/ns (speed of light)
const double DOM_RADIUS = 16.5;    // cm
const double TANK_HALF_HEIGHT = 45.72; // cm (36" / 2)

// Scintillator array geometry (must match DetectorConstruction)
const int SCINT_BARS_PER_LAYER = 12;
const double SCINT_BAR_LENGTH = 200.0;  // cm
const double SCINT_BAR_WIDTH = 10.0;    // cm
const double SCINT_LAYER0_Z = 55.72;    // cm (tank top + offset + half thickness)
const double SCINT_LAYER1_Z = 62.72;    // cm (layer0 + spacing + thickness)

// Refractive index of water as function of wavelength (nm)
// Interpolated from simulation values
double getRefractiveIndex(double wavelength_nm) {
    // Data points from simulation: (wavelength nm, n)
    // 620nm->1.333, 500nm->1.334, 400nm->1.336, 350nm->1.338, 320nm->1.340, 300nm->1.342
    if (wavelength_nm >= 620) return 1.333;
    if (wavelength_nm <= 300) return 1.342;
    
    // Linear interpolation
    double wl[] = {300, 320, 350, 400, 500, 620};
    double n[]  = {1.342, 1.340, 1.338, 1.336, 1.334, 1.333};
    
    for (int i = 0; i < 5; i++) {
        if (wavelength_nm >= wl[i] && wavelength_nm <= wl[i+1]) {
            double frac = (wavelength_nm - wl[i]) / (wl[i+1] - wl[i]);
            return n[i] + frac * (n[i+1] - n[i]);
        }
    }
    return 1.337; // fallback average
}

// Expected Cherenkov angle for beta~1 particle
double getCherenkovAngle(double n) {
    // cos(theta_c) = 1/(beta*n), for beta~1: theta_c = acos(1/n)
    if (n <= 1.0) return 0;
    return TMath::ACos(1.0/n) * 180.0 / TMath::Pi(); // degrees
}

void analyze_watertank(const char* filename = "output_default.root") {
    
    std::cout << "=== Water Tank Simulation Analysis ===" << std::endl;
    std::cout << "Opening file: " << filename << std::endl;
    
    // Open ROOT file
    TFile *file = TFile::Open(filename);
    if (!file || file->IsZombie()) {
        std::cerr << "Error: Cannot open file " << filename << std::endl;
        return;
    }
    
    // Get trees
    TTree *eventTree = (TTree*)file->Get("event");
    TTree *domhitsTree = (TTree*)file->Get("domhits");
    
    if (!eventTree || !domhitsTree) {
        std::cerr << "Error: Cannot find required trees in file" << std::endl;
        file->Close();
        return;
    }
    
    std::cout << "Event tree entries: " << eventTree->GetEntries() << std::endl;
    std::cout << "DOM hits tree entries: " << domhitsTree->GetEntries() << std::endl;
    
    // Get scintillator hits tree (new)
    TTree *scinthitsTree = (TTree*)file->Get("scinthits");
    if (scinthitsTree) {
        std::cout << "Scintillator hits tree entries: " << scinthitsTree->GetEntries() << std::endl;
    } else {
        std::cout << "Note: No scinthits tree found (may be older data format)" << std::endl;
    }
    
    // Set ROOT style for better plots
    gStyle->SetOptStat(111111);
    gStyle->SetPalette(1);
    gStyle->SetGridStyle(3);
    gStyle->SetGridWidth(1);
    gStyle->SetGridColor(kGray);
    
    // Optimize default margins and spacing for multi-panel layouts
    gStyle->SetPadTopMargin(0.12);
    gStyle->SetPadBottomMargin(0.12);
    gStyle->SetPadLeftMargin(0.12);
    gStyle->SetPadRightMargin(0.10);
    gStyle->SetTitleOffset(1.2, "X");
    gStyle->SetTitleOffset(1.3, "Y");
    
    // Smaller histogram titles to prevent overlap
    gStyle->SetTitleFontSize(0.028);
    gStyle->SetTitleH(0.08);
    
    // ========================================================
    // Event-level Analysis
    // PURPOSE: Analyze primary particle characteristics and overall event properties
    // These plots help validate cosmic ray flux, energy distributions, and detector response
    // ========================================================
    
    // Create canvas for event analysis
    TCanvas *c1 = new TCanvas("c1", "Water Tank Event-Level Physics Analysis", 1400, 900);
    c1->Divide(3, 2);
    c1->SetBorderMode(0);
    c1->SetFrameBorderMode(0);
    
    // 1. Primary energy distribution
    // PURPOSE: Shows the energy spectrum of incident cosmic ray muons
    // USE: Validates CRY cosmic ray generation matches expected atmospheric muon spectrum
    c1->cd(1);
    gPad->SetGrid(1,1);
    gPad->SetTickx(1);
    gPad->SetTicky(1);
    gPad->SetTopMargin(0.15);
    TH1F *h_energy = new TH1F("h_energy", "Incident Muon Energy Distribution", 50, 0, 10);
    h_energy->SetXTitle("Primary Muon Energy [GeV]");
    h_energy->SetYTitle("Number of Events");
    h_energy->SetTitleSize(0.032, "XY");
    h_energy->SetLabelSize(0.028, "XY");
    eventTree->Draw("PrimaryEnergy_GeV>>h_energy", "", "");
    h_energy->SetFillColor(kBlue-3);
    h_energy->SetLineColor(kBlue+2);
    h_energy->SetLineWidth(2);
    
    // 2. Energy deposition in water
    // PURPOSE: Shows energy lost by muons via ionization (dE/dx losses)
    // USE: Validates muon energy loss matches Bethe-Bloch formula (~2 MeV/cm in water)
    c1->cd(2);
    gPad->SetGrid(1,1);
    gPad->SetTickx(1);
    gPad->SetTicky(1);
    gPad->SetTopMargin(0.15);
    TH1F *h_edep = new TH1F("h_edep", "Muon Energy Loss in Water Tank", 50, 0, 0.5);
    h_edep->SetXTitle("Energy Deposited [GeV]");
    h_edep->SetYTitle("Number of Events");
    h_edep->SetTitleSize(0.032, "XY");
    h_edep->SetLabelSize(0.028, "XY");
    eventTree->Draw("Edep_GeV>>h_edep", "Edep_GeV>0", "");
    h_edep->SetFillColor(kRed-3);
    h_edep->SetLineColor(kRed+2);
    h_edep->SetLineWidth(2);
    
    // 3. Cherenkov photon multiplicity
    // PURPOSE: Shows number of photons detected per event (light collection efficiency)
    // USE: Higher multiplicity = better reconstruction, crucial for trigger design
    c1->cd(3);
    gPad->SetGrid(1,1);
    gPad->SetTickx(1);
    gPad->SetTicky(1);
    gPad->SetTopMargin(0.15);
    TH1F *h_hits = new TH1F("h_hits", "Cherenkov Light Collection per Event", 100, 0, 2000);
    h_hits->SetXTitle("Detected Photons per Event");
    h_hits->SetYTitle("Number of Events");
    h_hits->SetTitleSize(0.032, "XY");
    h_hits->SetLabelSize(0.028, "XY");
    eventTree->Draw("DOMHitCount>>h_hits", "DOMHitCount>0", "");
    h_hits->SetFillColor(kGreen-3);
    h_hits->SetLineColor(kGreen+2);
    h_hits->SetLineWidth(2);
    
    // 4. Light yield correlation
    // PURPOSE: Correlation between muon energy and detected photons (fundamental Cherenkov physics)
    // USE: Should show linear relationship - validates Cherenkov radiation modeling
    c1->cd(4);
    gPad->SetGrid(1,1);
    gPad->SetTickx(1);
    gPad->SetTicky(1);
    gPad->SetTopMargin(0.15);
    gPad->SetRightMargin(0.15);
    TH2F *h_yield_corr = new TH2F("h_yield_corr", "Cherenkov Light Yield vs Muon Energy", 25, 0, 10, 25, 0, 2000);
    h_yield_corr->SetXTitle("Primary Muon Energy [GeV]");
    h_yield_corr->SetYTitle("Detected Photons");
    h_yield_corr->SetTitleSize(0.032, "XY");
    h_yield_corr->SetLabelSize(0.028, "XY");
    eventTree->Draw("DOMHitCount:PrimaryEnergy_GeV>>h_yield_corr", "DOMHitCount>0", "colz");
    
    // 5. Photon timing (first arrival)
    // PURPOSE: Time of flight for fastest photons (direct path, minimal scattering)
    // USE: Validates light speed in water, crucial for timing-based reconstruction
    c1->cd(5);
    gPad->SetGrid(1,1);
    gPad->SetTickx(1);
    gPad->SetTicky(1);
    gPad->SetTopMargin(0.15);
    TH1F *h_first_time = new TH1F("h_first_time", "First Photon Arrival Time", 50, 0, 50);
    h_first_time->SetXTitle("Time of First Photon [ns]");
    h_first_time->SetYTitle("Number of Events");
    h_first_time->SetTitleSize(0.032, "XY");
    h_first_time->SetLabelSize(0.028, "XY");
    eventTree->Draw("FirstPhotonTime_ns>>h_first_time", "FirstPhotonTime_ns>0", "");
    h_first_time->SetFillColor(kMagenta-3);
    h_first_time->SetLineColor(kMagenta+2);
    h_first_time->SetLineWidth(2);
    
    // 6. Cherenkov wavelength spectrum
    // PURPOSE: Average wavelength of detected Cherenkov light per event
    // USE: Should peak at ~400-450 nm, validates optical physics and PMT response
    c1->cd(6);
    gPad->SetGrid(1,1);
    gPad->SetTickx(1);
    gPad->SetTicky(1);
    gPad->SetTopMargin(0.15);
    TH1F *h_wavelength = new TH1F("h_wavelength", "Average Cherenkov Wavelength per Event", 50, 300, 700);
    h_wavelength->SetXTitle("Average Wavelength [nm]");
    h_wavelength->SetYTitle("Number of Events");
    h_wavelength->SetTitleSize(0.032, "XY");
    h_wavelength->SetLabelSize(0.028, "XY");
    eventTree->Draw("AvgPhotonWavelength_nm>>h_wavelength", "AvgPhotonWavelength_nm>0", "");
    h_wavelength->SetFillColor(kOrange-3);
    h_wavelength->SetLineColor(kOrange+2);
    h_wavelength->SetLineWidth(2);
    
    c1->Update();
    c1->Print("water_tank_event_analysis.png");
    
    // ========================================================
    // Extended Event Analysis with Timing Statistics
    // PURPOSE: Validate timing distribution and scattering effects
    // ========================================================
    
    TCanvas *c1b = new TCanvas("c1b", "Extended Timing Analysis", 1400, 500);
    c1b->Divide(3, 1);
    c1b->SetBorderMode(0);
    
    // 1. Time RMS distribution - measures scattering effects
    c1b->cd(1);
    gPad->SetGrid(1,1);
    gPad->SetTopMargin(0.15);
    TH1F *h_time_rms = new TH1F("h_time_rms", "Photon Arrival Time Spread (RMS)", 50, 0, 20);
    h_time_rms->SetXTitle("Time RMS [ns]");
    h_time_rms->SetYTitle("Number of Events");
    eventTree->Draw("TimeRMS_ns>>h_time_rms", "TimeRMS_ns>0", "");
    h_time_rms->SetFillColor(kCyan-3);
    h_time_rms->SetLineColor(kCyan+2);
    h_time_rms->SetLineWidth(2);
    
    // 2. Time median distribution
    c1b->cd(2);
    gPad->SetGrid(1,1);
    gPad->SetTopMargin(0.15);
    TH1F *h_time_median = new TH1F("h_time_median", "Median Photon Arrival Time", 50, 0, 50);
    h_time_median->SetXTitle("Median Time [ns]");
    h_time_median->SetYTitle("Number of Events");
    eventTree->Draw("TimeMedian_ns>>h_time_median", "TimeMedian_ns>0", "");
    h_time_median->SetFillColor(kTeal-3);
    h_time_median->SetLineColor(kTeal+2);
    h_time_median->SetLineWidth(2);
    
    // 3. Time spread (last - first) distribution
    c1b->cd(3);
    gPad->SetGrid(1,1);
    gPad->SetTopMargin(0.15);
    TH1F *h_time_spread = new TH1F("h_time_spread", "Photon Time Window (Last - First)", 50, 0, 100);
    h_time_spread->SetXTitle("Time Window [ns]");
    h_time_spread->SetYTitle("Number of Events");
    eventTree->Draw("(LastPhotonTime_ns-FirstPhotonTime_ns)>>h_time_spread", 
                    "FirstPhotonTime_ns>0 && LastPhotonTime_ns>0", "");
    h_time_spread->SetFillColor(kPink-3);
    h_time_spread->SetLineColor(kPink+2);
    h_time_spread->SetLineWidth(2);
    
    c1b->Update();
    c1b->Print("water_tank_timing_analysis.png");
    
    // ========================================================
    // DOM Hit Analysis (individual photons)
    // PURPOSE: Analyze individual Cherenkov photon properties and detector response
    // These plots validate optical physics, PMT performance, and geometric reconstruction
    // ========================================================
    
    // Create canvas for DOM hit analysis  
    TCanvas *c2 = new TCanvas("c2", "Water Tank Individual Photon Analysis", 1400, 900);
    c2->Divide(3, 2);
    c2->SetBorderMode(0);
    c2->SetFrameBorderMode(0);
    
    // 1. Individual photon energy spectrum
    // PURPOSE: Energy distribution of detected Cherenkov photons
    // USE: Should match theoretical Cherenkov spectrum (1/λ² dependence, ~2-4 eV range)
    c2->cd(1);
    gPad->SetGrid(1,1);
    gPad->SetTickx(1);
    gPad->SetTicky(1);
    gPad->SetTopMargin(0.15);
    TH1F *h_photon_energy = new TH1F("h_photon_energy", "Individual Cherenkov Photon Energies", 50, 1.5, 4.5);
    h_photon_energy->SetXTitle("Photon Energy [eV]");
    h_photon_energy->SetYTitle("Number of Photons");
    h_photon_energy->SetTitleSize(0.032, "XY");
    h_photon_energy->SetLabelSize(0.028, "XY");
    domhitsTree->Draw("Energy_eV>>h_photon_energy", "", "");
    h_photon_energy->SetFillColor(kOrange-3);
    h_photon_energy->SetLineColor(kOrange+2);
    h_photon_energy->SetLineWidth(2);
    
    // 2. Cherenkov wavelength distribution
    // PURPOSE: Wavelength spectrum of Cherenkov light (fundamental physics validation)
    // USE: Should show 1/λ² distribution, blue-weighted, peak sensitivity ~400-500 nm
    c2->cd(2);
    gPad->SetGrid(1,1);
    gPad->SetTickx(1);
    gPad->SetTicky(1);
    gPad->SetTopMargin(0.15);
    TH1F *h_photon_wavelength = new TH1F("h_photon_wavelength", "Cherenkov Light Wavelength Spectrum", 50, 300, 700);
    h_photon_wavelength->SetXTitle("Wavelength [nm]");
    h_photon_wavelength->SetYTitle("Number of Photons");
    h_photon_wavelength->SetTitleSize(0.032, "XY");
    h_photon_wavelength->SetLabelSize(0.028, "XY");
    domhitsTree->Draw("Wavelength_nm>>h_photon_wavelength", "", "");
    h_photon_wavelength->SetFillColor(kViolet-3);
    h_photon_wavelength->SetLineColor(kViolet+2);
    h_photon_wavelength->SetLineWidth(2);
    
    // 3. Photon arrival time distribution
    // PURPOSE: Time spread of photon arrivals (scattering, path length variations)
    // USE: Shows detector time resolution limits, important for timing reconstruction
    c2->cd(3);
    gPad->SetGrid(1,1);
    gPad->SetTickx(1);
    gPad->SetTicky(1);
    gPad->SetTopMargin(0.15);
    gPad->SetLogy(1);
    TH1F *h_photon_time = new TH1F("h_photon_time", "Photon Time-of-Flight Distribution", 100, 0, 100);
    h_photon_time->SetXTitle("Photon Arrival Time [ns]");
    h_photon_time->SetYTitle("Number of Photons (log scale)");
    h_photon_time->SetTitleSize(0.032, "XY");
    h_photon_time->SetLabelSize(0.028, "XY");
    domhitsTree->Draw("Time_ns>>h_photon_time", "Time_ns>0", "");
    h_photon_time->SetFillColor(kSpring-3);
    h_photon_time->SetLineColor(kSpring+2);
    h_photon_time->SetLineWidth(2);
    
    // 4. DOM hit spatial distribution (X-Y plane)
    // PURPOSE: 2D map of where photons hit the spherical DOM surface (R=16.5cm)
    // USE: Shows muon track projection effects on spherical detector surface
    c2->cd(4);
    gPad->SetGrid(1,1);
    gPad->SetTickx(1);
    gPad->SetTicky(1);
    gPad->SetTopMargin(0.15);
    gPad->SetRightMargin(0.15);
    TH2F *h_xy_hits = new TH2F("h_xy_hits", "DOM Hit Pattern (Top View)", 30, -20, 20, 30, -20, 20);
    h_xy_hits->SetXTitle("X Position [cm]");
    h_xy_hits->SetYTitle("Y Position [cm]");
    h_xy_hits->SetTitleSize(0.032, "XY");
    h_xy_hits->SetLabelSize(0.028, "XY");
    domhitsTree->Draw("PosY_cm:PosX_cm>>h_xy_hits", "", "colz");
    
    // Add circle to show DOM outline
    TEllipse *domCircle = new TEllipse(0, 0, 16.5, 16.5);
    domCircle->SetLineColor(kRed);
    domCircle->SetLineWidth(2);
    domCircle->SetFillStyle(0);  // hollow
    domCircle->Draw("same");
    
    // 5. Cylindrical coordinate view (Z vs R) - Spherical DOM cross-section
    // PURPOSE: Side view of spherical DOM detector (R=16.5cm) hit pattern
    // USE: Shows vertical distribution on spherical detector surface
    c2->cd(5);
    gPad->SetGrid(1,1);
    gPad->SetTickx(1);
    gPad->SetTicky(1);
    gPad->SetTopMargin(0.15);
    gPad->SetRightMargin(0.15);
    TH2F *h_zr_hits = new TH2F("h_zr_hits", "DOM Hit Pattern (Side View)", 30, -20, 20, 30, 0, 20);
    h_zr_hits->SetXTitle("Z Position [cm]");
    h_zr_hits->SetYTitle("Radial Distance R [cm]");
    h_zr_hits->SetTitleSize(0.032, "XY");
    h_zr_hits->SetLabelSize(0.028, "XY");
    domhitsTree->Draw("sqrt(PosX_cm*PosX_cm + PosY_cm*PosY_cm):PosZ_cm>>h_zr_hits", "", "colz");
    
    // Add semicircle to show DOM profile (R=16.5cm sphere)
    TF1 *domProfile = new TF1("domProfile", "sqrt(16.5*16.5 - x*x)", -16.5, 16.5);
    domProfile->SetLineColor(kRed);
    domProfile->SetLineWidth(2);
    domProfile->Draw("same");
    
    // 6. Photon angular distribution at spherical DOM surface
    // PURPOSE: Direction vectors of photons hitting spherical DOM (R=16.5cm)
    // USE: Shows photon propagation directions when hitting detector surface
    c2->cd(6);
    gPad->SetGrid(1,1);
    gPad->SetTickx(1);
    gPad->SetTicky(1);
    gPad->SetTopMargin(0.15);
    gPad->SetRightMargin(0.15);
    TH2F *h_photon_dir = new TH2F("h_photon_dir", "Photon Direction at Spherical DOM", 36, -180, 180, 18, 0, 180);
    h_photon_dir->SetXTitle("Azimuthal Angle #phi [degrees]");
    h_photon_dir->SetYTitle("Polar Angle #theta [degrees]");
    h_photon_dir->SetTitleSize(0.032, "XY");
    h_photon_dir->SetLabelSize(0.028, "XY");
    domhitsTree->Draw("acos(abs(DirZ))*180/3.14159:atan2(DirY,DirX)*180/3.14159>>h_photon_dir", "", "colz");
    
    c2->Update();
    c2->Print("water_tank_photon_analysis.png");
    
    // ========================================================
    // Cherenkov Physics Validation
    // PURPOSE: Validate fundamental Cherenkov radiation physics
    // These plots compare simulation output to theoretical predictions
    // ========================================================
    
    TCanvas *c2b = new TCanvas("c2b", "Cherenkov Physics Validation", 1400, 900);
    c2b->Divide(3, 2);
    c2b->SetBorderMode(0);
    
    // 1. Cherenkov angle validation using photon directions at DOM surface
    // For a spherical DOM, we compute angle between photon direction and 
    // radial vector from DOM center to hit position
    c2b->cd(1);
    gPad->SetGrid(1,1);
    gPad->SetTopMargin(0.15);
    TH1F *h_cherenkov_angle = new TH1F("h_cherenkov_angle", 
        "Photon Incidence Angle at DOM Surface", 90, 0, 90);
    h_cherenkov_angle->SetXTitle("Incidence Angle [degrees]");
    h_cherenkov_angle->SetYTitle("Number of Photons");
    // Angle between photon direction and inward radial direction
    // radial = -pos/|pos|, so angle = acos(-dir . pos/|pos|)
    domhitsTree->Draw("acos(-(DirX*PosX_cm + DirY*PosY_cm + DirZ*PosZ_cm)/sqrt(PosX_cm*PosX_cm + PosY_cm*PosY_cm + PosZ_cm*PosZ_cm))*180/3.14159>>h_cherenkov_angle", "", "");
    h_cherenkov_angle->SetFillColor(kAzure-3);
    h_cherenkov_angle->SetLineColor(kAzure+2);
    h_cherenkov_angle->SetLineWidth(2);
    
    // Draw expected Cherenkov angle line for water (n~1.337 at 400nm)
    double expected_angle = getCherenkovAngle(1.337);
    TLine *cherenkov_line = new TLine(expected_angle, 0, expected_angle, h_cherenkov_angle->GetMaximum()*0.8);
    cherenkov_line->SetLineColor(kRed);
    cherenkov_line->SetLineWidth(2);
    cherenkov_line->SetLineStyle(2);
    cherenkov_line->Draw("same");
    
    TLatex *lat1 = new TLatex(expected_angle+2, h_cherenkov_angle->GetMaximum()*0.7, 
                              Form("#theta_{C} = %.1f#circ (n=1.337)", expected_angle));
    lat1->SetTextColor(kRed);
    lat1->SetTextSize(0.035);
    lat1->Draw();
    
    // 2. Wavelength spectrum compared to theoretical 1/lambda^2
    c2b->cd(2);
    gPad->SetGrid(1,1);
    gPad->SetTopMargin(0.15);
    TH1F *h_wl_theory = new TH1F("h_wl_theory", 
        "Cherenkov Spectrum: Data vs Theory (1/#lambda^{2})", 40, 300, 700);
    h_wl_theory->SetXTitle("Wavelength [nm]");
    h_wl_theory->SetYTitle("Relative Intensity");
    domhitsTree->Draw("Wavelength_nm>>h_wl_theory", "", "");
    h_wl_theory->SetLineColor(kBlue);
    h_wl_theory->SetLineWidth(2);
    h_wl_theory->SetFillStyle(0);
    
    // Overlay theoretical 1/lambda^2 curve (normalized to data)
    TF1 *f_theory = new TF1("f_theory", "[0]/(x*x)", 300, 700);
    // Normalize to match histogram integral
    double data_integral = h_wl_theory->Integral();
    double theory_norm = data_integral * 400 * 400 / 40; // approximate normalization
    f_theory->SetParameter(0, theory_norm);
    f_theory->SetLineColor(kRed);
    f_theory->SetLineWidth(2);
    f_theory->SetLineStyle(2);
    f_theory->Draw("same");
    
    TLegend *leg2 = new TLegend(0.5, 0.7, 0.88, 0.85);
    leg2->AddEntry(h_wl_theory, "Simulated spectrum", "l");
    leg2->AddEntry(f_theory, "Theory: 1/#lambda^{2}", "l");
    leg2->SetBorderSize(0);
    leg2->Draw();
    
    // 3. Detected spectrum weighted by QE
    // QE curve from simulation: peaks around 350-400nm
    c2b->cd(3);
    gPad->SetGrid(1,1);
    gPad->SetTopMargin(0.15);
    TH1F *h_detected_wl = new TH1F("h_detected_wl", 
        "Detected Wavelength (includes QE weighting)", 40, 300, 700);
    h_detected_wl->SetXTitle("Wavelength [nm]");
    h_detected_wl->SetYTitle("Detected Photons");
    domhitsTree->Draw("Wavelength_nm>>h_detected_wl", "", "");
    h_detected_wl->SetFillColor(kGreen-3);
    h_detected_wl->SetLineColor(kGreen+2);
    h_detected_wl->SetLineWidth(2);
    
    // Mark peak sensitivity region
    TLine *qe_lo = new TLine(350, 0, 350, h_detected_wl->GetMaximum()*0.9);
    TLine *qe_hi = new TLine(450, 0, 450, h_detected_wl->GetMaximum()*0.9);
    qe_lo->SetLineColor(kMagenta);
    qe_hi->SetLineColor(kMagenta);
    qe_lo->SetLineStyle(2);
    qe_hi->SetLineStyle(2);
    qe_lo->Draw("same");
    qe_hi->Draw("same");
    
    TLatex *lat2 = new TLatex(360, h_detected_wl->GetMaximum()*0.95, "Peak QE region");
    lat2->SetTextColor(kMagenta);
    lat2->SetTextSize(0.03);
    lat2->Draw();
    
    // 4. Time residuals: observed time vs expected direct path time
    c2b->cd(4);
    gPad->SetGrid(1,1);
    gPad->SetTopMargin(0.15);
    TH1F *h_time_residual = new TH1F("h_time_residual", 
        "Time Residual (observed - expected direct path)", 100, -10, 50);
    h_time_residual->SetXTitle("Time Residual [ns]");
    h_time_residual->SetYTitle("Number of Photons");
    // Expected time = n * distance / c, distance ~ DOM_RADIUS for surface hits
    // This is simplified; full calculation would need track geometry
    double n_water = 1.337;
    double expected_time_offset = n_water * DOM_RADIUS / c_light;
    domhitsTree->Draw(Form("Time_ns - %f>>h_time_residual", expected_time_offset), "Time_ns>0", "");
    h_time_residual->SetFillColor(kOrange-3);
    h_time_residual->SetLineColor(kOrange+2);
    h_time_residual->SetLineWidth(2);
    
    // 5. Photon yield vs estimated track length through tank
    // Track length estimate: for vertical muon through center, ~2*tank_half_height
    c2b->cd(5);
    gPad->SetGrid(1,1);
    gPad->SetTopMargin(0.15);
    gPad->SetRightMargin(0.15);
    TH2F *h_yield_vs_edep = new TH2F("h_yield_vs_edep", 
        "Photon Yield vs Energy Deposition (track length proxy)", 
        25, 0, 0.5, 25, 0, 2000);
    h_yield_vs_edep->SetXTitle("Energy Deposited [GeV]");
    h_yield_vs_edep->SetYTitle("Detected Photons");
    eventTree->Draw("DOMHitCount:Edep_GeV>>h_yield_vs_edep", "DOMHitCount>0 && Edep_GeV>0", "colz");
    
    // 6. Angular acceptance: hit rate vs theta (polar angle on DOM)
    c2b->cd(6);
    gPad->SetGrid(1,1);
    gPad->SetTopMargin(0.15);
    TH1F *h_dom_theta = new TH1F("h_dom_theta", 
        "DOM Hit Distribution vs Polar Angle (from +Z)", 36, 0, 180);
    h_dom_theta->SetXTitle("Polar Angle #theta [degrees]");
    h_dom_theta->SetYTitle("Number of Photons");
    // theta = acos(z/r) where r = DOM_RADIUS
    domhitsTree->Draw(Form("acos(PosZ_cm/%f)*180/3.14159>>h_dom_theta", DOM_RADIUS), "", "");
    h_dom_theta->SetFillColor(kViolet-3);
    h_dom_theta->SetLineColor(kViolet+2);
    h_dom_theta->SetLineWidth(2);
    
    c2b->Update();
    c2b->Print("water_tank_cherenkov_validation.png");
    
    // ========================================================
    // Physics Analysis and Performance Metrics
    // PURPOSE: Fundamental physics validation and detector performance assessment
    // These plots validate theoretical predictions and assess detector design
    // ========================================================
    
    TCanvas *c3 = new TCanvas("c3", "Water Tank Physics Validation", 1400, 700);
    c3->Divide(2, 1);
    c3->SetBorderMode(0);
    c3->SetFrameBorderMode(0);
    
    // 1. Cherenkov light yield correlation
    // PURPOSE: Shows relationship between muon energy and detected photon yield
    // USE: Should be linear for relativistic muons - validates fundamental Cherenkov physics
    c3->cd(1);
    gPad->SetGrid(1,1);
    gPad->SetTickx(1);
    gPad->SetTicky(1);
    gPad->SetTopMargin(0.15);
    gPad->SetRightMargin(0.15);
    
    TGraph *g_yield = new TGraph();
    g_yield->SetName("g_yield");
    g_yield->SetTitle("Cherenkov Light Yield vs Muon Energy");
    
    // Extract data for yield correlation
    int nEvents = eventTree->GetEntries();
    double energy, photonYield;
    eventTree->SetBranchAddress("PrimaryEnergy_GeV", &energy);
    eventTree->SetBranchAddress("PhotonYield_per_GeV", &photonYield);
    
    int pointIndex = 0;
    for (int i = 0; i < nEvents; i++) {
        eventTree->GetEntry(i);
        if (energy > 0 && photonYield > 0) {
            g_yield->SetPoint(pointIndex, energy, photonYield);
            pointIndex++;
        }
    }
    
    g_yield->Draw("AP");
    g_yield->SetMarkerStyle(20);
    g_yield->SetMarkerSize(1.2);
    g_yield->SetMarkerColor(kBlue);
    g_yield->SetLineColor(kBlue);
    g_yield->SetLineWidth(2);
    g_yield->GetXaxis()->SetTitle("Primary Muon Energy [GeV]");
    g_yield->GetYaxis()->SetTitle("Cherenkov Photons per GeV");
    g_yield->GetXaxis()->SetTitleSize(0.04);
    g_yield->GetYaxis()->SetTitleSize(0.04);
    g_yield->GetXaxis()->SetLabelSize(0.035);
    g_yield->GetYaxis()->SetLabelSize(0.035);
    
    // 2. Detection efficiency analysis
    // PURPOSE: Shows detector efficiency vs muon energy (trigger threshold effects)
    // USE: High efficiency indicates good detector coverage and sensitivity
    c3->cd(2);
    gPad->SetGrid(1,1);
    gPad->SetTickx(1);
    gPad->SetTicky(1);
    gPad->SetTopMargin(0.15);
    
    TH1F *h_efficiency = new TH1F("h_efficiency", "Water Tank Detection Efficiency", 20, 0, 10);
    TH1F *h_total = new TH1F("h_total", "", 20, 0, 10);
    
    // Calculate efficiency = events with hits / total events
    eventTree->Draw("PrimaryEnergy_GeV>>h_total", "", "goff");
    eventTree->Draw("PrimaryEnergy_GeV>>h_efficiency", "DOMHitCount>10", "goff");  // Require >10 hits for good reconstruction
    
    h_efficiency->Divide(h_total);
    h_efficiency->SetXTitle("Primary Muon Energy [GeV]");
    h_efficiency->SetYTitle("Detection Efficiency (>10 hits)");
    h_efficiency->SetTitleSize(0.04, "XY");
    h_efficiency->SetLabelSize(0.035, "XY");
    h_efficiency->SetMaximum(1.1);
    h_efficiency->SetMinimum(0.0);
    h_efficiency->SetFillColor(kGreen-3);
    h_efficiency->SetLineColor(kGreen+2);
    h_efficiency->SetLineWidth(2);
    h_efficiency->Draw();
    
    c3->Update();
    c3->Print("water_tank_physics_analysis.png");
    
    // ========================================================
    // Scintillator Array Analysis
    // PURPOSE: Analyze scintillator bar trigger performance and hit patterns
    // These plots characterize the cosmic ray muon trigger system
    // ========================================================
    
    // Check if scintillator data is available in the event tree
    bool hasScintData = (eventTree->GetBranch("ScintHitCount") != nullptr);
    
    if (hasScintData) {
        TCanvas *c4 = new TCanvas("c4", "Scintillator Array Analysis", 1400, 900);
        c4->Divide(3, 2);
        c4->SetBorderMode(0);
        
        // 1. Scintillator hit multiplicity per event
        c4->cd(1);
        gPad->SetGrid(1,1);
        gPad->SetTopMargin(0.15);
        TH1F *h_scint_hits = new TH1F("h_scint_hits", "Scintillator Hit Multiplicity", 30, 0, 30);
        h_scint_hits->SetXTitle("Number of Scintillator Hits per Event");
        h_scint_hits->SetYTitle("Number of Events");
        eventTree->Draw("ScintHitCount>>h_scint_hits", "", "");
        h_scint_hits->SetFillColor(kOrange-3);
        h_scint_hits->SetLineColor(kOrange+2);
        h_scint_hits->SetLineWidth(2);
        
        // 2. Layer 0 vs Layer 1 hit correlation
        c4->cd(2);
        gPad->SetGrid(1,1);
        gPad->SetTopMargin(0.15);
        gPad->SetRightMargin(0.15);
        TH2F *h_layer_corr = new TH2F("h_layer_corr", "Layer Hit Correlation (Coincidence Check)", 
                                       15, 0, 15, 15, 0, 15);
        h_layer_corr->SetXTitle("Layer 0 Hits (X-oriented bars)");
        h_layer_corr->SetYTitle("Layer 1 Hits (Y-oriented bars)");
        eventTree->Draw("ScintL1HitCount:ScintL0HitCount>>h_layer_corr", "", "colz");
        
        // 3. Scintillator trigger efficiency
        c4->cd(3);
        gPad->SetGrid(1,1);
        gPad->SetTopMargin(0.15);
        TH1F *h_scint_eff = new TH1F("h_scint_eff", "Scintillator Trigger Efficiency", 20, 0, 10);
        TH1F *h_scint_total = new TH1F("h_scint_total", "", 20, 0, 10);
        eventTree->Draw("PrimaryEnergy_GeV>>h_scint_total", "", "goff");
        eventTree->Draw("PrimaryEnergy_GeV>>h_scint_eff", "ScintCoincidence==1", "goff");
        h_scint_eff->Divide(h_scint_total);
        h_scint_eff->SetXTitle("Primary Muon Energy [GeV]");
        h_scint_eff->SetYTitle("Coincidence Trigger Efficiency");
        h_scint_eff->SetMaximum(1.1);
        h_scint_eff->SetMinimum(0.0);
        h_scint_eff->SetFillColor(kCyan-3);
        h_scint_eff->SetLineColor(kCyan+2);
        h_scint_eff->SetLineWidth(2);
        h_scint_eff->Draw();
        
        // 4. First hit bar distribution for Layer 0
        c4->cd(4);
        gPad->SetGrid(1,1);
        gPad->SetTopMargin(0.15);
        TH1F *h_l0_bar = new TH1F("h_l0_bar", "Layer 0 First Hit Bar Distribution", 12, -0.5, 11.5);
        h_l0_bar->SetXTitle("Bar Index (Layer 0, X-oriented)");
        h_l0_bar->SetYTitle("Number of Events");
        eventTree->Draw("ScintL0FirstBar>>h_l0_bar", "ScintL0FirstBar>=0", "");
        h_l0_bar->SetFillColor(kGreen-3);
        h_l0_bar->SetLineColor(kGreen+2);
        h_l0_bar->SetLineWidth(2);
        
        // 5. First hit bar distribution for Layer 1
        c4->cd(5);
        gPad->SetGrid(1,1);
        gPad->SetTopMargin(0.15);
        TH1F *h_l1_bar = new TH1F("h_l1_bar", "Layer 1 First Hit Bar Distribution", 12, -0.5, 11.5);
        h_l1_bar->SetXTitle("Bar Index (Layer 1, Y-oriented)");
        h_l1_bar->SetYTitle("Number of Events");
        eventTree->Draw("ScintL1FirstBar>>h_l1_bar", "ScintL1FirstBar>=0", "");
        h_l1_bar->SetFillColor(kMagenta-3);
        h_l1_bar->SetLineColor(kMagenta+2);
        h_l1_bar->SetLineWidth(2);
        
        // 6. Total scintillator energy deposition
        c4->cd(6);
        gPad->SetGrid(1,1);
        gPad->SetTopMargin(0.15);
        TH1F *h_scint_edep = new TH1F("h_scint_edep", "Total Scintillator Energy Deposit", 50, 0, 50);
        h_scint_edep->SetXTitle("Energy Deposited [MeV]");
        h_scint_edep->SetYTitle("Number of Events");
        eventTree->Draw("ScintTotalEdep_MeV>>h_scint_edep", "ScintTotalEdep_MeV>0", "");
        h_scint_edep->SetFillColor(kRed-3);
        h_scint_edep->SetLineColor(kRed+2);
        h_scint_edep->SetLineWidth(2);
        
        c4->Update();
        c4->Print("water_tank_scintillator_analysis.png");
        
        // ========================================================
        // Time-of-Flight Analysis
        // PURPOSE: Measure and validate TOF from scintillator trigger to DOM detection
        // Critical for understanding detector timing and muon velocity
        // ========================================================
        
        TCanvas *c5 = new TCanvas("c5", "Time-of-Flight Analysis", 1400, 900);
        c5->Divide(3, 2);
        c5->SetBorderMode(0);
        
        // 1. TOF distribution (scintillator to DOM)
        c5->cd(1);
        gPad->SetGrid(1,1);
        gPad->SetTopMargin(0.15);
        TH1F *h_tof = new TH1F("h_tof", "Time-of-Flight: Scintillator to DOM", 100, -10, 50);
        h_tof->SetXTitle("TOF [ns] (FirstPhotonTime - ScintFirstTime)");
        h_tof->SetYTitle("Number of Events");
        eventTree->Draw("TOF_ns>>h_tof", "TOF_ns>-100 && ScintCoincidence==1", "");
        h_tof->SetFillColor(kBlue-3);
        h_tof->SetLineColor(kBlue+2);
        h_tof->SetLineWidth(2);
        
        // Add expected TOF line for vertical muon
        // Distance from scint layer to DOM center ~60cm, light speed in water ~22.4 cm/ns
        double expected_tof = (SCINT_LAYER0_Z + TANK_HALF_HEIGHT) / (c_light / 1.337);
        TLine *tof_line = new TLine(expected_tof, 0, expected_tof, h_tof->GetMaximum()*0.8);
        tof_line->SetLineColor(kRed);
        tof_line->SetLineWidth(2);
        tof_line->SetLineStyle(2);
        tof_line->Draw("same");
        
        TLatex *lat_tof = new TLatex(expected_tof+2, h_tof->GetMaximum()*0.7, 
                                     Form("Expected ~%.1f ns", expected_tof));
        lat_tof->SetTextColor(kRed);
        lat_tof->SetTextSize(0.03);
        lat_tof->Draw();
        
        // 2. TOF from Layer 0 specifically
        c5->cd(2);
        gPad->SetGrid(1,1);
        gPad->SetTopMargin(0.15);
        TH1F *h_tof_l0 = new TH1F("h_tof_l0", "TOF from Layer 0 (X-oriented bars)", 100, -10, 50);
        h_tof_l0->SetXTitle("TOF from Layer 0 [ns]");
        h_tof_l0->SetYTitle("Number of Events");
        eventTree->Draw("TOF_L0_ns>>h_tof_l0", "TOF_L0_ns>-100 && ScintL0HitCount>0", "");
        h_tof_l0->SetFillColor(kGreen-3);
        h_tof_l0->SetLineColor(kGreen+2);
        h_tof_l0->SetLineWidth(2);
        
        // 3. TOF from Layer 1 specifically
        c5->cd(3);
        gPad->SetGrid(1,1);
        gPad->SetTopMargin(0.15);
        TH1F *h_tof_l1 = new TH1F("h_tof_l1", "TOF from Layer 1 (Y-oriented bars)", 100, -10, 50);
        h_tof_l1->SetXTitle("TOF from Layer 1 [ns]");
        h_tof_l1->SetYTitle("Number of Events");
        eventTree->Draw("TOF_L1_ns>>h_tof_l1", "TOF_L1_ns>-100 && ScintL1HitCount>0", "");
        h_tof_l1->SetFillColor(kMagenta-3);
        h_tof_l1->SetLineColor(kMagenta+2);
        h_tof_l1->SetLineWidth(2);
        
        // 4. TOF vs DOM hit count (correlation)
        c5->cd(4);
        gPad->SetGrid(1,1);
        gPad->SetTopMargin(0.15);
        gPad->SetRightMargin(0.15);
        TH2F *h_tof_vs_hits = new TH2F("h_tof_vs_hits", "TOF vs DOM Hit Multiplicity", 
                                        25, -5, 45, 25, 0, 2000);
        h_tof_vs_hits->SetXTitle("TOF [ns]");
        h_tof_vs_hits->SetYTitle("DOM Hit Count");
        eventTree->Draw("DOMHitCount:TOF_ns>>h_tof_vs_hits", "TOF_ns>-100 && ScintCoincidence==1", "colz");
        
        // 5. Scintillator timing: Layer 0 vs Layer 1 first hit times
        c5->cd(5);
        gPad->SetGrid(1,1);
        gPad->SetTopMargin(0.15);
        gPad->SetRightMargin(0.15);
        TH2F *h_scint_timing = new TH2F("h_scint_timing", "Scintillator Layer Timing Correlation", 
                                         50, 0, 20, 50, 0, 20);
        h_scint_timing->SetXTitle("Layer 0 First Hit Time [ns]");
        h_scint_timing->SetYTitle("Layer 1 First Hit Time [ns]");
        eventTree->Draw("ScintL1FirstTime_ns:ScintL0FirstTime_ns>>h_scint_timing", 
                        "ScintL0FirstTime_ns>0 && ScintL1FirstTime_ns>0", "colz");
        
        // Add diagonal line for simultaneous hits
        TF1 *diag = new TF1("diag", "x", 0, 20);
        diag->SetLineColor(kRed);
        diag->SetLineWidth(2);
        diag->SetLineStyle(2);
        diag->Draw("same");
        
        // 6. Layer timing difference (for muon direction)
        c5->cd(6);
        gPad->SetGrid(1,1);
        gPad->SetTopMargin(0.15);
        TH1F *h_layer_dt = new TH1F("h_layer_dt", "Time Difference Between Scintillator Layers", 100, -5, 5);
        h_layer_dt->SetXTitle("#DeltaT (Layer1 - Layer0) [ns]");
        h_layer_dt->SetYTitle("Number of Events");
        eventTree->Draw("(ScintL1FirstTime_ns-ScintL0FirstTime_ns)>>h_layer_dt", 
                        "ScintL0FirstTime_ns>0 && ScintL1FirstTime_ns>0", "");
        h_layer_dt->SetFillColor(kOrange-3);
        h_layer_dt->SetLineColor(kOrange+2);
        h_layer_dt->SetLineWidth(2);
        
        // Add line at zero (simultaneous hits = vertical muon)
        TLine *zero_line = new TLine(0, 0, 0, h_layer_dt->GetMaximum()*0.9);
        zero_line->SetLineColor(kRed);
        zero_line->SetLineWidth(2);
        zero_line->SetLineStyle(2);
        zero_line->Draw("same");
        
        c5->Update();
        c5->Print("water_tank_tof_analysis.png");
        
        // ========================================================
        // Combined Performance Analysis
        // PURPOSE: Overall system performance with scintillator trigger
        // ========================================================
        
        TCanvas *c6 = new TCanvas("c6", "Combined System Performance", 1400, 500);
        c6->Divide(3, 1);
        c6->SetBorderMode(0);
        
        // 1. DOM detection efficiency with scintillator coincidence requirement
        c6->cd(1);
        gPad->SetGrid(1,1);
        gPad->SetTopMargin(0.15);
        TH1F *h_dom_eff_coinc = new TH1F("h_dom_eff_coinc", "DOM Efficiency (with Scint Coincidence)", 20, 0, 10);
        TH1F *h_coinc_total = new TH1F("h_coinc_total", "", 20, 0, 10);
        eventTree->Draw("PrimaryEnergy_GeV>>h_coinc_total", "ScintCoincidence==1", "goff");
        eventTree->Draw("PrimaryEnergy_GeV>>h_dom_eff_coinc", "ScintCoincidence==1 && DOMHitCount>10", "goff");
        h_dom_eff_coinc->Divide(h_coinc_total);
        h_dom_eff_coinc->SetXTitle("Primary Muon Energy [GeV]");
        h_dom_eff_coinc->SetYTitle("DOM Detection Efficiency");
        h_dom_eff_coinc->SetMaximum(1.1);
        h_dom_eff_coinc->SetMinimum(0.0);
        h_dom_eff_coinc->SetFillColor(kAzure-3);
        h_dom_eff_coinc->SetLineColor(kAzure+2);
        h_dom_eff_coinc->SetLineWidth(2);
        h_dom_eff_coinc->Draw();
        
        // 2. TOF resolution (RMS) vs muon energy
        c6->cd(2);
        gPad->SetGrid(1,1);
        gPad->SetTopMargin(0.15);
        
        // Create profile histogram for TOF vs energy
        TProfile *p_tof_vs_energy = new TProfile("p_tof_vs_energy", "TOF vs Muon Energy", 10, 0, 10, -10, 60);
        p_tof_vs_energy->SetXTitle("Primary Muon Energy [GeV]");
        p_tof_vs_energy->SetYTitle("Mean TOF [ns]");
        eventTree->Draw("TOF_ns:PrimaryEnergy_GeV>>p_tof_vs_energy", "TOF_ns>-100 && ScintCoincidence==1", "");
        p_tof_vs_energy->SetMarkerStyle(20);
        p_tof_vs_energy->SetMarkerColor(kBlue);
        p_tof_vs_energy->SetLineColor(kBlue);
        p_tof_vs_energy->SetLineWidth(2);
        
        // 3. Combined trigger and DOM efficiency
        c6->cd(3);
        gPad->SetGrid(1,1);
        gPad->SetTopMargin(0.15);
        TH1F *h_combined_eff = new TH1F("h_combined_eff", "Combined System Efficiency", 20, 0, 10);
        TH1F *h_all_events = new TH1F("h_all_events", "", 20, 0, 10);
        eventTree->Draw("PrimaryEnergy_GeV>>h_all_events", "", "goff");
        eventTree->Draw("PrimaryEnergy_GeV>>h_combined_eff", "ScintCoincidence==1 && DOMHitCount>10", "goff");
        h_combined_eff->Divide(h_all_events);
        h_combined_eff->SetXTitle("Primary Muon Energy [GeV]");
        h_combined_eff->SetYTitle("Overall System Efficiency");
        h_combined_eff->SetMaximum(1.1);
        h_combined_eff->SetMinimum(0.0);
        h_combined_eff->SetFillColor(kTeal-3);
        h_combined_eff->SetLineColor(kTeal+2);
        h_combined_eff->SetLineWidth(2);
        h_combined_eff->Draw();
        
        c6->Update();
        c6->Print("water_tank_combined_performance.png");
        
    } else {
        std::cout << "\nNote: Scintillator data not found in event tree." << std::endl;
        std::cout << "Skipping scintillator and TOF analysis plots." << std::endl;
    }
    
    // ========================================================
    // Scintillator Hit-Level Analysis (if available)
    // ========================================================
    if (scinthitsTree && scinthitsTree->GetEntries() > 0) {
        TCanvas *c7 = new TCanvas("c7", "Scintillator Hit Details", 1400, 500);
        c7->Divide(3, 1);
        c7->SetBorderMode(0);
        
        // 1. Hit position X-Y distribution
        c7->cd(1);
        gPad->SetGrid(1,1);
        gPad->SetTopMargin(0.15);
        gPad->SetRightMargin(0.15);
        TH2F *h_scint_xy = new TH2F("h_scint_xy", "Scintillator Hit Positions (X-Y)", 
                                     50, -120, 120, 50, -120, 120);
        h_scint_xy->SetXTitle("X Position [cm]");
        h_scint_xy->SetYTitle("Y Position [cm]");
        scinthitsTree->Draw("PosY_cm:PosX_cm>>h_scint_xy", "", "colz");
        
        // Draw tank outline
        TEllipse *tankCircle = new TEllipse(0, 0, 90.17, 90.17);
        tankCircle->SetLineColor(kRed);
        tankCircle->SetLineWidth(2);
        tankCircle->SetFillStyle(0);
        tankCircle->Draw("same");
        
        // 2. Energy deposit per hit
        c7->cd(2);
        gPad->SetGrid(1,1);
        gPad->SetTopMargin(0.15);
        TH1F *h_hit_edep = new TH1F("h_hit_edep", "Energy Deposit per Scintillator Hit", 100, 0, 10);
        h_hit_edep->SetXTitle("Energy Deposit [MeV]");
        h_hit_edep->SetYTitle("Number of Hits");
        scinthitsTree->Draw("Edep_MeV>>h_hit_edep", "", "");
        h_hit_edep->SetFillColor(kViolet-3);
        h_hit_edep->SetLineColor(kViolet+2);
        h_hit_edep->SetLineWidth(2);
        
        // 3. Hit time distribution
        c7->cd(3);
        gPad->SetGrid(1,1);
        gPad->SetTopMargin(0.15);
        TH1F *h_scint_time = new TH1F("h_scint_time", "Scintillator Hit Time Distribution", 100, 0, 30);
        h_scint_time->SetXTitle("Hit Time [ns]");
        h_scint_time->SetYTitle("Number of Hits");
        scinthitsTree->Draw("Time_ns>>h_scint_time", "", "");
        h_scint_time->SetFillColor(kSpring-3);
        h_scint_time->SetLineColor(kSpring+2);
        h_scint_time->SetLineWidth(2);
        
        c7->Update();
        c7->Print("water_tank_scinthit_details.png");
    }
    
    // ========================================================
    // Analysis Summary and Statistics
    // ========================================================
    std::cout << "\n=======================================" << std::endl;
    std::cout << "    WATER TANK ANALYSIS SUMMARY" << std::endl;
    std::cout << "    IceCube DOM Cherenkov Calibration" << std::endl;
    std::cout << "=======================================" << std::endl;
    std::cout << "Total events analyzed: " << eventTree->GetEntries() << std::endl;
    std::cout << "Total photon hits: " << domhitsTree->GetEntries() << std::endl;
    
    if (eventTree->GetEntries() > 0) {
        double avgHitsPerEvent = (double)domhitsTree->GetEntries() / eventTree->GetEntries();
        std::cout << "Average photons per event: " << avgHitsPerEvent << std::endl;
        
        // Calculate some basic statistics
        eventTree->Draw("PrimaryEnergy_GeV", "", "goff");
        TH1F *htemp = (TH1F*)gDirectory->Get("htemp");
        if (htemp) {
            std::cout << "Average muon energy: " << htemp->GetMean() << " +/- " << htemp->GetRMS() << " GeV" << std::endl;
        }
        
        eventTree->Draw("DOMHitCount", "DOMHitCount>0", "goff");
        htemp = (TH1F*)gDirectory->Get("htemp");
        if (htemp) {
            std::cout << "Average hit multiplicity: " << htemp->GetMean() << " +/- " << htemp->GetRMS() << " photons" << std::endl;
        }
        
        eventTree->Draw("TimeRMS_ns", "TimeRMS_ns>0", "goff");
        htemp = (TH1F*)gDirectory->Get("htemp");
        if (htemp) {
            std::cout << "Average time spread (RMS): " << htemp->GetMean() << " +/- " << htemp->GetRMS() << " ns" << std::endl;
        }
        
        eventTree->Draw("AvgPhotonWavelength_nm", "AvgPhotonWavelength_nm>0", "goff");
        htemp = (TH1F*)gDirectory->Get("htemp");
        if (htemp) {
            std::cout << "Average detected wavelength: " << htemp->GetMean() << " +/- " << htemp->GetRMS() << " nm" << std::endl;
        }
    }
    
    // Physics validation summary
    std::cout << "\n--- Physics Validation ---" << std::endl;
    std::cout << "Expected Cherenkov angle (n=1.337): " << getCherenkovAngle(1.337) << " degrees" << std::endl;
    std::cout << "Speed of light in water: " << c_light/1.337 << " cm/ns" << std::endl;
    std::cout << "DOM radius: " << DOM_RADIUS << " cm" << std::endl;
    
    // Scintillator and TOF summary
    if (hasScintData) {
        std::cout << "\n--- Scintillator Array Statistics ---" << std::endl;
        eventTree->Draw("ScintHitCount", "ScintHitCount>0", "goff");
        TH1F *htemp2 = (TH1F*)gDirectory->Get("htemp");
        if (htemp2) {
            std::cout << "Average scintillator hits per event: " << htemp2->GetMean() << " +/- " << htemp2->GetRMS() << std::endl;
        }
        
        eventTree->Draw("ScintCoincidence", "", "goff");
        htemp2 = (TH1F*)gDirectory->Get("htemp");
        if (htemp2) {
            double coincRate = htemp2->GetMean() * 100;
            std::cout << "Two-layer coincidence rate: " << coincRate << "%" << std::endl;
        }
        
        eventTree->Draw("TOF_ns", "TOF_ns>-100 && ScintCoincidence==1", "goff");
        htemp2 = (TH1F*)gDirectory->Get("htemp");
        if (htemp2 && htemp2->GetEntries() > 0) {
            std::cout << "Average TOF (coincidence events): " << htemp2->GetMean() << " +/- " << htemp2->GetRMS() << " ns" << std::endl;
        }
        
        if (scinthitsTree) {
            std::cout << "Total scintillator hits: " << scinthitsTree->GetEntries() << std::endl;
        }
    }
    
    std::cout << "\nGenerated analysis plots:" << std::endl;
    std::cout << "- water_tank_event_analysis.png       (6 event-level plots)" << std::endl;
    std::cout << "- water_tank_timing_analysis.png      (3 timing statistics plots)" << std::endl;
    std::cout << "- water_tank_photon_analysis.png      (6 photon-level plots)" << std::endl;
    std::cout << "- water_tank_cherenkov_validation.png (6 physics validation plots)" << std::endl;
    std::cout << "- water_tank_physics_analysis.png     (2 performance plots)" << std::endl;
    if (hasScintData) {
        std::cout << "- water_tank_scintillator_analysis.png (6 scintillator plots)" << std::endl;
        std::cout << "- water_tank_tof_analysis.png          (6 time-of-flight plots)" << std::endl;
        std::cout << "- water_tank_combined_performance.png  (3 system performance plots)" << std::endl;
        if (scinthitsTree && scinthitsTree->GetEntries() > 0) {
            std::cout << "- water_tank_scinthit_details.png      (3 hit-level plots)" << std::endl;
        }
    }
    std::cout << "=======================================" << std::endl;
    
    // Close file
    file->Close();
}

// Convenience function to run analysis on latest output file
void quick_analysis() {
    // Look for the most recent output file
    analyze_watertank("output_default.root");
}
// ========================================================
// Water Tank Analysis ROOT Macro
// ========================================================
// Comprehensive analysis and plotting of water tank simulation data
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
#include <iostream>

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
    // Analysis Summary and Statistics
    // ========================================================
    std::cout << "\n=======================================" << std::endl;
    std::cout << "    WATER TANK ANALYSIS SUMMARY" << std::endl;
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
            std::cout << "Average muon energy: " << htemp->GetMean() << " ± " << htemp->GetRMS() << " GeV" << std::endl;
        }
        
        eventTree->Draw("DOMHitCount", "DOMHitCount>0", "goff");
        htemp = (TH1F*)gDirectory->Get("htemp");
        if (htemp) {
            std::cout << "Average hit multiplicity: " << htemp->GetMean() << " ± " << htemp->GetRMS() << " photons" << std::endl;
        }
    }
    
    std::cout << "\nGenerated analysis plots:" << std::endl;
    std::cout << "- water_tank_event_analysis.png (6 event-level plots)" << std::endl;
    std::cout << "- water_tank_photon_analysis.png (6 photon-level plots)" << std::endl;
    std::cout << "- water_tank_physics_analysis.png (2 physics validation plots)" << std::endl;
    std::cout << "=======================================" << std::endl;
    
    // Close file
    file->Close();
}

// Convenience function to run analysis on latest output file
void quick_analysis() {
    // Look for the most recent output file
    analyze_watertank("output_default.root");
}
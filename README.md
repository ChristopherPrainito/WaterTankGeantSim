# Water Tank Geant4 Simulation

A comprehensive Geant4-based simulation of a water-Cherenkov calibration system designed for the IceCube Upgrade project. This simulation models cosmic ray interactions in a cylindrical water tank with Digital Optical Module (DOM) photodetectors, aimed at understanding muon detection and calibration for neutrino astronomy applications.

## Project Overview

This simulation recreates the geometry and physics of a water-Cherenkov detector tank used for IceCube calibration studies. The system consists of:

- **Cylindrical polypropylene tank** (71" diameter × 36" height) filled with ultrapure water
- **Glass DOM photomultiplier sphere** for optical photon detection  
- **Cosmic ray shower simulation** using the CRY (Cosmic Ray Yield) library
- **Comprehensive optical physics** including Cherenkov radiation, refraction, and absorption
- **ROOT-based data output** for analysis of particle interactions and photon yields

This work serves as a foundation for developing a muon scintillator trigger array system for the IceCube Upgrade and forms part of an undergraduate thesis project focused on calibration systems for large-scale neutrino detectors.

## Physics Features

### Particle Physics
- **Complete electromagnetic and hadronic physics** via Geant4's QBBC physics list
- **Cosmic ray simulation** using CRY library for realistic muon flux
- **Optical physics** with detailed Cherenkov radiation modeling
- **Material property modeling** for water, glass, and polypropylene

### Detector Response
- **Photomultiplier tube simulation** with realistic quantum efficiency
- **Optical surface modeling** between water and glass interfaces  
- **Time-resolved photon detection** for pulse shape analysis
- **Energy deposition tracking** in both water and DOM volumes

### Realistic Geometry
- **Scale-accurate tank dimensions** matching IceCube calibration hardware
- **Wavelength-dependent optical properties** (300-620 nm range)
- **DOM positioning** optimized for maximum photon collection

## Prerequisites

### Required Software
- **Geant4** (version 10.7 or later) with Qt/OpenGL visualization
- **ROOT** (version 6.0 or later) for data analysis and plotting
- **CMake** (version 3.6 or later)
- **CRY** (Cosmic Ray Yield) library v1.7 (included in repository)
- **C++17 compatible compiler** (GCC 7+, Clang 5+)

### System Requirements
- Linux/macOS/Windows with WSL2
- Minimum 4 GB RAM (8 GB recommended for large event samples)
- OpenGL-capable graphics card for visualization

## Installation and Setup

### 1. Clone Repository
```bash
git clone https://github.com/ChristopherPrainito/WaterTankGeantSim.git
cd WaterTankGeantSim
```

### 2. Verify Required Software
```bash
# Check Geant4 environment
echo $G4INSTALL
geant4-config --version

# Verify ROOT installation
root-config --version
which root
```

### 3. Build CRY Library (if needed)
```bash
cd cry_v1.7
make
cd ..
```

### 4. Build Simulation
```bash
mkdir build
cd build
cmake ..
make -j4
```

## Usage

### Interactive Mode (Visualization)
Launch the simulation with Qt/OpenGL visualization for geometry inspection and event display:

```bash
cd build
./exampleWaterTank
```

In the interactive session:
```bash
# Initialize geometry and physics
/run/initialize

# Configure visualization  
/vis/open OGL 600x600-0+0
/vis/drawVolume
/vis/viewer/set/viewpointVector -1 0 0
/vis/scene/add/trajectories
/vis/scene/endOfEventAction accumulate

# Run single muon events
/run/beamOn 10
```

### Batch Mode Simulations

#### Single Muon Test
```bash
./exampleWaterTank test.mac
```

#### Cosmic Ray Simulation
```bash
./exampleWaterTank test_cry.mac
```

### Macro Commands

#### Primary Generator Control
```bash
# Switch between single muon and CRY modes
/watertank/generator/useCRY true/false

# Single muon configuration  
/watertank/generator/muon/energy 4 GeV
/watertank/generator/muon/direction 0 0 -1
/watertank/generator/muon/position 0 0 50 cm
```

#### Physics Settings
```bash
# Optical physics parameters
/process/optical/cerenkov/setMaxPhotons 300
/process/optical/cerenkov/setStackPhotons true

# Verbosity control
/run/verbose 1
/event/verbose 1
/tracking/verbose 1
```

## Output Data Format

The simulation generates ROOT files (`output_default.root`) with comprehensive event and hit-level data stored in two main trees:

### Event Tree (`event`)
Contains 15 branches with event-level physics data:

- `EventID`: Unique event identifier
- `PrimaryEnergy_GeV`: Initial particle energy (GeV)
- `PrimaryPDG`: Particle type (PDG code)
- `PrimaryPosX/Y/Z_cm`: Initial particle position (cm)
- `PrimaryDirX/Y/Z`: Initial momentum direction (unit vector)
- `Edep_GeV`: Total energy deposited in water (GeV)
- `DOMHitCount`: Number of photons detected by DOM
- `PhotonYield_per_GeV`: Light yield efficiency (photons/GeV)
- `FirstPhotonTime_ns`: Time of first photon detection (ns)
- `LastPhotonTime_ns`: Time of last photon detection (ns)
- `AvgPhotonWavelength_nm`: Average detected photon wavelength (nm)

### DOM Hits Tree (`domhits`)
Contains 12 branches with individual photon hit data:

- `EventID`: Associated event identifier
- `Time_ns`: Photon arrival time (ns)
- `Energy_eV`: Photon energy (eV)
- `Wavelength_nm`: Photon wavelength (nm)
- `PosX/Y/Z_cm`: Hit position on DOM surface (cm)
- `DirX/Y/Z`: Photon direction at detection (unit vector)
- `TrackID`: Geant4 track identifier
- `ParentID`: Parent track identifier

## Analysis Tools

### ROOT Analysis Macro
The simulation includes a comprehensive ROOT analysis script (`analyze_watertank.C`) that automatically generates detailed physics plots and statistics.

#### Running the Analysis
```bash
# From the build directory
root -l -b -q "../analyze_watertank.C(\"output_default.root\")"

# Or interactively in ROOT
root
.x analyze_watertank.C
```

#### Generated Output
The analysis creates three detailed plot sets:

1. **Event Analysis** (`water_tank_event_analysis.png`) - 6-panel event-level physics
2. **Photon Analysis** (`water_tank_photon_analysis.png`) - 6-panel detector response  
3. **Physics Analysis** (`water_tank_physics_analysis.png`) - 2-panel efficiency/yield plots

### Plot Descriptions

#### Event Analysis Plots
1. **Primary Particle Energy** - Distribution of incident muon energies showing beam characteristics
2. **Energy Deposition in Water** - Energy lost by muons through ionization and electromagnetic processes
3. **DOM Hit Multiplicity** - Number of Cherenkov photons detected per event, indicating light collection efficiency
4. **Photon Yield vs Primary Energy** - 2D correlation showing relationship between muon energy and light production
5. **First Photon Arrival Time** - Timing distribution of earliest detected photons, crucial for trigger studies
6. **Average Photon Wavelength** - Spectral characteristics of detected Cherenkov light (should peak ~400-450nm)

#### Photon Analysis Plots
1. **Photon Energy Spectrum** - Energy distribution of individual detected photons (1.5-4.5 eV range)
2. **Photon Wavelength Spectrum** - Detailed wavelength distribution showing Cherenkov 1/λ² spectrum
3. **Photon Arrival Times** - Temporal spread of photon hits (log scale), reveals scattering and detector response
4. **DOM Hit Positions (X-Y View)** - Spatial distribution of photon hits on DOM hemisphere 
5. **DOM Hit Positions (Z-R View)** - Cylindrical view showing hit pattern vs depth and radius
6. **Photon Direction Distribution** - Angular distribution of detected photons in azimuth/polar coordinates

#### Physics Analysis Plots  
1. **Cherenkov Light Yield** - Light production efficiency vs muon energy, fundamental physics validation
2. **Detection Efficiency** - Fraction of events producing detectable light, critical for trigger design

### Analysis Statistics
The macro automatically calculates and displays:
- Mean primary energy and energy deposition
- Average photon detection rates  
- Overall detection efficiency
- Light yield in photons per GeV
- Event processing statistics

## Complete Analysis Workflow

### 1. Run Simulation
```bash
cd build
./exampleWaterTank test_cry.mac        # Cosmic ray simulation
# or
./exampleWaterTank test.mac            # Single muon test
```

### 2. Analyze Results
```bash
# Generate comprehensive analysis plots
root -l -b -q "../analyze_watertank.C(\"output_default.root\")"
```

### 3. View Output
The analysis generates three PNG files with detailed physics plots:
- `water_tank_event_analysis.png`: Event-level distributions and correlations
- `water_tank_photon_analysis.png`: Individual photon characteristics and detector response
- `water_tank_physics_analysis.png`: Light yield efficiency and detection performance

### 4. Interpret Results

#### Expected Physics Signatures
- **Cherenkov Light Yield**: ~200-300 photons/GeV for high-energy muons
- **First Photon Time**: ~9-10 ns (speed of light propagation + detector response)
- **Wavelength Spectrum**: Peak at 400-450 nm with 1/λ² falloff toward UV
- **Detection Efficiency**: >90% for muons crossing full detector volume
- **Energy Deposition**: ~100-200 MeV for 4 GeV muons (minimum ionizing)

#### Quality Checks
- Energy conservation: Primary energy >> Energy deposition
- Timing consistency: First photon < Last photon times  
- Spectral range: Detected wavelengths within 300-700 nm window
- Spatial distribution: Hits concentrated on DOM hemisphere facing muon track

## Configuration Files

### CRY Setup (`cry_setup.file`)
Controls cosmic ray generation:
```
returnMuons 1          # Generate muons
returnElectrons 0      # Disable electrons  
returnGammas 0         # Disable gammas
latitude 42.36         # IceCube latitude (degrees)
altitude 0             # Sea level (meters)
subboxLength 3         # Generation box size (meters)
date 1-1-2024          # Cosmic ray flux date
```

### Visualization (`vis.mac`)
```bash
/vis/open OGL 800x600-0+0
/vis/drawVolume worlds
/vis/scene/add/trajectories smooth
/vis/modeling/trajectories/create/drawByCharge
/vis/viewer/set/autoRefresh false
```

## File Structure

```
WaterTankTest/
├── README.md                    # This documentation
├── CMakeLists.txt               # Build configuration
├── exampleWaterTank.cc          # Main simulation executable
├── analyze_watertank.C          # ROOT analysis macro
├── test.mac                     # Basic single muon test
├── test_cry.mac                 # Cosmic ray simulation macro
├── init_vis.mac                 # Visualization initialization
├── vis.mac                      # Visualization settings
├── cry_setup.file               # CRY cosmic ray configuration
│
├── include/                     # Header files
│   ├── WaterTankDetectorConstruction.hh
│   ├── WaterTankCRYPrimaryGenerator.hh  
│   ├── WaterTankPrimaryGeneratorAction.hh
│   ├── WaterTankPrimaryGeneratorMessenger.hh
│   ├── WaterTankDOMSD.hh
│   ├── WaterTankDOMHit.hh
│   ├── WaterTankEventAction.hh
│   ├── WaterTankRunAction.hh
│   ├── WaterTankSteppingAction.hh
│   ├── WaterTankAnalysis.hh
│   └── WaterTankActionInitialization.hh
│
├── src/                         # Source files
│   ├── WaterTankDetectorConstruction.cc
│   ├── WaterTankCRYPrimaryGenerator.cc
│   ├── WaterTankPrimaryGeneratorAction.cc  
│   ├── WaterTankPrimaryGeneratorMessenger.cc
│   ├── WaterTankDOMSD.cc
│   ├── WaterTankDOMHit.cc
│   ├── WaterTankEventAction.cc
│   ├── WaterTankRunAction.cc
│   ├── WaterTankSteppingAction.cc
│   └── WaterTankActionInitialization.cc
│
├── build/                       # Build directory (after compilation)
│   ├── exampleWaterTank         # Compiled executable
│   ├── output_default.root      # Simulation output data
│   ├── water_tank_event_analysis.png    # Event analysis plots
│   ├── water_tank_photon_analysis.png   # Photon analysis plots
│   └── water_tank_physics_analysis.png  # Physics validation plots
│
└── cry_v1.7/                    # CRY cosmic ray library
    ├── src/                     # CRY source code
    ├── lib/libCRY.a             # Compiled CRY library
    ├── data/                    # Cosmic ray data files
    └── setup                    # CRY setup script
```

## Key Classes

### `WaterTankDetectorConstruction`
- Defines cylindrical tank geometry (71" × 36") and material properties
- Implements optical surfaces between water/glass/air interfaces
- Configures DOM positioning and PMT quantum efficiency curves
- Sets up wavelength-dependent refractive indices and absorption lengths

### `WaterTankCRYPrimaryGenerator` 
- Interfaces with CRY library for realistic cosmic ray flux simulation
- Handles muon energy/angular distributions based on atmospheric models
- Switches between single muon and cosmic ray shower modes
- Manages particle gun configuration for controlled studies

### `WaterTankDOMSD` (Sensitive Detector)
- Records optical photon interactions with DOM photocathode surface
- Implements realistic photoelectron conversion with quantum efficiency
- Tracks precise timing (sub-nanosecond resolution) and wavelength information
- Simulates PMT response including dark noise and afterpulses

### `WaterTankEventAction` & `WaterTankRunAction`
- Collect comprehensive event-level and run-level statistics
- Manage ROOT ntuple structure with 15 event variables and 12 hit variables
- Coordinate multi-threaded data collection and merging
- Calculate physics observables (light yield, detection efficiency, etc.)

### `WaterTankSteppingAction`
- Tracks particle interactions step-by-step through detector volumes
- Records energy deposition patterns for different particle types
- Monitors Cherenkov photon production and propagation
- Provides detailed trajectory information for analysis

## References and Related Work

### IceCube Collaboration
- [IceCube Neutrino Observatory](https://icecube.wisc.edu/)
- [IceCube Upgrade Technical Design Report](https://icecube.wisc.edu/science/beyond-icecube/upgrade/)

### Geant4 Documentation  
- [Geant4 User Guide](https://geant4-userdoc.web.cern.ch/UsersGuides/ForApplicationDeveloper/html/)
- [Geant4 Optical Physics Reference](https://geant4-userdoc.web.cern.ch/UsersGuides/ForApplicationDeveloper/html/TrackingAndPhysics/physicsProcess.html#optical-photon-processes)

### CRY Cosmic Ray Library
- [CRY: Cosmic Ray Shower Library](https://nuclear.llnl.gov/simulation/doc_cry_v1.7/cry.pdf)
- [CRY Physics Documentation](https://nuclear.llnl.gov/simulation/doc_cry_v1.7/cry_physics.pdf)

## Contact and Support

**Author:** Christopher Prainito  
**Institution:** Harvard College

For questions, bug reports, or collaboration inquiries, please open an issue on GitHub.

---

*This simulation was developed as part of research into calibration systems for the IceCube Upgrade project. The IceCube Neutrino Observatory is operated by the IceCube Collaboration with support from the National Science Foundation.*
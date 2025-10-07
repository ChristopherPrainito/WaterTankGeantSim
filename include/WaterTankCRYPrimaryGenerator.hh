/// \file WaterTankCRYPrimaryGenerator.hh
/// \brief Definition of the WaterTankCRYPrimaryGenerator class

#ifndef WaterTankCRYPrimaryGenerator_h
#define WaterTankCRYPrimaryGenerator_h 1

#include "G4VPrimaryGenerator.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "globals.hh"
#include "Randomize.hh"

// CRY includes
#include "CRYSetup.h"
#include "CRYGenerator.h"
#include "CRYParticle.h"
#include "CRYUtils.h"
#include <vector>

class G4Event;

/// Template class for random number generator wrapper
template<class T>
class RNGWrapper { 
  public:
    static void set(T* object, double (T::*func)(void));
    static double rng(void);
  private:
    static T* m_obj;
    static double (T::*m_func)(void);
};

template<class T> T* RNGWrapper<T>::m_obj;
template<class T> double (T::*RNGWrapper<T>::m_func)(void);

template<class T> void RNGWrapper<T>::set(T* object, double (T::*func)(void)) {
  m_obj = object; m_func = func;
}

template<class T> double RNGWrapper<T>::rng(void) { 
  return (m_obj->*m_func)(); 
}

/// Primary generator using CRY cosmic ray shower library
///
/// This class interfaces with the CRY library to generate realistic 
/// cosmic ray showers. It provides a configurable interface for
/// cosmic ray simulation with geographic and temporal flexibility.

class WaterTankCRYPrimaryGenerator : public G4VPrimaryGenerator
{
  public:
    WaterTankCRYPrimaryGenerator();
    WaterTankCRYPrimaryGenerator(const G4String& setupFile);
    virtual ~WaterTankCRYPrimaryGenerator();

    // Method from base class
    virtual void GeneratePrimaryVertex(G4Event* anEvent);
    
    // Configuration methods
    void SetupCRY(const G4String& setupFile);
    void SetupCRY(const G4String& setupString, const G4String& dataPath);
    
    G4bool IsInitialized() const { return fInitialized; }

  private:
    G4ParticleGun* fParticleGun;
    G4ParticleTable* fParticleTable;
    CRYGenerator* fCRYGenerator;
    std::vector<CRYParticle*>* fParticleVector;
    G4bool fInitialized;
    
    void Initialize();
};

#endif
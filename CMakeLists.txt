
#----------------------------------------------------------------------------
# Setup the project
cmake_minimum_required(VERSION 3.6 FATAL_ERROR)
project(WaterTank)

#----------------------------------------------------------------------------
# Find Geant4 package, activating all available UI and Vis drivers by default
# You can set WITH_GEANT4_UIVIS to OFF via the command line or ccmake/cmake-gui
# to build a batch mode only executable
#
option(WITH_GEANT4_UIVIS "Build example with Geant4 UI and Vis drivers" ON)
if(WITH_GEANT4_UIVIS)
  find_package(Geant4 REQUIRED ui_all vis_all)
else()
  find_package(Geant4 REQUIRED)
endif()

#----------------------------------------------------------------------------
# Setup Geant4 include directories and compile definitions
# Setup include directory for this project
#
include(${Geant4_USE_FILE})
include_directories(${PROJECT_SOURCE_DIR}/include)

#----------------------------------------------------------------------------
# Find and setup CRY cosmic ray shower library
#
set(CRY_DIR ${PROJECT_SOURCE_DIR}/cry_v1.7)
set(CRY_LIB_DIR ${CRY_DIR}/lib)
set(CRY_INC_DIR ${CRY_DIR}/src)
set(CRY_DATA_DIR ${CRY_DIR}/data)

# Check if CRY library exists
if(NOT EXISTS ${CRY_LIB_DIR}/libCRY.a)
    message(FATAL_ERROR "CRY library not found at ${CRY_LIB_DIR}/libCRY.a. Please build CRY first.")
endif()

# Add CRY include directory
include_directories(${CRY_INC_DIR})

# Define CRY_DATA preprocessor definition for the data path
add_definitions(-DCRY_DATA="${CRY_DATA_DIR}")


#----------------------------------------------------------------------------
# Locate sources and headers for this project
# NB: headers are included so they will show up in IDEs
#
file(GLOB sources ${PROJECT_SOURCE_DIR}/src/*.cc)
file(GLOB headers ${PROJECT_SOURCE_DIR}/include/*.hh)

#----------------------------------------------------------------------------
# Add the executable, and link it to the Geant4 libraries
#
add_executable(exampleWaterTank exampleWaterTank.cc ${sources} ${headers})
target_link_libraries(exampleWaterTank ${Geant4_LIBRARIES} ${CRY_LIB_DIR}/libCRY.a)

#----------------------------------------------------------------------------
# Copy all scripts to the build directory, i.e. the directory in which we
# build WaterTank. This is so that we can run the executable directly because it
# relies on these scripts being in the current working directory.
#
set(EXAMPLEWaterTank_SCRIPTS
  init_vis.mac
  vis.mac
  cry_setup.file
  test_cry.mac
  test_cry_simple.mac
  )

foreach(_script ${EXAMPLEWaterTank_SCRIPTS})
  configure_file(
    ${PROJECT_SOURCE_DIR}/${_script}
    ${PROJECT_BINARY_DIR}/${_script}
    COPYONLY
    )
endforeach()

#----------------------------------------------------------------------------
# For internal Geant4 use - but has no effect if you build this
# example standalone
#
add_custom_target(WaterTank DEPENDS exampleWaterTank)

#----------------------------------------------------------------------------
# Install the executable to 'bin' directory under CMAKE_INSTALL_PREFIX
#
install(TARGETS exampleWaterTank DESTINATION bin)



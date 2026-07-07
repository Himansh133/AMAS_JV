cmake_minimum_required(VERSION 3.16)
project(AMAS VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable automatic Qt build pipelines
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find Qt 6 libraries
find_package(Qt6 COMPONENTS Core Widgets SerialPort Charts)

# Auto-detect target bitness and choose VISA defaults
if(CMAKE_SIZEOF_VOID_P EQUAL 4)
    message(STATUS "32-bit compiler detected. Setting VISA paths to 32-bit WinNT directory.")
    set(VISA_INCLUDE_DIR_DEFAULT "C:/Program Files (x86)/IVI Foundation/VISA/WinNT/Include")
    set(VISA_LIBRARY_DIR_DEFAULT "C:/Program Files (x86)/IVI Foundation/VISA/WinNT/lib/msc")
    set(VISA_LIB_NAME "visa32")
else()
    message(STATUS "64-bit compiler detected. Setting VISA paths to 64-bit Win64 directory.")
    set(VISA_INCLUDE_DIR_DEFAULT "C:/Program Files/IVI Foundation/VISA/Win64/Include")
    set(VISA_LIBRARY_DIR_DEFAULT "C:/Program Files/IVI Foundation/VISA/Win64/Lib_x64/msc")
    set(VISA_LIB_NAME "visa64")
endif()

# Configure Keysight VISA paths (changeable via cache variables)
set(VISA_INCLUDE_DIR "${VISA_INCLUDE_DIR_DEFAULT}" CACHE PATH "Path to VISA include directory")
set(VISA_LIBRARY_DIR "${VISA_LIBRARY_DIR_DEFAULT}" CACHE PATH "Path to VISA library directory")

if(EXISTS "${VISA_INCLUDE_DIR}/visa.h")
    message(STATUS "VISA headers found at: ${VISA_INCLUDE_DIR}")
else()
    message(WARNING "visa.h not found at: ${VISA_INCLUDE_DIR}. Please set VISA_INCLUDE_DIR correctly.")
endif()

# Find the VISA library for the detected architecture
find_library(VISA_LIBRARY NAMES ${VISA_LIB_NAME} PATHS ${VISA_LIBRARY_DIR} NO_DEFAULT_PATH)

if(VISA_LIBRARY)
    message(STATUS "VISA library found at: ${VISA_LIBRARY}")
else()
    message(WARNING "${VISA_LIB_NAME} library not found in: ${VISA_LIBRARY_DIR}. Please set VISA_LIBRARY_DIR correctly.")
endif()

# Define VISA Imported Target
add_library(VISA::VISA UNKNOWN IMPORTED)
set_target_properties(VISA::VISA PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${VISA_INCLUDE_DIR}"
    IMPORTED_LOCATION "${VISA_LIBRARY}"
)

# Project include directories
include_directories(
    src
    src/core
    src/core/visa
    src/core/vna
    src/core/positioner
    src/core/log
    src/core/config
    src/core/control
    src/measurement
    src/logger
    src/reports
    src/controllers
    src/gui
    src/gui/theme
    src/gui/widgets
)

# Source files list for the core library and executable
set(SOURCES
    src/main.cpp
    src/Application.h
    src/Application.cpp
    # VISA and hardware drivers
    src/core/visa/VISASession.h
    src/core/visa/VISASession.cpp
    src/core/vna/IVnaDriver.h
    src/core/vna/FieldFoxDriver.h
    src/core/vna/FieldFoxDriver.cpp
    src/core/vna/MockVnaDriver.h
    src/core/vna/MockVnaDriver.cpp
    src/core/positioner/IPositionerDriver.h
    src/core/positioner/MockPositionerDriver.h
    src/core/positioner/MockPositionerDriver.cpp
    src/core/positioner/TAP3001Driver.h
    src/core/positioner/TAP3001Driver.cpp
    # Config and Session
    src/core/config/MeasurementProfile.h
    src/core/config/MeasurementSession.h
    # Logging
    src/core/log/Logger.h
    src/core/log/Logger.cpp
    # Expanded Backend Architecture
    src/measurement/BandConfiguration.h
    src/measurement/BandConfiguration.cpp
    src/measurement/CalibrationManager.h
    src/measurement/CalibrationManager.cpp
    src/measurement/MeasurementProfile.h
    src/measurement/MeasurementProfile.cpp
    src/measurement/MeasurementSession.h
    src/measurement/MeasurementSession.cpp
    src/measurement/MeasurementProcessor.h
    src/measurement/MeasurementProcessor.cpp
    src/logger/DataLogger.h
    src/logger/DataLogger.cpp
    src/reports/ReportGenerator.h
    src/reports/ReportGenerator.cpp
    src/controllers/MeasurementController.h
    src/controllers/MeasurementController.cpp
    # GUI Skeleton
    src/gui/MainWindow.h
    src/gui/MainWindow.cpp
    src/gui/theme/Colors.h
    src/gui/theme/ThemeManager.h
    src/gui/theme/ThemeManager.cpp
    src/gui/DashboardPage.h
    src/gui/DashboardPage.cpp
    src/gui/DevicesPage.h
    src/gui/DevicesPage.cpp
    src/gui/MeasurementSetupPage.h
    src/gui/MeasurementSetupPage.cpp
    src/gui/MeasurementProgressPage.h
    src/gui/MeasurementProgressPage.cpp
    src/gui/ResultsPage.h
    src/gui/ResultsPage.cpp
    src/gui/ProfileManagerPage.h
    src/gui/ProfileManagerPage.cpp
    src/gui/framework/ActionManager.h
    src/gui/framework/ActionManager.cpp
    src/gui/framework/CommandManager.h
    src/gui/framework/CommandManager.cpp
    src/gui/framework/SettingsManager.h
    src/gui/framework/SettingsManager.cpp
    src/gui/framework/WindowStateManager.h
    src/gui/framework/WindowStateManager.cpp
    src/gui/presentation/MeasurementPresenter.h
    src/gui/presentation/MeasurementPresenter.cpp
    src/gui/presentation/DashboardPresenter.h
    src/gui/presentation/DashboardPresenter.cpp
    src/gui/presentation/DevicesPresenter.h
    src/gui/presentation/DevicesPresenter.cpp
    src/gui/presentation/SetupPresenter.h
    src/gui/presentation/SetupPresenter.cpp
    src/gui/presentation/ProgressPresenter.h
    src/gui/presentation/ProgressPresenter.cpp
    src/gui/presentation/ResultsPresenter.h
    src/gui/presentation/ResultsPresenter.cpp
    src/gui/presentation/ProfilePresenter.h
    src/gui/presentation/ProfilePresenter.cpp
    src/gui/widgets/DeviceStatusCard.h
    src/gui/widgets/DeviceStatusCard.cpp
    src/gui/widgets/MeasProfileCard.h
    src/gui/widgets/MeasProfileCard.cpp
    src/gui/widgets/QuickActionButton.h
    src/gui/widgets/QuickActionButton.cpp
    src/gui/widgets/NoWheelWidgets.h
    src/gui/widgets/MeasurementPlotWidget.h
    src/gui/widgets/MeasurementPlotWidget.cpp
)


# Main Executable (Requires Qt6)
if(Qt6_FOUND)
    message(STATUS "Qt6 found. Main GUI target 'AMAS' will be built.")
    add_executable(AMAS ${SOURCES})
    target_link_libraries(AMAS PRIVATE
        Qt6::Core
        Qt6::Widgets
        Qt6::SerialPort
        Qt6::Charts
        VISA::VISA
    )
else()
    message(STATUS "Qt6 not found. Skipping build of main GUI target 'AMAS'. Only CLI tools and tests will be built.")
endif()

# VNA Connection Test Executable
set(VNA_TEST_SOURCES
    src/vna_connection_test.cpp
    src/core/visa/VISASession.h
    src/core/visa/VISASession.cpp
    src/core/vna/IVnaDriver.h
    src/core/vna/FieldFoxDriver.h
    src/core/vna/FieldFoxDriver.cpp
    src/core/log/Logger.h
    src/core/log/Logger.cpp
)
add_executable(vna_connection_test ${VNA_TEST_SOURCES})
target_link_libraries(vna_connection_test PRIVATE
    VISA::VISA
)

# Unit Testing setup using FetchContent for GTest
option(BUILD_TESTING "Build unit tests" ON)
if(BUILD_TESTING)
    enable_testing()
    
    include(FetchContent)
    FetchContent_Declare(
      googletest
      URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
    )
    
    # Prevent overriding MSVC/MinGW runtime library settings
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    set(TEST_SOURCES
        tests/test_vna_driver.cpp
        tests/test_positioner_driver.cpp
        tests/test_backend_modules.cpp
        # Sources under test
        src/core/visa/VISASession.cpp
        src/core/vna/FieldFoxDriver.cpp
        src/core/vna/MockVnaDriver.cpp
        src/core/positioner/MockPositionerDriver.cpp
        src/core/log/Logger.cpp
        # Backend modules
        src/measurement/BandConfiguration.cpp
        src/measurement/CalibrationManager.cpp
        src/measurement/MeasurementProfile.cpp
        src/measurement/MeasurementSession.cpp
        src/measurement/MeasurementProcessor.cpp
        src/logger/DataLogger.cpp
        src/reports/ReportGenerator.cpp
        src/controllers/MeasurementController.cpp
    )
    
    add_executable(AMAS_tests ${TEST_SOURCES})
    target_link_libraries(AMAS_tests PRIVATE
        gtest_main
        VISA::VISA
        Qt6::Core
    )
    
    find_package(Threads REQUIRED)
    target_link_libraries(AMAS_tests PRIVATE Threads::Threads)
    
    add_test(NAME AMAS_unit_tests COMMAND AMAS_tests)
endif()

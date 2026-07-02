#include <gtest/gtest.h>
#include "measurement/BandConfiguration.h"
#include "measurement/CalibrationManager.h"
#include "measurement/MeasurementProfile.h"
#include "measurement/MeasurementSession.h"
#include "measurement/MeasurementProcessor.h"
#include "logger/DataLogger.h"
#include "reports/ReportGenerator.h"
#include "MockVnaDriver.h"
#include "MockPositionerDriver.h"
#include "controllers/MeasurementController.h"
#include <fstream>
#include <filesystem>

using namespace AMAS;

// 1. BandConfiguration Tests
TEST(BandConfigurationTest, PredefinedBandsCheck) {
    const auto& bands = BandConfiguration::getPredefinedBands();
    ASSERT_EQ(bands.size(), 4);
    EXPECT_EQ(bands[0].name, "8-12 GHz");
    EXPECT_EQ(bands[1].name, "12-18 GHz");
    EXPECT_EQ(bands[2].name, "18-26.5 GHz");
    EXPECT_EQ(bands[3].name, "26.5-40 GHz");

    EXPECT_DOUBLE_EQ(bands[0].startFreqHz, 8.0e9);
    EXPECT_DOUBLE_EQ(bands[0].stopFreqHz, 12.0e9);
    EXPECT_EQ(bands[0].sweepPoints, 401);
}

TEST(BandConfigurationTest, GetBandByName) {
    BandInfo band;
    EXPECT_TRUE(BandConfiguration::getBandByName("18-26.5 GHz", band));
    EXPECT_DOUBLE_EQ(band.stopFreqHz, 26.5e9);
    EXPECT_EQ(band.sweepPoints, 851);

    EXPECT_FALSE(BandConfiguration::getBandByName("Non-existent Band", band));
}

TEST(BandConfigurationTest, GetBandByFrequency) {
    BandInfo band;
    // 10 GHz should be in X-Band
    EXPECT_TRUE(BandConfiguration::getBandByFrequency(10.0e9, band));
    EXPECT_EQ(band.name, "8-12 GHz");

    // 22 GHz should be in K-Band
    EXPECT_TRUE(BandConfiguration::getBandByFrequency(22.0e9, band));
    EXPECT_EQ(band.name, "18-26.5 GHz");

    // 5 GHz should not be in any standard band
    EXPECT_FALSE(BandConfiguration::getBandByFrequency(5.0e9, band));
}

// 2. CalibrationManager Tests
TEST(CalibrationManagerTest, DefaultMappings) {
    CalibrationManager calManager;
    
    std::string s11_x = calManager.getCalibrationFile("S11", "8-12 GHz");
    EXPECT_EQ(s11_x, "calchamber_s11_1_18ghz.sta");

    std::string s21_k = calManager.getCalibrationFile("S21", "18-26.5 GHz");
    EXPECT_EQ(s21_k, "calchamber18_26p5ghz.sta");

    std::string invalid = calManager.getCalibrationFile("S11", "Invalid-Band");
    EXPECT_TRUE(invalid.empty());
}

TEST(CalibrationManagerTest, RegisterAndGet) {
    CalibrationManager calManager;
    calManager.registerCalibration("Gain", "Custom-Band", "custom_cal.sta");
    
    EXPECT_EQ(calManager.getCalibrationFile("Gain", "Custom-Band"), "custom_cal.sta");
}

TEST(CalibrationManagerTest, ValidateFile) {
    CalibrationManager calManager;
    
    // Empty path is invalid
    EXPECT_FALSE(calManager.validateCalibrationFile(""));

    // Check if check behaves correctly (non-existent file returns false)
    EXPECT_FALSE(calManager.validateCalibrationFile("non_existent_file_path_xyz.sta"));
}

// 3. MeasurementProfile Tests
TEST(MeasurementProfileTest, DefaultConstructor) {
    MeasurementProfile profile;
    EXPECT_EQ(profile.profileName, "Default Profile");
    EXPECT_EQ(profile.measurementType, "S11");
    EXPECT_DOUBLE_EQ(profile.startFrequencyHz, 8.0e9);
    EXPECT_DOUBLE_EQ(profile.stopFrequencyHz, 12.0e9);
    EXPECT_EQ(profile.sweepPoints, 401);
    EXPECT_DOUBLE_EQ(profile.outputPowerDbm, -15.0);
    EXPECT_DOUBLE_EQ(profile.ifBandwidthHz, 1000.0);
    EXPECT_TRUE(profile.positioner.usePositioner);
    EXPECT_FLOAT_EQ(profile.positioner.startAngleDeg, 0.0f);
}

TEST(MeasurementProfileTest, LoadSaveSerialization) {
    MeasurementProfile profile;
    profile.profileName = "Test K-Band Profile";
    profile.measurementType = "S21";
    profile.startFrequencyHz = 18.0e9;
    profile.stopFrequencyHz = 26.5e9;
    profile.sweepPoints = 851;
    profile.outputPowerDbm = -10.0;
    profile.ifBandwidthHz = 200.0;
    profile.calibrationFile = "k_band_cal.sta";
    profile.positioner.usePositioner = true;
    profile.positioner.startAngleDeg = 10.0f;
    profile.positioner.stopAngleDeg = 90.0f;
    profile.positioner.stepAngleDeg = 2.0f;
    profile.positioner.settleTimeMs = 300;

    std::string tempFile = "temp_test_profile.txt";
    ASSERT_TRUE(profile.saveToFile(tempFile));

    MeasurementProfile loaded;
    ASSERT_TRUE(loaded.loadFromFile(tempFile));

    EXPECT_EQ(loaded.profileName, "Test K-Band Profile");
    EXPECT_EQ(loaded.measurementType, "S21");
    EXPECT_DOUBLE_EQ(loaded.startFrequencyHz, 18.0e9);
    EXPECT_DOUBLE_EQ(loaded.stopFrequencyHz, 26.5e9);
    EXPECT_EQ(loaded.sweepPoints, 851);
    EXPECT_DOUBLE_EQ(loaded.outputPowerDbm, -10.0);
    EXPECT_DOUBLE_EQ(loaded.ifBandwidthHz, 200.0);
    EXPECT_EQ(loaded.calibrationFile, "k_band_cal.sta");
    EXPECT_TRUE(loaded.positioner.usePositioner);
    EXPECT_FLOAT_EQ(loaded.positioner.startAngleDeg, 10.0f);
    EXPECT_FLOAT_EQ(loaded.positioner.stopAngleDeg, 90.0f);
    EXPECT_FLOAT_EQ(loaded.positioner.stepAngleDeg, 2.0f);
    EXPECT_EQ(loaded.positioner.settleTimeMs, 300);

    std::filesystem::remove(tempFile);
}

// 4. MeasurementProcessor Tests
TEST(MeasurementProcessorTest, BasicConversions) {
    // 0 dB linear reflection = 1.0
    EXPECT_NEAR(MeasurementProcessor::linearToDb(1.0), 0.0, 1e-6);
    EXPECT_NEAR(MeasurementProcessor::linearToDb(0.1), -20.0, 1e-6);
    EXPECT_DOUBLE_EQ(MeasurementProcessor::linearToDb(0.0), -150.0); // clamped noise floor

    EXPECT_NEAR(MeasurementProcessor::dbToLinear(0.0), 1.0, 1e-6);
    EXPECT_NEAR(MeasurementProcessor::dbToLinear(-20.0), 0.1, 1e-6);
}

TEST(MeasurementProcessorTest, VSWRAndReturnLoss) {
    // S11 = -20 dB -> return loss is 20 dB, VSWR is ~1.22
    EXPECT_DOUBLE_EQ(MeasurementProcessor::calculateReturnLoss(-20.0), 20.0);
    
    double vswr_20 = MeasurementProcessor::calculateVSWR(-20.0);
    EXPECT_NEAR(vswr_20, 1.2222, 1e-4);

    // S11 = 0 dB -> VSWR is infinity (clamped to 999.0)
    EXPECT_DOUBLE_EQ(MeasurementProcessor::calculateVSWR(0.0), 999.0);
}

TEST(MeasurementProcessorTest, AnalyzePatternCheck) {
    std::vector<float> angles = { 0.0f, 15.0f, 30.0f, 45.0f, 60.0f, 75.0f, 90.0f, 105.0f, 120.0f, 135.0f, 150.0f, 165.0f, 180.0f };
    // Simulated main beam centered at 45 degrees, peak magnitude = 5 dBi
    std::vector<double> mags = { -20.0, -10.0, 1.9, 5.0, 1.9, -8.0, -22.0, -25.0, -30.0, -20.0, -15.0, -22.0, -18.0 };

    auto metrics = MeasurementProcessor::analyzePattern(angles, mags);

    EXPECT_DOUBLE_EQ(metrics.peakGainDb, 5.0);
    EXPECT_FLOAT_EQ(metrics.beamDirectionDeg, 45.0f);
    
    // HPBW crossings should be around 30 degrees (2.0 dBi) and 60 degrees (2.0 dBi)
    // Drop of 3dB from 5 dBi peak is 2 dBi, which is exactly at 30 and 60 deg, width = 30 deg
    EXPECT_NEAR(metrics.beamwidth3dB, 30.0, 1e-6);

    // Front-to-Back: Back angle is 45 + 180 = 225 deg. Since we cap at 180 deg in the input vector,
    // the closest angle to 225 deg is 180 deg (mag = -18 dBi).
    // F/B = 5 - (-18) = 23 dB
    EXPECT_NEAR(metrics.frontToBackRatioDb, 23.0, 1e-1);

    // SLL: Side lobes are peaks outside the main lobe.
    // At index 10 (150 deg), we have a local maximum of -15 dB.
    // SLL = -15 - 5 = -20 dB
    EXPECT_NEAR(metrics.sideLobeLevelDb, -20.0, 1e-1);
}

// 5. DataLogger & ReportGenerator Integration
TEST(LoggerAndReportTest, OutputGeneration) {
    MeasurementSession session;
    session.sessionName = "System Integration Test";
    session.measurementType = "RadiationPattern";
    session.timestamp = "2026-06-29_23-59-59";
    session.bandName = "8-12 GHz";
    session.calibrationFile = "cal_test.sta";
    session.outputFolder = "test_run";
    session.metadata.operatorName = "UnitTester";
    session.metadata.antennaModel = "HORN-12";
    session.metadata.notes = "Checking report factory outputs.";

    // Prepopulate some results
    for (int i = 0; i < 5; ++i) {
        ProcessedMeasurementPoint pt;
        pt.angleDeg = i * 15.0f;
        pt.frequencyHz = 10.0e9;
        pt.magnitudeDb = -10.0 - i;
        pt.phaseDeg = 45.0 * i;
        pt.returnLossDb = 10.0 + i;
        pt.vswr = 1.9;
        session.results.push_back(pt);
    }

    // 1. DataLogger Test
    DataLogger logger;
    logger.setBaseDirectory("test_outputs");
    std::string loggedDir;
    ASSERT_TRUE(logger.logSession(session, loggedDir));
    EXPECT_FALSE(loggedDir.empty());

    // Verify CSV file exists
    std::string csvPath = loggedDir + "/measurement_results.csv";
    EXPECT_TRUE(std::filesystem::exists(csvPath));

    // Verify metadata file exists
    std::string metaPath = loggedDir + "/session_metadata.txt";
    EXPECT_TRUE(std::filesystem::exists(metaPath));

    // 2. ReportGenerator Test
    auto mdReport = ReportGenerator::generateReport(session, ReportFormat::Markdown);
    EXPECT_EQ(mdReport->getFormat(), ReportFormat::Markdown);
    EXPECT_NE(mdReport->getContent().find("# AMAS Measurement & Analysis Report"), std::string::npos);
    EXPECT_NE(mdReport->getContent().find("HORN-12"), std::string::npos);

    auto htmlReport = ReportGenerator::generateReport(session, ReportFormat::HTML);
    EXPECT_EQ(htmlReport->getFormat(), ReportFormat::HTML);
    EXPECT_NE(htmlReport->getContent().find("<!DOCTYPE html>"), std::string::npos);
    EXPECT_NE(htmlReport->getContent().find("RadiationPattern"), std::string::npos);

    std::string mdPath = loggedDir + "/report.md";
    EXPECT_TRUE(ReportGenerator::saveReportToFile(mdReport, mdPath));
    EXPECT_TRUE(std::filesystem::exists(mdPath));

    // Clean up
    std::filesystem::remove_all("test_outputs");
}

// 6. MeasurementController Integration (Smoke Simulation)
TEST(MeasurementControllerTest, SimulatedRunFlow) {
    auto vna = std::make_shared<MockVnaDriver>();
    auto positioner = std::make_shared<MockPositionerDriver>();
    auto calManager = std::make_shared<CalibrationManager>();
    auto logger = std::make_shared<DataLogger>();
    logger->setBaseDirectory("simulated_runs");

    MeasurementController controller(vna, positioner, calManager, logger);

    // Initial state
    EXPECT_FALSE(controller.isVnaConnected());
    EXPECT_FALSE(controller.isPositionerConnected());

    // Connect mock hardware
    ASSERT_TRUE(controller.connectHardware("GPIB0::18::INSTR", "COM3"));
    EXPECT_TRUE(controller.isVnaConnected());
    EXPECT_TRUE(controller.isPositionerConnected());

    // Setup a mock measurement profile
    MeasurementProfile profile;
    profile.profileName = "Simulated Coordinated Sweep";
    profile.measurementType = "RadiationPattern";
    profile.startFrequencyHz = 8.0e9;
    profile.stopFrequencyHz = 12.0e9;
    profile.sweepPoints = 11;
    profile.positioner.usePositioner = true;
    profile.positioner.startAngleDeg = 0.0f;
    profile.positioner.stopAngleDeg = 30.0f;
    profile.positioner.stepAngleDeg = 10.0f; // 0, 10, 20, 30 -> 4 steps
    profile.positioner.settleTimeMs = 50;

    MeasurementSession session;
    int progressCalls = 0;
    auto progressCb = [&progressCalls](float progressPercent, const std::string& status) {
        progressCalls++;
        EXPECT_GE(progressPercent, 0.0f);
        EXPECT_LE(progressPercent, 100.0f);
    };

    ASSERT_TRUE(controller.runMeasurement(profile, session, progressCb));

    EXPECT_GT(progressCalls, 0);
    // 4 angles * 11 frequency points = 44 result points
    EXPECT_EQ(session.results.size(), 44);

    // Clean up
    std::filesystem::remove_all("simulated_runs");
}

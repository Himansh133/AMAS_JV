#include <gtest/gtest.h>
#include "SweepConfig.h"
#include "MockVnaDriver.h"

using namespace AMAS;

// 1. SweepConfig Validation Tests
TEST(SweepConfigTest, DefaultConfigIsValid) {
    SweepConfig config;
    EXPECT_TRUE(config.validate());
}

TEST(SweepConfigTest, InvalidFrequencyRangeFails) {
    SweepConfig config;
    config.startFreq_Hz = 2.5e9;
    config.stopFreq_Hz = 2.4e9; // Start frequency must be less than stop frequency
    EXPECT_FALSE(config.validate());

    config.startFreq_Hz = -1.0; // Frequencies must be positive
    config.stopFreq_Hz = 2.4e9;
    EXPECT_FALSE(config.validate());
}

TEST(SweepConfigTest, InvalidNumPointsFails) {
    SweepConfig config;
    config.numPoints = 1; // Must be > 1 points
    EXPECT_FALSE(config.validate());

    config.numPoints = 20000; // Too many points for hardware limits
    EXPECT_FALSE(config.validate());
}

TEST(SweepConfigTest, InvalidPowerLevelFails) {
    SweepConfig config;
    config.power_dBm = -60.0; // Out of safe range
    EXPECT_FALSE(config.validate());

    config.power_dBm = 30.0; // Out of safe range
    EXPECT_FALSE(config.validate());
}

TEST(SweepConfigTest, InvalidAnglesFails) {
    SweepConfig config;
    config.startAngle_deg = -361.0f; // Invalid angle
    EXPECT_FALSE(config.validate());

    config.stepAngle_deg = -5.0f; // Step angle must be positive
    EXPECT_FALSE(config.validate());
}

// 2. MockVnaDriver Integration Tests
TEST(MockVnaDriverTest, ConnectionFlow) {
    MockVnaDriver vna;
    EXPECT_FALSE(vna.isConnected());
    
    EXPECT_TRUE(vna.connect("GPIB0::18::INSTR"));
    EXPECT_TRUE(vna.isConnected());
    EXPECT_NE(vna.getIdnString().find("Mock"), std::string::npos);
    
    vna.disconnect();
    EXPECT_FALSE(vna.isConnected());
}

TEST(MockVnaDriverTest, ConfigureBeforeConnectFails) {
    MockVnaDriver vna;
    SweepConfig config;
    EXPECT_FALSE(vna.configure(config));
}

TEST(MockVnaDriverTest, SweepAndReadData) {
    MockVnaDriver vna;
    ASSERT_TRUE(vna.connect("GPIB0::18::INSTR"));

    SweepConfig config;
    config.startFreq_Hz = 1.0e9;
    config.stopFreq_Hz = 3.0e9;
    config.numPoints = 101;
    config.sparams = { SParamType::S11, SParamType::S21 };

    ASSERT_TRUE(vna.configure(config));
    ASSERT_TRUE(vna.triggerSweep());
    
    auto results = vna.readSParameters();
    ASSERT_EQ(results.size(), 2);
    
    // Check S11
    EXPECT_EQ(results[0].type, SParamType::S11);
    EXPECT_EQ(results[0].frequencies_Hz.size(), 101);
    EXPECT_EQ(results[0].magnitudes_dB.size(), 101);
    EXPECT_EQ(results[0].phases_deg.size(), 101);
    EXPECT_NEAR(results[0].frequencies_Hz.front(), 1.0e9, 1e-6);
    EXPECT_NEAR(results[0].frequencies_Hz.back(), 3.0e9, 1e-6);
    EXPECT_NEAR(results[0].frequencies_Hz[50], 2.0e9, 1e-6); // Midpoint frequency

    // Check S21
    EXPECT_EQ(results[1].type, SParamType::S21);
    EXPECT_EQ(results[1].frequencies_Hz.size(), 101);
    EXPECT_EQ(results[1].frequencies_Hz.front(), 1.0e9);
}

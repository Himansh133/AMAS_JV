#include <gtest/gtest.h>
#include "MockPositionerDriver.h"

using namespace AMAS;

TEST(MockPositionerDriverTest, ConnectionFlow) {
    MockPositionerDriver pos;
    EXPECT_FALSE(pos.isConnected());
    
    EXPECT_TRUE(pos.connect("COM3"));
    EXPECT_TRUE(pos.isConnected());
    EXPECT_FLOAT_EQ(pos.getCurrentAngle(), 0.0f);
    
    pos.disconnect();
    EXPECT_FALSE(pos.isConnected());
}

TEST(MockPositionerDriverTest, MoveAndSettle) {
    MockPositionerDriver pos;
    ASSERT_TRUE(pos.connect("COM3"));

    // Move to 90 degrees
    EXPECT_TRUE(pos.moveTo(90.0f));
    
    // Positioner shouldn't immediately reach 90 degrees (simulating physics)
    EXPECT_FLOAT_EQ(pos.getCurrentAngle(), 0.0f);

    // Wait for settle, providing enough timeout (e.g. 5000ms. 90deg / 30deg/s = 3s rotation + buffer)
    EXPECT_TRUE(pos.waitForSettle(5000));
    
    // Positioner should now have settled at 90.0 degrees
    EXPECT_FLOAT_EQ(pos.getCurrentAngle(), 90.0f);
}

TEST(MockPositionerDriverTest, MoveTimeout) {
    MockPositionerDriver pos;
    ASSERT_TRUE(pos.connect("COM3"));

    // Move to 180 degrees (requires 180 / 30 = 6 seconds)
    EXPECT_TRUE(pos.moveTo(180.0f));

    // Wait with a small timeout (e.g. 1000ms), which should trigger a timeout failure
    EXPECT_FALSE(pos.waitForSettle(1000));
    
    // The angle should be partially rotated (approx 30 degrees)
    EXPECT_GT(pos.getCurrentAngle(), 0.0f);
    EXPECT_LT(pos.getCurrentAngle(), 180.0f);
}

TEST(MockPositionerDriverTest, HomeFlow) {
    MockPositionerDriver pos;
    ASSERT_TRUE(pos.connect("COM3"));

    // Move to some position first
    EXPECT_TRUE(pos.moveTo(45.0f));
    EXPECT_TRUE(pos.waitForSettle(3000));
    EXPECT_FLOAT_EQ(pos.getCurrentAngle(), 45.0f);

    // Call home
    pos.home();
    
    // Homing should block and return to 0
    EXPECT_FLOAT_EQ(pos.getCurrentAngle(), 0.0f);
}

TEST(MockPositionerDriverTest, EmergencyStop) {
    MockPositionerDriver pos;
    ASSERT_TRUE(pos.connect("COM3"));

    EXPECT_TRUE(pos.moveTo(90.0f));
    
    // Stop immediately
    pos.emergencyStop();
    
    // Wait for settle should succeed immediately because we stopped
    EXPECT_TRUE(pos.waitForSettle(1000));
    
    // Current angle should remain 0.0f since we stopped before moving
    EXPECT_FLOAT_EQ(pos.getCurrentAngle(), 0.0f);
}

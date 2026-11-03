/**
 * @file test_usd1_v0_parsing.cpp
 * @brief Unit test that verifies the USD1 driver correctly parses V0 (3‑byte) frames
 *        even when the data stream is split across multiple reads.
 */

#include <gtest/gtest.h>

#include "AP_RangeFinder_USD1_Serial.h"
#include "AP_RangeFinder.h"
#include "AP_SerialManager.h"

// A mock SerialManager that feeds a predefined byte stream to the driver.
class MockSerialManager : public AP_SerialManager {
public:
    MockSerialManager(const std::vector<uint8_t> &stream) : _stream(stream), _pos(0) {}

    // Simulate read_uart() used inside the driver.
    bool read_uart(uint8_t &c)
    {
        if (_pos >= _stream.size()) {
            return false;
        }
        c = _stream[_pos++];
        return true;
    }

    // Provide the driver access to the mock read function.
    bool read_byte(uint8_t &c) { return read_uart(c); }

private:
    std::vector<uint8_t> _stream;
    size_t _pos;
};

// Helper that injects the mock manager into the driver and drives get_reading().
static float read_distance_from_stream(const std::vector<uint8_t> &bytes, uint8_t version)
{
    MockSerialManager mock(bytes);
    AP_RangeFinder frontend; // dummy frontend – not used for distance calculation
    AP_RangeFinder_USD1_Serial driver(frontend, mock);
    driver.set_version(version);
    float distance = 0.0f;
    // Call get_reading repeatedly to simulate multiple driver updates.
    for (size_t i = 0; i < bytes.size(); ++i) {
        if (driver.get_reading(distance)) {
            return distance;
        }
    }
    return -1.0f; // indicate failure
}

TEST(USD1V0Parsing, SingleCompleteFrame)
{
    // Frame: header 0x48, high byte 0x01, low byte 0xF4 => 0x01F4 = 500 cm
    std::vector<uint8_t> frame = {0x48, 0x01, 0xF4};
    float dist = read_distance_from_stream(frame, 0);
    EXPECT_NEAR(dist, 500.0f, 0.1f);
}

TEST(USD1V0Parsing, SplitAcrossCalls)
{
    // Send the same frame but split into two reads: first only header, then the rest.
    std::vector<uint8_t> split = {0x48, 0x01, 0xF4};
    // Simulate driver being called after each byte arrives.
    MockSerialManager mock(split);
    AP_RangeFinder frontend;
    AP_RangeFinder_USD1_Serial driver(frontend, mock);
    driver.set_version(0);
    float distance = 0.0f;
    uint8_t c;
    // First byte – should not produce a reading.
    ASSERT_TRUE(mock.read_byte(c));
    driver._process_byte(c); // private helper assumed; if not accessible, we simulate via get_reading
    ASSERT_FALSE(driver.get_reading(distance));
    // Remaining bytes – now a full frame.
    while (mock.read_byte(c)) {
        driver._process_byte(c);
    }
    ASSERT_TRUE(driver.get_reading(distance));
    EXPECT_NEAR(distance, 500.0f, 0.1f);
}


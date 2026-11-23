// Host-side tests. Expected bytes come from the protocol notes
// (notes/skill-galaxys-edge-droid-bluetooth.md) and the Python implementation.
#include <unity.h>

#include "astromech_config.h"
#include "astromech_protocol.h"

using namespace astromech;
using namespace astromech::protocol;

void setUp() {}
void tearDown() {}

static void expect(const uint8_t* want, size_t want_len, const Command& got) {
  TEST_ASSERT_EQUAL_UINT(want_len, got.len);
  TEST_ASSERT_EQUAL_HEX8_ARRAY(want, got.bytes, want_len);
}

void test_login_is_raw() {
  const uint8_t w[] = {0x22, 0x20, 0x01};
  expect(w, sizeof(w), login());
}

void test_ping() {
  const uint8_t w[] = {0x25, 0x42, 0x02, 0x42, 0x00, 0x00};
  expect(w, sizeof(w), ping());
}

void test_audio() {
  const uint8_t g[] = {0x27, 0x42, 0x0f, 0x44, 0x44, 0x00, 0x1f, 0x03};
  expect(g, sizeof(g), setAudioGroup(3));
  const uint8_t p[] = {0x27, 0x42, 0x0f, 0x44, 0x44, 0x00, 0x18, 0x00};  // notes: "Play a sound"
  expect(p, sizeof(p), playFromGroup(0));
  const uint8_t v[] = {0x27, 0x42, 0x0f, 0x44, 0x44, 0x00, 0x0e, 0x1f};  // notes: max volume
  expect(v, sizeof(v), setVolume(0x1f));
  expect(v, sizeof(v), setVolume(0xff));  // clamped
}

void test_motor_matches_notes() {
  // Rotate head left: 29 42 05 46 02 a0 01 2c 00 00
  const uint8_t head_l[] = {0x29, 0x42, 0x05, 0x46, 0x02, 0xa0, 0x01, 0x2c, 0x00, 0x00};
  expect(head_l, sizeof(head_l), motor(Direction::Left, Motor::Head, 0xa0, 300));
  const uint8_t head_r[] = {0x29, 0x42, 0x05, 0x46, 0x82, 0xa0, 0x01, 0x2c, 0x00, 0x00};
  expect(head_r, sizeof(head_r), motor(Direction::Right, Motor::Head, 0xa0, 300));
  const uint8_t right_fwd[] = {0x29, 0x42, 0x05, 0x46, 0x01, 0xa0, 0x01, 0x2c, 0x00, 0x00};
  expect(right_fwd, sizeof(right_fwd), motor(Direction::Forward, Motor::Right, 0xa0, 300));
  const uint8_t left_back[] = {0x29, 0x42, 0x05, 0x46, 0x80, 0xa0, 0x01, 0x2c, 0x00, 0x00};
  expect(left_back, sizeof(left_back), motor(Direction::Backward, Motor::Left, 0xa0, 300));
  const uint8_t stop[] = {0x29, 0x42, 0x05, 0x46, 0x00, 0x00, 0x01, 0x2c, 0x00, 0x00};
  expect(stop, sizeof(stop), motor(Direction::Forward, Motor::Left, 0, 300));
}

void test_motor_delay_big_endian() {
  const uint8_t w[] = {0x29, 0x42, 0x05, 0x46, 0x02, 0x80, 0x12, 0x34, 0x56, 0x78};
  expect(w, sizeof(w), motor(Direction::Left, Motor::Head, 0x80, 0x1234, 0x5678));
}

void test_center_head() {
  const uint8_t a[] = {0x27, 0x42, 0x0f, 0x44, 0x44, 0x01, 0xff, 0x00};  // notes
  expect(a, sizeof(a), centerHead(0xff, true));
  const uint8_t b[] = {0x27, 0x42, 0x0f, 0x44, 0x44, 0x01, 0xa0, 0x01};
  expect(b, sizeof(b), centerHead(0xa0, false));
}

void test_encode_truncates() {
  uint8_t big[40] = {0};
  Command c = encode(0x0f, big, sizeof(big));
  TEST_ASSERT_EQUAL_UINT(kMaxCommandLen, c.len);
  TEST_ASSERT_EQUAL_HEX8(0x40 + 31, c.bytes[3]);
}

void test_personality_beacons() {
  // Status byte is 0x01, plus 0x80 when paired with a remote (formula in the notes
  // and in python beacon.py). The notes' example table shows 0x81 throughout, i.e.
  // the paired form; we follow the formula/Python for the unpaired default.
  const uint8_t blue[] = {0x03, 0x04, 0x44, 0x01, 0x8A, 0x03};
  auto p = personalityBeacon(Affiliation::Resistance, kChipBlue);
  TEST_ASSERT_EQUAL_UINT(6, p.len);
  TEST_ASSERT_EQUAL_HEX8_ARRAY(blue, p.bytes, 6);
  const uint8_t red_paired[] = {0x03, 0x04, 0x44, 0x81, 0x92, 0x05};  // notes table
  p = personalityBeacon(Affiliation::FirstOrder, kChipRed, true);
  TEST_ASSERT_EQUAL_HEX8_ARRAY(red_paired, p.bytes, 6);
  const uint8_t rdef_paired[] = {0x03, 0x04, 0x44, 0x81, 0x82, 0x01};  // notes table
  p = personalityBeacon(Affiliation::Scoundrel, kChipRSeriesDefault, true);
  TEST_ASSERT_EQUAL_HEX8_ARRAY(rdef_paired, p.bytes, 6);
  // The server's "silent" keep-awake beacon: affiliation 0x0d -> 0x9a, chip 1.
  const uint8_t silent[] = {0x03, 0x04, 0x44, 0x01, 0x9A, 0x01};
  p = personalityBeacon(Affiliation::Silent, kChipRSeriesDefault);
  TEST_ASSERT_EQUAL_HEX8_ARRAY(silent, p.bytes, 6);
}

void test_location_beacon() {
  const uint8_t m[] = {0x0A, 0x04, 0x01, 0x02, 0xA6, 0x01};  // notes: marketplace
  auto p = locationBeacon(1, 0x02);
  TEST_ASSERT_EQUAL_HEX8_ARRAY(m, p.bytes, 6);
  const uint8_t d[] = {0x0A, 0x04, 0x04, 0x0C, 0xA6, 0x01};  // Python defaults
  p = locationBeacon(4);
  TEST_ASSERT_EQUAL_HEX8_ARRAY(d, p.bytes, 6);
}

void test_manufacturer_data() {
  uint8_t out[8];
  auto p = personalityBeacon(Affiliation::Resistance, kChipBlue);
  size_t n = manufacturerData(p, out);
  const uint8_t w[] = {0x83, 0x01, 0x03, 0x04, 0x44, 0x01, 0x8A, 0x03};
  TEST_ASSERT_EQUAL_UINT(8, n);
  TEST_ASSERT_EQUAL_HEX8_ARRAY(w, out, 8);
}

void test_generated_config() {
  const Personality* p = findPersonality("resistance_blue");
  TEST_ASSERT_NOT_NULL(p);
  TEST_ASSERT_TRUE(p->count > 0);
  TEST_ASSERT_NULL(findPersonality("nope"));
  TEST_ASSERT_EQUAL_UINT8(0xa0, config::k_r2_defaults.wheel_speed);
  TEST_ASSERT_EQUAL_UINT16(300, config::k_r2_defaults.wheel_ramp_ms);
  TEST_ASSERT_EQUAL_UINT8(0x80, config::k_bb_defaults.head_speed);
  TEST_ASSERT_EQUAL_UINT16(0, config::k_bb_defaults.head_ramp_ms);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_login_is_raw);
  RUN_TEST(test_ping);
  RUN_TEST(test_audio);
  RUN_TEST(test_motor_matches_notes);
  RUN_TEST(test_motor_delay_big_endian);
  RUN_TEST(test_center_head);
  RUN_TEST(test_encode_truncates);
  RUN_TEST(test_personality_beacons);
  RUN_TEST(test_location_beacon);
  RUN_TEST(test_manufacturer_data);
  RUN_TEST(test_generated_config);
  return UNITY_END();
}

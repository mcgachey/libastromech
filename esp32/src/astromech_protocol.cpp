#include "astromech_protocol.h"

namespace astromech {
namespace protocol {

Command encode(uint8_t command_id, const uint8_t* data, size_t data_len) {
  if (data_len > kMaxDataLen) data_len = kMaxDataLen;
  Command c;
  c.bytes[0] = static_cast<uint8_t>(0x1f + 4 + data_len);
  c.bytes[1] = 0x42;
  c.bytes[2] = command_id;
  c.bytes[3] = static_cast<uint8_t>(0x40 + data_len);
  for (size_t i = 0; i < data_len; i++) c.bytes[4 + i] = data[i];
  c.len = 4 + data_len;
  return c;
}

Command login() {
  Command c;
  c.bytes[0] = 0x22;
  c.bytes[1] = 0x20;
  c.bytes[2] = 0x01;
  c.len = 3;
  return c;
}

Command ping() {
  const uint8_t d[] = {0, 0};
  return encode(kCmdPing, d, sizeof(d));
}

static Command audio(uint8_t cmd, uint8_t param) {
  const uint8_t d[] = {0x44, 0x00, cmd, param};
  return encode(kCmdAudioOrHead, d, sizeof(d));
}

Command setAudioGroup(uint8_t group) { return audio(0x1f, group); }
Command playFromGroup(uint8_t sound) { return audio(0x18, sound); }
Command setVolume(uint8_t volume) { return audio(0x0e, volume > 0x1f ? 0x1f : volume); }

Command motor(Direction d, Motor m, uint8_t speed, uint16_t ramp_ms, uint16_t delay_after_ms) {
  const uint8_t data[] = {
      static_cast<uint8_t>(static_cast<uint8_t>(d) | static_cast<uint8_t>(m)),
      speed,
      static_cast<uint8_t>(ramp_ms >> 8),
      static_cast<uint8_t>(ramp_ms & 0xff),
      static_cast<uint8_t>(delay_after_ms >> 8),
      static_cast<uint8_t>(delay_after_ms & 0xff),
  };
  return encode(kCmdMotor, data, sizeof(data));
}

Command centerHead(uint8_t speed, bool stop_at_center) {
  const uint8_t d[] = {0x44, 0x01, speed, static_cast<uint8_t>(stop_at_center ? 0x00 : 0x01)};
  return encode(kCmdAudioOrHead, d, sizeof(d));
}

BeaconPayload locationBeacon(uint8_t location_id, uint8_t interval, uint8_t rssi_threshold,
                             uint8_t flag) {
  BeaconPayload p = {{kLocationBeaconType, 0x04, location_id, interval, rssi_threshold, flag}, 6};
  return p;
}

BeaconPayload personalityBeacon(Affiliation a, uint8_t chip_id, bool paired_with_remote) {
  const uint8_t status = static_cast<uint8_t>(0x01 | (paired_with_remote ? 0x80 : 0x00));
  const uint8_t aff = static_cast<uint8_t>(0x80 + static_cast<uint8_t>(a) * 2);
  BeaconPayload p = {{kPersonalityBeaconType, 0x04, 0x44, status, aff, chip_id}, 6};
  return p;
}

size_t manufacturerData(const BeaconPayload& p, uint8_t* out) {
  out[0] = kDisneyManufacturerId & 0xff;
  out[1] = kDisneyManufacturerId >> 8;
  for (size_t i = 0; i < p.len; i++) out[2 + i] = p.bytes[i];
  return 2 + p.len;
}

}  // namespace protocol
}  // namespace astromech

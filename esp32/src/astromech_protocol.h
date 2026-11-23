#pragma once

// Pure wire-format encoding: no Arduino/BLE dependencies, so it is unit-tested
// on the host (`pio test -e native`). Mirrors python/src/libastromech.

#include <stddef.h>
#include <stdint.h>

#include "astromech_types.h"

namespace astromech {
namespace protocol {

// GATT identifiers (notes: "BLE Service & Characteristics").
static const char* const kServiceUuid = "09b600a0-3e42-41fc-b474-e9c0c8f0c801";
static const char* const kCommandUuid = "09b600b1-3e42-41fc-b474-e9c0c8f0c801";
static const char* const kNotifyUuid = "09b600b0-3e42-41fc-b474-e9c0c8f0c801";

static const uint8_t kCmdPing = 0x02;
static const uint8_t kCmdMotor = 0x05;
static const uint8_t kCmdAudioOrHead = 0x0f;

static const size_t kMaxDataLen = 31;
static const size_t kMaxCommandLen = 4 + kMaxDataLen;

struct Command {
  uint8_t bytes[kMaxCommandLen];
  size_t len;
};

// [0x1f + 4 + n, 0x42, id, 0x40 + n, data...]. Data beyond 31 bytes is dropped.
Command encode(uint8_t command_id, const uint8_t* data, size_t data_len);

// Raw login frame (22 20 01), not wrapped; send twice after connecting.
Command login();
Command ping();
Command setAudioGroup(uint8_t group);
Command playFromGroup(uint8_t sound);
Command setVolume(uint8_t volume /* 0..0x1f */);
Command motor(Direction d, Motor m, uint8_t speed, uint16_t ramp_ms, uint16_t delay_after_ms = 0);
Command centerHead(uint8_t speed, bool stop_at_center);

// ---- Beacons (manufacturer ID 0x0183) -------------------------------------

static const uint16_t kDisneyManufacturerId = 0x0183;
static const uint8_t kLocationBeaconType = 0x0A;
static const uint8_t kPersonalityBeaconType = 0x03;

enum class Affiliation : uint8_t {
  Scoundrel = 1,
  Resistance = 5,
  FirstOrder = 9,
  Silent = 0x0d,  // droids ignore it but it still resets their sleep timer
};

// Chip IDs (notes: "Personality Chips").
enum ChipId : uint8_t {
  kChipRSeriesDefault = 0x01,
  kChipBBSeriesDefault = 0x02,
  kChipBlue = 0x03,
  kChipGray = 0x04,
  kChipRed = 0x05,
  kChipOrange = 0x06,
  kChipPurple = 0x07,
  kChipBlack = 0x08,
  kChipCB23 = 0x09,
  kChipYellow = 0x0A,
  kChipC110P = 0x0B,
  kChipDO = 0x0C,
  kChipBlue2 = 0x0D,
  kChipBD1 = 0x0E,
  kChipALT = 0x0F,
  kChipWhiteDrum = 0x10,
};

struct BeaconPayload {
  uint8_t bytes[6];
  size_t len;
};

BeaconPayload locationBeacon(uint8_t location_id, uint8_t interval = 0x0C,
                             uint8_t rssi_threshold = 0xA6, uint8_t flag = 0x01);
BeaconPayload personalityBeacon(Affiliation a, uint8_t chip_id, bool paired_with_remote = false);

// Full manufacturer-specific AD data: company ID (little-endian) + payload.
// Returns the number of bytes written (always 2 + payload.len); out must hold 8.
size_t manufacturerData(const BeaconPayload& p, uint8_t* out);

}  // namespace protocol
}  // namespace astromech

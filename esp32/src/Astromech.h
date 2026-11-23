#pragma once

// ESP32 port of libastromech. One Astromech object == one BLE connection to one
// droid; use one board per droid (NimBLE is not multiplexed across droids here).
// The Beacon class lives on a separate board.
//
// API is blocking and exception-free: methods return true on success. Sends are
// serialised by a mutex, so a heartbeat task may share the object with an HTTP
// task (the equivalent of the Python library's asyncio lock).

#include <Arduino.h>
#include <NimBLEDevice.h>

#include <mutex>

#include "astromech_ble.h"
#include "astromech_config.h"
#include "astromech_protocol.h"
#include "astromech_types.h"

namespace astromech {

// Pass to speed/ramp parameters to use the droid-type default from config.
static const int kDefault = -1;

struct DiscoveredDroid {
  char address[18];  // "XX:XX:XX:XX:XX:XX"
  uint8_t address_type;
  int rssi;
};

// Scan for devices advertising as "DROID". Returns the number written to out.
size_t scanForDroids(DiscoveredDroid* out, size_t max, uint32_t scan_ms = 5000);

class Astromech {
 public:
  typedef void (*NotifyCallback)(const uint8_t* data, size_t len, void* ctx);

  // mac: "XX:XX:XX:XX:XX:XX" (droids use random addresses). personality may be null.
  Astromech(const char* mac, const DroidDefaults& defaults, const Personality* personality);
  virtual ~Astromech();

  // ---- connection ---------------------------------------------------------
  bool connect(uint32_t timeout_ms = 30000);  // connect + notify + double login
  void disconnect();
  bool isConnected();
  // Ping; on failure drop and re-establish the connection. Returns availability.
  // Call periodically (the Python keep_alive / server heartbeat equivalent).
  bool checkAlive();
  bool ping();
  // Milliseconds since the last successful command/ping; UINT32_MAX if never.
  uint32_t msSinceLastSuccess() const;

  void onNotification(NotifyCallback cb, void* ctx = nullptr);

  // ---- audio --------------------------------------------------------------
  bool setAudioGroup(uint8_t group);
  bool playSoundFromCurrentGroup(uint8_t sound);
  bool setVolume(uint8_t volume);  // 0..0x1f
  // wait=true blocks for the clip duration (sound.ms).
  bool play(const Sound& sound, bool wait = false);
  // Play personality sound by index; false if no personality or out of range.
  bool play(size_t index, bool wait = false);

  // ---- motion -------------------------------------------------------------
  bool stop(uint32_t delay_ms = 0);
  // Start, change or stop one wheel motor (speed 0 stops it). It keeps running until told otherwise.
  bool setWheel(Direction d, Motor m, int speed, int ramp_ms = kDefault) {
    return m != Motor::Head && sendMotor(d, m, speed, ramp_ms);
  }

  const Personality* personality() const { return personality_; }
  const char* mac() const { return mac_; }

 protected:
  bool moveWheels(Direction left, Direction right, uint32_t duration_ms, int left_speed,
                  int right_speed, int ramp_ms);
  bool sendMotor(Direction d, Motor m, int speed, int ramp_ms, uint16_t delay_after = 0);
  bool send(const protocol::Command& c);

  DroidDefaults defaults_;

 private:
  bool doConnect(uint32_t timeout_ms);
  bool rawWrite(const protocol::Command& c);
  void markSuccess();
  static void notifyTrampoline(NimBLERemoteCharacteristic* chr, uint8_t* data, size_t len,
                               bool is_notify);

  char mac_[18];
  const Personality* personality_;
  NimBLEClient* client_ = nullptr;
  NimBLERemoteCharacteristic* command_char_ = nullptr;
  NotifyCallback notify_cb_ = nullptr;
  void* notify_ctx_ = nullptr;
  std::recursive_mutex mutex_;
  uint32_t last_success_ms_ = 0;
  bool ever_succeeded_ = false;
};

}  // namespace astromech

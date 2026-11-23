#include "Astromech.h"

namespace astromech {

namespace {
// The notify trampoline has no context parameter, so (with one droid per board)
// route notifications through the single live instance.
Astromech* g_notify_target = nullptr;
}  // namespace

size_t scanForDroids(DiscoveredDroid* out, size_t max, uint32_t scan_ms) {
  NimBLEScan* scan = NimBLEDevice::getScan();
  scan->setActiveScan(true);
  NimBLEScanResults results = scan->getResults(scan_ms, false);
  size_t n = 0;
  for (int i = 0; i < results.getCount() && n < max; i++) {
    const NimBLEAdvertisedDevice* d = results.getDevice(i);
    if (d->getName() != "DROID") continue;
    strncpy(out[n].address, d->getAddress().toString().c_str(), sizeof(out[n].address) - 1);
    out[n].address[sizeof(out[n].address) - 1] = '\0';
    out[n].address_type = d->getAddress().getType();
    out[n].rssi = d->getRSSI();
    n++;
  }
  scan->clearResults();
  return n;
}

Astromech::Astromech(const char* mac, const DroidDefaults& defaults, const Personality* personality)
    : defaults_(defaults), personality_(personality) {
  strncpy(mac_, mac, sizeof(mac_) - 1);
  mac_[sizeof(mac_) - 1] = '\0';
}

Astromech::~Astromech() {
  disconnect();
  if (g_notify_target == this) g_notify_target = nullptr;
}

void Astromech::notifyTrampoline(NimBLERemoteCharacteristic*, uint8_t* data, size_t len, bool) {
  Astromech* self = g_notify_target;
  if (self && self->notify_cb_) self->notify_cb_(data, len, self->notify_ctx_);
}

void Astromech::onNotification(NotifyCallback cb, void* ctx) {
  notify_cb_ = cb;
  notify_ctx_ = ctx;
}

void Astromech::markSuccess() {
  last_success_ms_ = millis();
  ever_succeeded_ = true;
}

uint32_t Astromech::msSinceLastSuccess() const {
  return ever_succeeded_ ? millis() - last_success_ms_ : UINT32_MAX;
}

bool Astromech::isConnected() { return client_ && client_->isConnected() && command_char_; }

bool Astromech::rawWrite(const protocol::Command& c) {
  if (!isConnected()) return false;
  return command_char_->writeValue(c.bytes, c.len, /*response=*/true);
}

bool Astromech::doConnect(uint32_t timeout_ms) {
  if (!client_) {
    client_ = NimBLEDevice::createClient();
  }
  client_->setConnectTimeout(timeout_ms);
  command_char_ = nullptr;

  NimBLEAddress addr(std::string(mac_), BLE_ADDR_RANDOM);
  if (!client_->connect(addr)) return false;

  NimBLERemoteService* svc = client_->getService(protocol::kServiceUuid);
  NimBLERemoteCharacteristic* cmd = svc ? svc->getCharacteristic(protocol::kCommandUuid) : nullptr;
  NimBLERemoteCharacteristic* notify = svc ? svc->getCharacteristic(protocol::kNotifyUuid) : nullptr;
  if (!cmd || !notify) {
    client_->disconnect();
    return false;
  }
  g_notify_target = this;
  if (!notify->subscribe(true, &Astromech::notifyTrampoline)) {
    client_->disconnect();
    return false;
  }
  command_char_ = cmd;

  // The droid drops us within seconds unless it sees the login twice.
  delay(500);
  if (!rawWrite(protocol::login()) || !rawWrite(protocol::login())) {
    client_->disconnect();
    command_char_ = nullptr;
    return false;
  }
  markSuccess();
  return true;
}

bool Astromech::connect(uint32_t timeout_ms) {
  std::lock_guard<std::recursive_mutex> lock(mutex_);
  if (isConnected()) return true;
  return doConnect(timeout_ms);
}

void Astromech::disconnect() {
  std::lock_guard<std::recursive_mutex> lock(mutex_);
  if (client_) {
    if (client_->isConnected()) client_->disconnect();
    NimBLEDevice::deleteClient(client_);
    client_ = nullptr;
  }
  command_char_ = nullptr;
}

// Up to 3 attempts, reconnecting from scratch between failures (as the Python
// _execute_with_retry does).
bool Astromech::send(const protocol::Command& c) {
  std::lock_guard<std::recursive_mutex> lock(mutex_);
  for (int attempt = 0; attempt < 3; attempt++) {
    if (!isConnected()) {
      disconnect();
      if (!doConnect(30000)) {
        delay(1000);
        continue;
      }
    }
    if (rawWrite(c)) {
      markSuccess();
      return true;
    }
    disconnect();  // force full reconnect on the next attempt
    delay(1000);
  }
  return false;
}

bool Astromech::ping() { return send(protocol::ping()); }

bool Astromech::checkAlive() { return ping(); }  // send() already reconnects

bool Astromech::setAudioGroup(uint8_t group) { return send(protocol::setAudioGroup(group)); }

bool Astromech::playSoundFromCurrentGroup(uint8_t sound) {
  return send(protocol::playFromGroup(sound));
}

bool Astromech::setVolume(uint8_t volume) { return send(protocol::setVolume(volume)); }

bool Astromech::play(const Sound& sound, bool wait) {
  {
    // Hold the lock across the pair so a concurrent play() can't slip its group
    // in between. It must be released before waiting, or the clip duration would
    // block every other task (e.g. a head movement) from using the droid.
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (!setAudioGroup(sound.group) || !playSoundFromCurrentGroup(sound.sound)) return false;
  }
  if (wait) delay(sound.ms);
  return true;
}

bool Astromech::play(size_t index, bool wait) {
  if (!personality_ || index >= personality_->count) return false;
  return play(personality_->sounds[index], wait);
}

bool Astromech::sendMotor(Direction d, Motor m, int speed, int ramp_ms, uint16_t delay_after) {
  const bool head = (m == Motor::Head);
  const uint8_t s = speed != kDefault ? speed : (head ? defaults_.head_speed : defaults_.wheel_speed);
  const uint16_t r = ramp_ms != kDefault ? ramp_ms : (head ? defaults_.head_ramp_ms : defaults_.wheel_ramp_ms);
  return send(protocol::motor(d, m, s, r, delay_after));
}

bool Astromech::moveWheels(Direction left, Direction right, uint32_t duration_ms, int left_speed,
                           int right_speed, int ramp_ms) {
  bool ok = sendMotor(left, Motor::Left, left_speed, ramp_ms) &&
            sendMotor(right, Motor::Right, right_speed, ramp_ms);
  // Always attempt the stop, even if a start command failed.
  return stop(duration_ms) && ok;
}

bool Astromech::stop(uint32_t delay_ms) {
  if (delay_ms) delay(delay_ms);
  // Stop commands carry speed 0; ramp uses the wheel default as in Python.
  bool l = sendMotor(Direction::Forward, Motor::Left, 0, kDefault);
  bool r = sendMotor(Direction::Forward, Motor::Right, 0, kDefault);
  return l && r;
}

}  // namespace astromech

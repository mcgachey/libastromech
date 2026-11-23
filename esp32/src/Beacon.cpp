#include "Beacon.h"

namespace astromech {

bool Beacon::start(const protocol::BeaconPayload& payload) {
  payload_ = payload;
  uint8_t mfg[8];
  size_t n = protocol::manufacturerData(payload_, mfg);

  NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();
  adv->stop();
  adv->reset();
  NimBLEAdvertisementData data;
  data.setManufacturerData(mfg, n);
  if (!adv->setAdvertisementData(data)) return false;
  adv->setConnectableMode(BLE_GAP_CONN_MODE_NON);
  adv->enableScanResponse(false);
  adv->setMinInterval(160);  // 100 ms (0.625 ms units)
  adv->setMaxInterval(240);  // 150 ms
  configured_ = true;
  started_ms_ = millis();
  return adv->start();
}

void Beacon::stop() { NimBLEDevice::getAdvertising()->stop(); }

bool Beacon::isAdvertising() const { return NimBLEDevice::getAdvertising()->isAdvertising(); }

bool Beacon::ensureRunning() {
  if (!configured_) return false;
  if (isAdvertising()) return true;
  return start(payload_);
}

}  // namespace astromech

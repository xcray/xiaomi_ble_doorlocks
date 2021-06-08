#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/esp32_ble_tracker/esp32_ble_tracker.h"
#include "esphome/components/xiaomi_ble/xiaomi_ble.h"

#ifdef ARDUINO_ARCH_ESP32

namespace esphome {
namespace xiaomi_zelkova {

class XiaomiZELKOVA : public Component, public esp32_ble_tracker::ESPBTDeviceListener {
 public:
  void set_address(uint64_t address) { address_ = address; };
  void set_bindkey(const std::string &bindkey);

  bool parse_device(const esp32_ble_tracker::ESPBTDevice &device) override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }
  void set_opmethod(sensor::Sensor *opmethod) { opmethod_ = opmethod; }
  void set_lockattr(sensor::Sensor *lockattr) { lockattr_ = lockattr; }
  void set_battlvl(sensor::Sensor *battlvl) { battlvl_ = battlvl; }
  void set_doorevt(sensor::Sensor *doorevt) { doorevt_ = doorevt; }
  void set_opts(sensor::Sensor *opts) { opts_ = opts; }
  void set_keyid(sensor::Sensor *keyid) { keyid_ = keyid; }
  void set_battlvlts(sensor::Sensor *battlvlts) { battlvlts_ = battlvlts; }
  void set_doorevtts(sensor::Sensor *doorevtts) { doorevtts_ = doorevtts; }

 protected:
  uint64_t address_;
  uint8_t bindkey_[16];
  sensor::Sensor *opmethod_{nullptr};
  sensor::Sensor *lockattr_{nullptr};
  sensor::Sensor *battlvl_{nullptr};
  sensor::Sensor *doorevt_{nullptr};
  sensor::Sensor *opts_{nullptr};
  sensor::Sensor *keyid_{nullptr};
  sensor::Sensor *battlvlts_{nullptr};
  sensor::Sensor *doorevtts_{nullptr};
};

}  // namespace xiaomi_zelkova
}  // namespace esphome

#endif

#include "xiaomi_zelkova.h"
#include "esphome/core/log.h"

#ifdef ARDUINO_ARCH_ESP32

namespace esphome {
namespace xiaomi_zelkova {

static const char *TAG = "xiaomi_zelkova";

void XiaomiZELKOVA::dump_config() {
  ESP_LOGCONFIG(TAG, "Xiaomi ZELKOVA");
  ESP_LOGCONFIG(TAG, "  Bindkey: %s", hexencode(this->bindkey_, 16).c_str());
  LOG_SENSOR("  ", "OpMethod", this->opmethod_);
  LOG_SENSOR("  ", "OpKeyID", this->keyid_);
  LOG_SENSOR("  ", "OpTS", this->opts_);
  LOG_SENSOR("  ", "LockAttr", this->lockattr_);
  LOG_SENSOR("  ", "BattLvl", this->battlvl_);
  LOG_SENSOR("  ", "BattLvlTS", this->battlvlts_);
  LOG_SENSOR("  ", "DoorEvt", this->doorevt_);
  LOG_SENSOR("  ", "DoorEvtTS", this->doorevtts_);
}

bool XiaomiZELKOVA::parse_device(const esp32_ble_tracker::ESPBTDevice &device) {
  if (device.address_uint64() != this->address_) {
    ESP_LOGVV(TAG, "parse_device(): unknown MAC address.");
    return false;
  }
  ESP_LOGVV(TAG, "parse_device(): MAC address %s found.", device.address_str().c_str());

  bool success = false;
  for (auto &service_data : device.get_manufacturer_datas()) {
    auto res = xiaomi_ble::parse_xiaomi_header(service_data);
    if (!res.has_value()) {
      continue;
    }
    if (res->is_duplicate) {
      continue;
    }
    if (res->has_encryption &&
        (!(xiaomi_ble::decrypt_xiaomi_payload(const_cast<std::vector<uint8_t> &>(service_data.data), this->bindkey_,
                                              this->address_)))) {
      continue;
    }
    if (!(xiaomi_ble::parse_xiaomi_message(service_data.data, *res))) {
      continue;
    }
    if (!(xiaomi_ble::report_xiaomi_results(res, device.address_str()))) {
      continue;
    }
    if (res->opmethod.has_value() && this->opmethod_ != nullptr)
      this->opmethod_->publish_state(*res->opmethod);
    if (res->keyid.has_value() && this->keyid_ != nullptr)
      this->keyid_->publish_state(*res->keyid);
    if (res->opts.has_value() && this->opts_ != nullptr)
      this->opts_->publish_state(*res->opts);
    if (res->lockattr.has_value() && this->lockattr_ != nullptr)
      this->lockattr_->publish_state(*res->lockattr);
    if (res->battlvl.has_value() && this->battlvl_ != nullptr)
      this->battlvl_->publish_state(*res->battlvl);
    if (res->battlvlts.has_value() && this->battlvlts_ != nullptr)
      this->battlvlts_->publish_state(*res->battlvlts);
    if (res->doorevt.has_value() && this->doorevt_ != nullptr)
      this->doorevt_->publish_state(*res->doorevt);
    if (res->doorevtts.has_value() && this->doorevtts_ != nullptr)
      this->doorevtts_->publish_state(*res->doorevtts);
    success = true;
  }

  if (!success) {
    return false;
  }

  return true;
}

void XiaomiZELKOVA::set_bindkey(const std::string &bindkey) {
  memset(bindkey_, 0, 16);
  if (bindkey.size() != 32) {
    return;
  }
  char temp[3] = {0};
  for (int i = 0; i < 16; i++) {
    strncpy(temp, &(bindkey.c_str()[i * 2]), 2);
    bindkey_[i] = std::strtoul(temp, NULL, 16);
  }
}

}  // namespace xiaomi_zelkova
}  // namespace esphome

#endif

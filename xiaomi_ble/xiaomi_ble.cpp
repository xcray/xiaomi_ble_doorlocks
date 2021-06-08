#include "xiaomi_ble.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"

#ifdef ARDUINO_ARCH_ESP32

#include <vector>
#include "mbedtls/ccm.h"

namespace esphome {
namespace xiaomi_ble {

static const char *TAG = "xiaomi_ble";

bool parse_xiaomi_value(uint8_t value_type, const uint8_t *data, uint8_t value_length, XiaomiParseResult &result) {
  // 操作方式, 10字节，第二字节含action和method，锁事件5，后面是keyid和时间戳
  if ((value_type == 0x05) && (value_length == 10)) {
    const int8_t opmethod = data[1];
    const int32_t keyid = encode_uint32(data[5], data[4], data[3], data[2]);
    const int32_t opts = encode_uint32(data[9], data[8], data[7], data[6]);
    result.opmethod = opmethod;
    result.keyid = keyid;
    result.opts = opts;
  }
  // 锁属性4110, 1 字节, 只有0、1、2三个取值，代表开着、已关、反锁
  else if ((value_type == 0x0E) && (value_length == 1)) {
    const int8_t lockattr = data[0];
    result.lockattr = lockattr;
  }
  // battery, 5 byte, 8-bit unsigned integer, 1 %，后面为时间戳
  else if ((value_type == 0x0A) && (value_length == 5)) {
    const int32_t battlvlts = encode_uint32(data[4], data[3], data[2], data[1]);
    result.battlvl = data[0];
    result.battlvlts = battlvlts;
  }
  // 门事件7，5字节，首字节仅取值02，代表超时未关，后面为时间戳
  else if ((value_type == 0x07) && (value_length == 5)) {
    const int8_t doorevt = data[0];
    result.doorevt = doorevt;
    const int32_t doorevtts = encode_uint32(data[4], data[3], data[2], data[1]);
    result.doorevtts = doorevtts;
  }
  else {
    return false;
  }

  return true;
}

bool parse_xiaomi_message(const std::vector<uint8_t> &message, XiaomiParseResult &result) {
  result.has_encryption = (message[0] & 0x08) ? true : false;  // update encryption status
  if (result.has_encryption) {
    ESP_LOGD(TAG, "parse_xiaomi_message(): payload is encrypted, stop reading message.");
    return false;
  }

  // Data point specs
  // Byte 0: type
  // Byte 1: fixed 0x10
  // Byte 2: length
  // Byte 3..3+len-1: data point value

  const uint8_t *payload = message.data()+5;// + result.raw_offset;
  uint8_t payload_length = message.size()-12;// - result.raw_offset;
  uint8_t payload_offset = 0;
  bool success = false;

  if (payload_length < 4) {
    ESP_LOGD(TAG, "parse_xiaomi_message(): payload has wrong size (%d)!", payload_length);
    return false;
  }

  /*if (payload[payload_offset + 1] != 0x10) {
    ESP_LOGD(TAG, "parse_xiaomi_message(): fixed byte not found, stop parsing residual data.");
    break;
  } */
  const uint8_t value_length = payload[2];
  ESP_LOGD(TAG, "value_length:%i;payload_length:%i",value_length,payload_length);
  if ((value_length < 1) || (payload_length < (3 + value_length))) {
    ESP_LOGD(TAG, "parse_xiaomi_message(): value has wrong size (%d)!", value_length);
    ESP_LOGD(TAG, "payload[0~3]%02X%02X%02X%02X",payload[0],payload[1],payload[2],payload[3]);
  }

  const uint8_t value_type = payload[0];
  const uint8_t *data = &payload[3];

  if (parse_xiaomi_value(value_type, data, value_length, result))
    success = true;

  //payload_length -= 3 + value_length;
  //payload_offset += 3 + value_length;

  return success;
}

optional<XiaomiParseResult> parse_xiaomi_header(const esp32_ble_tracker::ServiceData &service_data) {
  XiaomiParseResult result;
  if (!service_data.uuid.contains(0x8F, 0x03)) {
    ESP_LOGD(TAG, "parse_xiaomi_header(): no service data UUID magic bytes.");
    return {};
  }
  auto raw = service_data.data;
  result.has_data = (raw[0] & 0x40) ? true : false;
  result.has_capability = (raw[0] & 0x20) ? true : false;
  result.has_encryption = (raw[0] & 0x08) ? true : false;

  if (!result.has_data) {
    ESP_LOGD(TAG, "parse_xiaomi_header(): service data has no DATA flag.");
    return {};
  }

  static uint8_t last_frame_count = 0;
  if (last_frame_count == raw[4]) {
    ESP_LOGD(TAG, "parse_xiaomi_header(): duplicate data packet received (%d).", static_cast<int>(last_frame_count));
    result.is_duplicate = true;
    return {};
  }
  last_frame_count = raw[4];
  result.is_duplicate = false;
  result.raw_offset = result.has_capability ? 12 : 11;

  if ((raw[2] == 0x97) && (raw[3] == 0x01)) {  // 榉树门锁，加密
    result.type = XiaomiParseResult::TYPE_ZELKOVA;
    result.name = "ZELKOVA";
  } else {
    ESP_LOGD(TAG, "parse_xiaomi_header(): unknown device, no magic bytes.");
    return {};
  }

  return result;
}

bool decrypt_xiaomi_payload(std::vector<uint8_t> &raw, const uint8_t *bindkey, const uint64_t &address) {
  /* if (!((raw.size() == 19) || ((raw.size() >= 22) && (raw.size() <= 24)))) {
    ESP_LOGD(TAG, "decrypt_xiaomi_payload(): data packet has wrong size (%d)!", raw.size());
    ESP_LOGD(TAG, "  Packet : %s", hexencode(raw.data(), raw.size()).c_str());
    return false;
  } */

  uint8_t mac_reverse[6] = {0};
  mac_reverse[5] = (uint8_t)(address >> 40);
  mac_reverse[4] = (uint8_t)(address >> 32);
  mac_reverse[3] = (uint8_t)(address >> 24);
  mac_reverse[2] = (uint8_t)(address >> 16);
  mac_reverse[1] = (uint8_t)(address >> 8);
  mac_reverse[0] = (uint8_t)(address >> 0);

  XiaomiAESVector vector{.key = {0},
                         .plaintext = {0},
                         .ciphertext = {0},
                         .authdata = {0x11},
                         .iv = {0},
                         .tag = {0},
                         .keysize = 16,
                         .authsize = 1,
                         .datasize = 0,
                         .tagsize = 4,
                         .ivsize = 12};

  vector.datasize = raw.size() - 12 ;
  int cipher_pos = 5;

  const uint8_t *v = raw.data();

  memcpy(vector.key, bindkey, vector.keysize);
  memcpy(vector.ciphertext, v + cipher_pos, vector.datasize);
  memcpy(vector.tag, v + raw.size() - vector.tagsize, vector.tagsize);
  memcpy(vector.iv, mac_reverse, 6);             // MAC address reverse
  memcpy(vector.iv + 6, v + 2, 3);               // sensor type (2) + packet id (1)
  memcpy(vector.iv + 9, v + raw.size() - 7, 3);  // payload counter

  mbedtls_ccm_context ctx;
  mbedtls_ccm_init(&ctx);

  int ret = mbedtls_ccm_setkey(&ctx, MBEDTLS_CIPHER_ID_AES, vector.key, vector.keysize * 8);
  if (ret) {
    ESP_LOGD(TAG, "decrypt_xiaomi_payload(): mbedtls_ccm_setkey() failed.");
    mbedtls_ccm_free(&ctx);
    return false;
  }

  ret = mbedtls_ccm_auth_decrypt(&ctx, vector.datasize, vector.iv, vector.ivsize, vector.authdata, vector.authsize,
                                 vector.ciphertext, vector.plaintext, vector.tag, vector.tagsize);
  if (ret) {
    uint8_t mac_address[6] = {0};
    memcpy(mac_address, mac_reverse + 5, 1);
    memcpy(mac_address + 1, mac_reverse + 4, 1);
    memcpy(mac_address + 2, mac_reverse + 3, 1);
    memcpy(mac_address + 3, mac_reverse + 2, 1);
    memcpy(mac_address + 4, mac_reverse + 1, 1);
    memcpy(mac_address + 5, mac_reverse, 1);
    ESP_LOGD(TAG, "decrypt_xiaomi_payload(): authenticated decryption failed.");
    ESP_LOGD(TAG, "  MAC address : %s", hexencode(mac_address, 6).c_str());
    ESP_LOGD(TAG, "       Packet : %s", hexencode(raw.data(), raw.size()).c_str());
    ESP_LOGD(TAG, "          Key : %s", hexencode(vector.key, vector.keysize).c_str());
    ESP_LOGD(TAG, "           Iv : %s", hexencode(vector.iv, vector.ivsize).c_str());
    ESP_LOGD(TAG, "       Cipher : %s", hexencode(vector.ciphertext, vector.datasize).c_str());
    ESP_LOGD(TAG, "          Tag : %s", hexencode(vector.tag, vector.tagsize).c_str());
    mbedtls_ccm_free(&ctx);
    return false;
  }

  // replace encrypted payload with plaintext
  uint8_t *p = vector.plaintext;
  for (std::vector<uint8_t>::iterator it = raw.begin() + cipher_pos; it != raw.begin() + cipher_pos + vector.datasize;
       ++it) {
    *it = *(p++);
  }

  // clear encrypted flag
  raw[0] &= ~0x08;

  ESP_LOGD(TAG, "decrypt_xiaomi_payload(): authenticated decryption passed.");
  ESP_LOGD(TAG, "  Plaintext : %s, Packet : %d", hexencode(raw.data() + cipher_pos, vector.datasize).c_str(),
            static_cast<int>(raw[4]));

  mbedtls_ccm_free(&ctx);
  return true;
}

bool report_xiaomi_results(const optional<XiaomiParseResult> &result, const std::string &address) {
  if (!result.has_value()) {
    ESP_LOGD(TAG, "report_xiaomi_results(): no results available.");
    return false;
  }

  ESP_LOGD(TAG, "Got Xiaomi %s (%s):", result->name.c_str(), address.c_str());

  if (result->opmethod.has_value()) {
    ESP_LOGD(TAG, "  OpMethod:%i", *result->opmethod);
  }
  if (result->lockattr.has_value()) {
    ESP_LOGD(TAG, "  LockAttr:%i", *result->lockattr);
  }
  if (result->battlvl.has_value()) {
    ESP_LOGD(TAG, "  BattLevel: %i", *result->battlvl);
  }
  if (result->doorevt.has_value()) {
    ESP_LOGD(TAG, "  DoorEvt: %i", *result->doorevt);
  }
  if (result->opmethod.has_value()) {
    ESP_LOGD(TAG, "  OpTS:%i", *result->opts);
  }
  if (result->keyid.has_value()) {
    ESP_LOGD(TAG, "  KeyID:%i", *result->keyid);
  }
  if (result->battlvlts.has_value()) {
    ESP_LOGD(TAG, "  BattLevelTS: %i", *result->battlvlts);
  }
  if (result->doorevtts.has_value()) {
    ESP_LOGD(TAG, "  DoorEvtTS: %i", *result->doorevtts);
  }

  return true;
}

bool XiaomiListener::parse_device(const esp32_ble_tracker::ESPBTDevice &device) {
  // Previously the message was parsed twice per packet, once by XiaomiListener::parse_device()
  // and then again by the respective device class's parse_device() function. Parsing the header
  // here and then for each device seems to be unneccessary and complicates the duplicate packet filtering.
  // Hence I disabled the call to parse_xiaomi_header() here and the message parsing is done entirely
  // in the respecive device instance. The XiaomiListener class is defined in __init__.py and I was not
  // able to remove it entirely.

  return false;  // with true it's not showing device scans
}

}  // namespace xiaomi_ble
}  // namespace esphome

#endif

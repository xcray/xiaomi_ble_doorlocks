import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor, esp32_ble_tracker
from esphome.const import (
    CONF_BATTERY_LEVEL,
    CONF_MAC_ADDRESS,
    ICON_EMPTY,
    UNIT_PERCENT,
    CONF_ID,
    CONF_BINDKEY,
    DEVICE_CLASS_BATTERY,
)

CODEOWNERS = ["@XCray"]

DEPENDENCIES = ["esp32_ble_tracker"]
AUTO_LOAD = ["xiaomi_ble"]

xiaomi_zelkova_ns = cg.esphome_ns.namespace("xiaomi_zelkova")
XiaomiZELKOVA = xiaomi_zelkova_ns.class_(
    "XiaomiZELKOVA", esp32_ble_tracker.ESPBTDeviceListener, cg.Component
)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(XiaomiZELKOVA),
            cv.Required(CONF_BINDKEY): cv.bind_key,
            cv.Required(CONF_MAC_ADDRESS): cv.mac_address,
            cv.Optional("battlvl"): sensor.sensor_schema(UNIT_PERCENT, ICON_EMPTY, 0, DEVICE_CLASS_BATTERY),
            cv.Optional("opmethod"): sensor.sensor_schema("", ICON_EMPTY, 0, ""),
            cv.Optional("lockattr"): sensor.sensor_schema("", ICON_EMPTY, 0, ""),
            cv.Optional("doorevt"): sensor.sensor_schema("", ICON_EMPTY, 0, ""),
            cv.Optional("opts"): sensor.sensor_schema("", ICON_EMPTY, 0, ""),
            cv.Optional("keyid"): sensor.sensor_schema("", ICON_EMPTY, 0, ""),
            cv.Optional("doorevtts"): sensor.sensor_schema("", ICON_EMPTY, 0, ""),
            cv.Optional("battlvlts"): sensor.sensor_schema("", ICON_EMPTY, 0, ""),

        }
    )
    .extend(esp32_ble_tracker.ESP_BLE_DEVICE_SCHEMA)
    .extend(cv.COMPONENT_SCHEMA)
)


def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    yield cg.register_component(var, config)
    yield esp32_ble_tracker.register_ble_device(var, config)

    cg.add(var.set_address(config[CONF_MAC_ADDRESS].as_hex))
    cg.add(var.set_bindkey(config[CONF_BINDKEY]))

    if "opmethod" in config:
        sens = yield sensor.new_sensor(config["opmethod"])
        cg.add(var.set_opmethod(sens))
    if "lockattr" in config:
        sens = yield sensor.new_sensor(config["lockattr"])
        cg.add(var.set_lockattr(sens))
    if "battlvl" in config:
        sens = yield sensor.new_sensor(config["battlvl"])
        cg.add(var.set_battlvl(sens))
    if "doorevt" in config:
        sens = yield sensor.new_sensor(config["doorevt"])
        cg.add(var.set_doorevt(sens))
    if "opts" in config:
        sens = yield sensor.new_sensor(config["opts"])
        cg.add(var.set_opts(sens))
    if "keyid" in config:
        sens = yield sensor.new_sensor(config["keyid"])
        cg.add(var.set_keyid(sens))
    if "battlvlts" in config:
        sens = yield sensor.new_sensor(config["battlvlts"])
        cg.add(var.set_battlvlts(sens))
    if "doorevtts" in config:
        sens = yield sensor.new_sensor(config["doorevtts"])
        cg.add(var.set_doorevtts(sens))

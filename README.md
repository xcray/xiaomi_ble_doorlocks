# xiaomi_ble_doorlocks
modified esphome code of xiaomi_lywsd03mmc &amp; xiaomi_ble to support the door lock brand Zelkova.

if your lock is a diffrent brand or model, maybe you need to modify the code further.

configuration:
```
esp32_ble_tracker:
  scan_parameters:
   window: 180ms
sensor:
  - platform: xiaomi_zelkova
    mac_address: $mac_of_lock
    bindkey: $beaconkey
    lockattr:
      name: "LockAttr"
    opmethod:
      name: "OpMethod"
    doorevt:
      name: "DoorEvt"
    battlvl:
      name: "BattLvl"
    keyid:
      name: "KeyID"
    opts:
      name: "OpTS"
    doorevtts:
      name: "DoorEvtTS"
    battlvlts:
      name: "BattLvlTS"
```

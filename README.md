# LocoNetMQTTGateway

## Hardware

A loconet interface is required.

`LOCONET_PIN_RX` has to be setup,
so that it's pin state reflects
whether the LocoNet data line is pulled to GND.

`LOCONET_PIN_TX` has to be setup,
so that it pulls the LocoNet data line to GND.

Either pin can be setup to operate in inverted polarity mode.

## Setup

Create file `include/wificredentials.h`:
```cpp
#define WIFI_SSID "your wifi ssid"
#define WIFI_PASS "your wifi password"
```

Create file `include/config.h`:
```cpp
#define MQTT_SERVER "your mqtt server hostname or ip address"
#define LOCONET_PIN_RX 22
#define LOCONET_PIN_TX 23
```

Optionally add
```cpp
#define LOCONET_UART_SIGNAL_INVERT_RX false
#define LOCONET_UART_SIGNAL_INVERT_TX true
```
to invert the LocoNet pins.

## Usage

The Gateway adapts all LocoNet messages into MQTT PUBLISH
and all MQTT SUBSCRIBE into LocoNet messages.

LocoNet Message | MQTT topic | MQTT Direction | Description
-|-|-|-
`OPC_INPUT_REP`|`ln/sensor/<address>`|`PUBLISH`| Send LocoNet sensor report to MQTT
`OPC_INPUT_REP`|`ln/reportSensor/<address>`|`SUBSCRIBE`| Send MQTT sensor report to LocoNet
`OPC_INPUT_REP`|`ln/sensorB/<card address>/<block number>`|`PUBLISH`| Send LocoNet sensor report to MQTT (Blücher GBM16XN mode)
`OPC_INPUT_REP`|`ln/reportSensor/<address>`|`SUBSCRIBE`| Send MQTT sensor report to LocoNet (Blücher GBM16XN mode)
`OPC_SW_REQ`|`ln/switch/<address>`|`PUBLISH`| Send LocoNet switch request to MQTT
`OPC_SW_REQ`|`ln/requestSwitch/<address>`|`SUBSCRIBE`| Send MQTT switch request to LocoNet
`OPC_GPON`/`OPC_GPOFF`|`ln/power`|`PUBLISH`| Send LocoNet Power state to MQTT
`OPC_GPON`/`OPC_GPOFF`|`ln/requestPower`|`SUBSCRIBE`| Send MQTT Power state to LocoNet

## Advanced Usage

You can change the top-level MQTT topic
(default `ln`)
by setting `#define LN_TOPIC "ln"` in `include/config.h`.

If you operate your MQTT broker on a port different that `1883`,
you can set `#define MQTT_PORT 1883` in `include/config.h`
to overwrite it to any port number.

If you do not want MQTT to be able to control the power state,
add `#undef ALLOW_LN_POWER` to `include/config.h`.

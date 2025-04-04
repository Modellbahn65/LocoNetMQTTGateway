
#define ARDUINO_ESP32_MCU_esp32

#define ALLOW_LN_POWER

#include <LocoNetStreamESP32.h>
#include "config.h"

LocoNetBus bus;

#if !defined(LOCONET_PIN_RX) || !defined(LOCONET_PIN_TX)
  #error LocoNet pins not defined
#endif

#ifndef LOCONET_UART_SIGNAL_INVERT_RX
  #define LOCONET_UART_SIGNAL_INVERT_RX false
#endif
#ifndef LOCONET_UART_SIGNAL_INVERT_TX
  #define LOCONET_UART_SIGNAL_INVERT_TX false
#endif

#include <LocoNetStream.h>
LocoNetDispatcher parser(&bus);
LocoNetStreamESP32 lnStream(1,
                            LOCONET_PIN_RX,
                            LOCONET_PIN_TX,
                            LOCONET_UART_SIGNAL_INVERT_RX,
                            LOCONET_UART_SIGNAL_INVERT_TX,
                            &bus);

#include <WiFi.h>
#include "wificredentials.h"

#if !defined(WIFI_SSID) || !defined(WIFI_PASS)
  #error WiFi credentials not defined
#endif

#include <PubSubClient.h>

#ifndef MQTT_SERVER
  #error MQTT_SERVER not defined
#endif

#ifndef MQTT_PORT
  #define MQTT_PORT 1883
#endif

WiFiClient espClient;
PubSubClient client(MQTT_SERVER, MQTT_PORT, espClient);

#ifndef LN_TOPIC
  #define LN_TOPIC "ln"
#endif

// This call-back function is called from LocoNet.processSwitchSensorMessage
// for all Sensor messages
void notifySensor(uint16_t Address, uint8_t State) {
  Serial.print("Sensor: ");
  Serial.print(Address, DEC);
  Serial.print(" - ");
  Serial.println(State ? "Active" : "Inactive");

  String topic = LN_TOPIC "/sensor/";
  topic += Address;
  client.publish(topic.c_str(), State ? "1" : "0");
}

// This call-back function is called from LocoNet.processSwitchSensorMessage
// for all Switch Request messages
void notifySwitchRequest(uint16_t Address, uint8_t Output, uint8_t Direction) {
  Serial.print("Switch Request: ");
  Serial.print(Address, DEC);
  Serial.print(':');
  Serial.print(Direction ? "Closed" : "Thrown");
  Serial.print(" - ");
  Serial.println(Output ? "On" : "Off");

  String topic = LN_TOPIC "/switch/";
  topic += Address;
  topic += '/';
  topic += Direction;
  client.publish(topic.c_str(), Output ? "1" : "0");
}

// This call-back function is called from LocoNet.processSwitchSensorMessage
// for all Power messages
void notifyPower(uint8_t State) {
  Serial.print("Layout Power State: ");
  Serial.println(State ? "On" : "Off");

  String topic = LN_TOPIC "/power";
  client.publish(topic.c_str(), State ? "1" : "0");
}

// This call-back function is called from LocoNet.processSwitchSensorMessage
// for all Sensor messages
void notifySensorB(uint8_t address, uint8_t block, bool State) {
  Serial.printf("SensorB: address=%2d block=%d present=%d\n", address, block,
                State);
  // char msg[100];
  // snprintf(msg, sizeof(msg), "SensorB: address=%2d block=%d present=%d",
  // address, block, State); client.publish(LN_TOPIC + "/debug", msg, false);
  String topic = LN_TOPIC "/sensorB/";
  topic += address;
  topic += '/';
  topic += block;
  client.publish(topic.c_str(), State ? "1" : "0");
}

enum MqttTypes {
  TYPE_UNKNOWN,
  TYPE_REPORTSENSOR,
  TYPE_REPORTSENSORB,
  TYPE_REQUESTSWITCH,
  TYPE_REQUESTPOWER,
};

MqttTypes resolveType(String type) {
  if (type == "reportSensor")
    return TYPE_REPORTSENSOR;
  if (type == "reportSensorB")
    return TYPE_REPORTSENSORB;
  if (type == "requestSwitch")
    return TYPE_REQUESTSWITCH;
  if (type == "requestPower")
    return TYPE_REQUESTPOWER;
  return TYPE_UNKNOWN;
}

void mqttCallback(const char* topic, byte* payload, unsigned int length) {
  String t = topic;
  String p = String(payload, length);
  Serial.printf("Received MQTT message for topic %s: %s\n", t.c_str(),
                p.c_str());

  String topicStripped = t.substring(t.indexOf('/') + 1);

  String type = topicStripped.substring(0, topicStripped.indexOf('/'));
  topicStripped = topicStripped.substring(topicStripped.indexOf('/') + 1);
  switch (resolveType(type)) {
    default:
    case TYPE_UNKNOWN:
      break;
    case TYPE_REPORTSENSOR:
      reportSensor(&bus, topicStripped.toInt(), p.charAt(0) != '0');
      break;
    case TYPE_REPORTSENSORB: {
      int addr = topicStripped.substring(0, topicStripped.indexOf('/')).toInt();
      int block =
          topicStripped.substring(topicStripped.indexOf('/') + 1).toInt();
      reportSensorB(&bus, addr, block, p.charAt(0) != '0');
    } break;
    case TYPE_REQUESTSWITCH: {
      int addr = topicStripped.substring(0, topicStripped.indexOf('/')).toInt();
      int direction =
          topicStripped.substring(topicStripped.indexOf('/') + 1).toInt();
      requestSwitch(&bus, addr, p.charAt(0) != '0', direction);
    } break;
    case TYPE_REQUESTPOWER:
      reportPower(&bus, p.charAt(0) != '0');
      break;
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("LocoNet2 MQTT Relay");

  Serial.printf("Connecting to WiFi network %s\n", WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  if (WiFi.waitForConnectResult() != WL_CONNECTED)
    ESP.restart();
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  Serial.printf("Connecting to MQTT server \"%s\"\n", MQTT_SERVER);
  while (!client.connected())
    client.connect("LocoNet2-MQTT-Relay", "ha", "ha");
  Serial.println("Connected to MQTT server");

  client.setCallback(mqttCallback);
  client.subscribe(LN_TOPIC "/reportSensor/+");
  client.subscribe(LN_TOPIC "/reportSensorB/#");
  client.subscribe(LN_TOPIC "/requestSwitch/#");
#ifdef ALLOW_LN_POWER
  client.subscribe(LN_TOPIC "/requestPower");
#endif

  lnStream.start();

  parser.onSensorChangeB(notifySensorB);
  parser.onSensorChange(notifySensor);
  parser.onSwitchRequest(notifySwitchRequest);
  parser.onPowerChange(notifyPower);
}

uint16_t sensorIndex = 0;

void loop() {
  client.loop();
  lnStream.process();
}

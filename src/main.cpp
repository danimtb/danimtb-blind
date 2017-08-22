#include <map>
#include <cstdint>

#include <ArduinoOTA.h>

#include "../lib/Relay/Relay.h"
#include "../lib/LED/LED.h"
#include "../lib/Button/Button.h"
#include "../lib/DataManager/DataManager.h"
#include "../lib/MqttManager/MqttManager.h"
#include "../lib/SimpleTimer/SimpleTimer.h"
#include "../lib/WifiManager/WifiManager.h"
#include "../lib/WebServer/WebServer.h"
#include "../lib/UpdateManager/UpdateManager.h"



//#################### FIRMWARE DATA ####################

#define FIRMWARE "electroblind"
#define FIRMWARE_VERSION "0.0.1"

//#################### ======= ####################

#define HARDWARE "electrodragon-3b"
#define LED_PIN 16
#define BUTTON_STOP_PIN 14
#define BUTTON_UP_PIN 4
#define BUTTON_DOWN_PIN 5
#define RELAY_UP_PIN 12
#define RELAY_DOWN_PIN 13


UpdateManager updateManager;
DataManager dataManager;
WifiManager wifiManager;
MqttManager mqttManager;
SimpleTimer relayUpTimer;
SimpleTimer relayDownTimer;
Relay relayUp;
Relay relayDown;
Button buttonStop;
Button buttonUp;
Button buttonDown;
LED led;

String wifi_ssid = dataManager.get("wifi_ssid");
String wifi_password = dataManager.get("wifi_password");
String ip = dataManager.get("ip");
String mask = dataManager.get("mask");
String gateway = dataManager.get("gateway");
String ota_server = dataManager.get("ota_server");
String mqtt_server = dataManager.get("mqtt_server");
String mqtt_port = dataManager.get("mqtt_port");
String mqtt_username = dataManager.get("mqtt_username");
String mqtt_password = dataManager.get("mqtt_username");
String device_name = dataManager.get("device_name");
String mqtt_status = dataManager.get("mqtt_status");
String mqtt_command = dataManager.get("mqtt_command");

void blindOpen()
{
    Serial.println("blindOpen()");
    relayDown.off();
    relayUp.on();
    relayUpTimer.start();
}

void blindClose()
{
    Serial.println("blindClose()");
    relayUp.off();
    relayDown.on();
    relayDownTimer.start();
}

void blindStop()
{
    Serial.println("blindStop()");
    relayDown.off();
    relayUp.off();
    mqttManager.publishMQTT(mqtt_status, "STOP");
}

std::vector<std::pair<String, String>> getWebServerData()
{
    std::vector<std::pair<String, String>> webServerData;

    std::pair<String, String> generic_pair;

    generic_pair.first = "wifi_ssid";
    generic_pair.second = wifi_ssid;
    webServerData.push_back(generic_pair);

    generic_pair.first = "wifi_password";
    generic_pair.second = wifi_password;
    webServerData.push_back(generic_pair);

    generic_pair.first = "ip";
    generic_pair.second = ip;
    webServerData.push_back(generic_pair);

    generic_pair.first = "mask";
    generic_pair.second = mask;
    webServerData.push_back(generic_pair);

    generic_pair.first = "gateway";
    generic_pair.second = gateway;
    webServerData.push_back(generic_pair);

    generic_pair.first = "ota_server";
    generic_pair.second = ota_server;
    webServerData.push_back(generic_pair);

    generic_pair.first = "mqtt_server";
    generic_pair.second = mqtt_server;
    webServerData.push_back(generic_pair);

    generic_pair.first = "mqtt_port";
    generic_pair.second = mqtt_port;
    webServerData.push_back(generic_pair);

    generic_pair.first = "mqtt_username";
    generic_pair.second = mqtt_username;
    webServerData.push_back(generic_pair);

    generic_pair.first = "mqtt_password";
    generic_pair.second = mqtt_password;
    webServerData.push_back(generic_pair);

    generic_pair.first = "device_name";
    generic_pair.second = device_name;
    webServerData.push_back(generic_pair);

    generic_pair.first = "mqtt_status";
    generic_pair.second = mqtt_status;
    webServerData.push_back(generic_pair);

    generic_pair.first = "mqtt_command";
    generic_pair.second = mqtt_command;
    webServerData.push_back(generic_pair);

    generic_pair.first = "firmware_version";
    generic_pair.second = FIRMWARE_VERSION;
    webServerData.push_back(generic_pair);

    generic_pair.first = "hardware";
    generic_pair.second = HARDWARE;
    webServerData.push_back(generic_pair);

    return webServerData;
}

void webServerSubmitCallback(std::map<String, String> inputFieldsContent)
{
    //Save config to dataManager
    Serial.println("webServerSubmitCallback()");

    dataManager.set("wifi_ssid", inputFieldsContent["wifi_ssid"]);
    dataManager.set("wifi_password", inputFieldsContent["wifi_password"]);
    dataManager.set("ip", inputFieldsContent["ip"]);
    dataManager.set("mask", inputFieldsContent["mask"]);
    dataManager.set("gateway", inputFieldsContent["gateway"]);
    dataManager.set("ota_server", inputFieldsContent["ota_server"]);
    dataManager.set("mqtt_server", inputFieldsContent["mqtt_server"]);
    dataManager.set("mqtt_port", inputFieldsContent["mqtt_port"]);
    dataManager.set("mqtt_username", inputFieldsContent["mqtt_username"]);
    dataManager.set("mqtt_password", inputFieldsContent["mqtt_password"]);
    dataManager.set("device_name", inputFieldsContent["device_name"]);
    dataManager.set("mqtt_status", inputFieldsContent["mqtt_status"]);
    dataManager.set("mqtt_command", inputFieldsContent["mqtt_command"]);

    ESP.restart(); // Restart device with new config
}

void MQTTcallback(String topicString, String payloadString)
{
    Serial.print("Message arrived from topic [");
    Serial.print(topicString);
    Serial.println("] ");

    if (topicString == mqtt_command)
    {
        if (payloadString == "OPEN")
        {
            blindOpen();
        }
        else if (payloadString == "CLOSE")
        {
            blindClose();
        }
        else if (payloadString == "STOP")
        {
            blindStop();
        }
        else
        {
            Serial.print("MQTT payload unknown: ");
            Serial.println(payloadString.c_str());
        }
    }
    else
    {
        Serial.print("MQTT topic unknown:");
        Serial.println(topicString);
    }
}

void ultraLongPress()
{
    Serial.println("ultraLongPress()");

    if(wifiManager.apModeEnabled())
    {
        WebServer::getInstance().stop();
        wifiManager.destroyApWifi();

        ESP.restart();
    }
    else
    {
        mqttManager.stopConnection();
        wifiManager.createApWifi();
        WebServer::getInstance().start();
    }
}

void setup()
{
    // Init serial comm
    Serial.begin(115200);

    // Configure Timers
    relayUpTimer.setup(RT_ON, 30000);
    relayDownTimer.setup(RT_ON, 30000);

    // Configure Relay
    relayUp.setup(RELAY_UP_PIN, RELAY_HIGH_LVL);
    relayDown.setup(RELAY_DOWN_PIN, RELAY_HIGH_LVL);

    // Configure Buttons
    buttonStop.setup(BUTTON_STOP_PIN, ButtonType::PULLUP);
    buttonStop.setShortPressCallback(blindStop);
    buttonStop.setLongPressCallback(blindStop);
    buttonStop.setUltraLongPressCallback(ultraLongPress);

    buttonUp.setup(BUTTON_UP_PIN, ButtonType::PULLUP);
    buttonUp.setShortPressCallback(blindOpen);
    buttonUp.setLongPressCallback(blindOpen);

    buttonDown.setup(BUTTON_DOWN_PIN, ButtonType::PULLUP);
    buttonDown.setShortPressCallback(blindClose);
    buttonDown.setLongPressCallback(blindClose);

    // Configure LED
    led.setup(LED_PIN, LED_HIGH_LVL);
    led.on();
    delay(300);
    led.off();

    // Configure Wifi
    wifiManager.setup(wifi_ssid, wifi_password, ip, mask, gateway, HARDWARE);
    wifiManager.connectStaWifi();

    // Configure MQTT
    mqttManager.setup(mqtt_server, mqtt_port, mqtt_username, mqtt_password);
    mqttManager.setDeviceData(device_name, HARDWARE, ip, FIRMWARE, FIRMWARE_VERSION);
    mqttManager.addStatusTopic(mqtt_status);
    mqttManager.addSubscribeTopic(mqtt_command);
    mqttManager.setCallback(MQTTcallback);
    mqttManager.startConnection();

    //Configure WebServer
    WebServer::getInstance().setup("/index.html.gz", webServerSubmitCallback);
    WebServer::getInstance().setData(getWebServerData());

    // OTA setup
    ArduinoOTA.setHostname(device_name.c_str());
    ArduinoOTA.begin();

    // UpdateManager setup
    updateManager.setup(ota_server, FIRMWARE, FIRMWARE_VERSION, HARDWARE);
}

void loop()
{
    // Process Buttons events
    buttonStop.loop();
    buttonUp.loop();
    buttonDown.loop();

    // Check Wifi status
    wifiManager.loop();

    // Check MQTT status and OTA Updates
    if (wifiManager.connected())
    {
        mqttManager.loop();
        updateManager.loop();
        ArduinoOTA.handle();
    }

    // Handle WebServer connections
    if(wifiManager.apModeEnabled())
    {
        WebServer::getInstance().loop();
    }

    // LED Status
    if (mqttManager.connected())
    {
        led.on();
    }
    else if(wifiManager.apModeEnabled())
    {
        led.blink(1000);
    }
    else
    {
        led.off();
    }

    // Relay Up Timer control
    if(relayUp.getState())
    {
        if(relayUpTimer.check())
        {
            relayUp.off();
            mqttManager.publishMQTT(mqtt_status, "OPEN");
        }
    }

    // Relay Down Timer control
    if(relayDown.getState())
    {
        if(relayDownTimer.check())
        {
            relayDown.off();
            mqttManager.publishMQTT(mqtt_status, "CLOSE");
        }
    }
}

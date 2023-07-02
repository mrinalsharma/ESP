#ifndef COMMAND_h
#define COMMAND_h
#include "Arduino.h"
#include <ArduinoJson.h>
class Command {
private:
  String deviceType;
  String groupName;
  String deviceName;
  String commandName;
  String commandValue;

public:
  Command();
  void setCommand(String deviceType, String groupName, String deviceName, String commandName, String commandValue) {
    this->deviceType = deviceType;
    this->groupName = groupName;
    this->deviceName = deviceName;
    this->commandName = commandName;
    this->commandValue = commandValue;
  }

  String toJson() {
    const int capacity = JSON_OBJECT_SIZE(8);
    StaticJsonDocument<capacity> doc;

    doc["deviceType"] = deviceType;
    doc["groupName"] = groupName;
    doc["deviceName"] = deviceName;
    doc["commandName"] = commandName;
    doc["commandValue"] = commandValue;
    String output;
    serializeJson(doc, output);
    return output;
  }

  String toJson() {
    const int capacity = JSON_OBJECT_SIZE(8);
    StaticJsonDocument<capacity> doc;

    doc["deviceType"] = deviceType;
    doc["groupName"] = groupName;
    doc["deviceName"] = deviceName;
    doc["commandName"] = commandName;
    doc["commandValue"] = commandValue;
    String output;
    serializeJson(doc, output);
    return output;
  }

  Command* fromJson(char* jsonString) {
    StaticJsonBuffer<200> jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject(jsonString);
    if (!root.success()) {
      Serial.println("parseObject() failed");
      return false;
    }
    Command* newCommand = new Command();
    newCommand->setCommand(String(root["deviceType"]), String(root["groupName"]), String(root["deviceName"]),
                           String(root["commandName"]), String(root["commandValue"]));
    return newCommand;
  }
};
#endif
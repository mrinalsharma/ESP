#include "Event.h"
#include <ArduinoJson.h>

HomeEvent ::HomeEvent() {
}

void HomeEvent::setEvent(String deviceType, String eventName, String deviceState, String groupName, String deviceName) {
  this->deviceType = deviceType;
  this->eventName = eventName;
  this->deviceState = deviceState;
  this->groupName = groupName;
  this->deviceName =  deviceName;
}

String HomeEvent::toJson() {
  const int capacity = JSON_OBJECT_SIZE(8);
  StaticJsonDocument<capacity> doc;

  doc["deviceType"] = deviceType;
  doc["eventName"] = eventName;
  doc["deviceState"] = deviceState;
  doc["groupName"] = groupName;
  doc["deviceName"] = deviceName;
  String output;
  serializeJson(doc, output);
  return output;
}
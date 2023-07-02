#ifndef HomeEvent_h
#define HomeEvent_h
#include "Arduino.h"
class HomeEvent {
private:
  String deviceType;
  String eventName;
  String deviceState;
  String groupName;
  String deviceName;
  
public:
  HomeEvent();
  void setEvent(String deviceType, String eventName, String deviceState, String groupName, String deviceName);
  String toJson();
};
#endif
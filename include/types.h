#ifndef TYPES_H
#define TYPES_H

#include <Arduino.h>
#include <stdint.h>

enum EspNowCommandType : uint8_t {
  ESPNOW_CMD_NONE = 0,
  ESPNOW_CMD_START_HEATER = 1,
  ESPNOW_CMD_STOP_HEATER = 2,
  ESPNOW_CMD_REQUEST_STATUS = 3
};

#pragma pack(push, 1)
struct EspNowCommandPacket {
  uint8_t magic;
  uint8_t version;
  uint8_t command;
  uint32_t unixTimeHint;
};

struct EspNowStatusPacket {
  uint8_t magic;
  uint8_t version;
  uint8_t heatingActive;
  float innenTemperatur;
  float batterieSpannung;
  float batterieLeistung;
  uint16_t restzeitS;
  uint32_t uptimeS;
};
#pragma pack(pop)

struct RemoteState {
  bool statusValid;
  bool heatingActive;
  float innenTemperatur;
  float batterieSpannung;
  float batterieLeistung;
  uint16_t restzeitS;
  uint32_t uptimeS;
  uint32_t lastStatusMs;
  char lastAction[24];
};

#endif

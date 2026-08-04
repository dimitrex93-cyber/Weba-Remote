#include <Arduino.h>
#include <string.h>
#include <esp_sleep.h>
#include <driver/gpio.h>

#include "config.h"
#include "types.h"
#include "espnow_bridge.h"
#include "display_manager.h"

RemoteState remoteState = {false, false, 0.0f, 0.0f, 0.0f, 0, 0, 0, "Boot"};
EspNowBridge espNow;
DisplayManager display;

uint32_t nextDisplayDueMs = 0;
uint32_t buttonDownSinceMs = 0;
uint32_t lastUserActivityMs = 0;
bool buttonWasDown = false;
// Review 04.08.2026: Rate-Limit für den Hold-Poll + periodische Statusabfrage
uint32_t lastStatusRequestMs = 0;
uint32_t lastStatusPollMs = 0;

void handleButton();
void printMacAddress();
void setLastAction(const char* action);
void enterLightSleepIfIdle(uint32_t nowMs);

void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(400);

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  if (!display.initialize()) {
    Serial.println("OLED init failed");
  }

  if (!espNow.initialize()) {
    Serial.println("ESP-NOW init failed");
  } else {
    Serial.println("ESP-NOW ready");
  }

  printMacAddress();

  setLastAction("Ready");
  espNow.sendCommand(ESPNOW_CMD_REQUEST_STATUS);
  lastUserActivityMs = millis();
}

void loop() {
  espNow.copyState(remoteState);
  uint32_t nowMs = millis();

  enterLightSleepIfIdle(nowMs);
  handleButton();

  // Periodische Statusabfrage (Review 04.08.2026): STATUS_POLL_INTERVAL_MS
  // war definiert, wurde aber nie benutzt – Temperatur/Batterie/Restzeit
  // veralteten und das Link-Icon starb nach 12 s.
  if ((uint32_t)(nowMs - lastStatusPollMs) >= STATUS_POLL_INTERVAL_MS) {
    espNow.sendCommand(ESPNOW_CMD_REQUEST_STATUS);
    lastStatusPollMs = nowMs;
  }

  if ((int32_t)(nowMs - nextDisplayDueMs) >= 0) {
    bool linkAlive = remoteState.statusValid &&
                     ((uint32_t)(nowMs - remoteState.lastStatusMs) <= LINK_OK_TIMEOUT_MS);
    display.render(remoteState, linkAlive);
    nextDisplayDueMs = nowMs + 160;
  }
}

void handleButton() {
  bool isDown = (digitalRead(BUTTON_PIN) == LOW);
  uint32_t nowMs = millis();

  if (isDown && !buttonWasDown) {
    buttonDownSinceMs = nowMs;
    lastUserActivityMs = nowMs;
  }

  if (!isDown && buttonWasDown) {
    uint32_t pressedMs = nowMs - buttonDownSinceMs;
    if (pressedMs >= 900) {
      if (espNow.sendCommand(ESPNOW_CMD_STOP_HEATER)) {
        setLastAction("Long press: STOP");
        lastUserActivityMs = nowMs;
      }
    } else {
      if (remoteState.heatingActive) {
        if (espNow.sendCommand(ESPNOW_CMD_STOP_HEATER)) {
          setLastAction("Short press: STOP");
          lastUserActivityMs = nowMs;
        }
      } else {
        if (espNow.sendCommand(ESPNOW_CMD_START_HEATER)) {
          setLastAction("Short press: START");
          lastUserActivityMs = nowMs;
        }
      }
    }
  }

  // Rate-Limit (Review 04.08.2026): vorher feuerte der Hold-Poll über das
  // ganze 400-ms-Fenster mehrfach (Kanal-Last + Akku-Drain). Jetzt max.
  // ein Request pro 400 ms.
  if (isDown && (nowMs - buttonDownSinceMs) > 2500 && (nowMs - lastStatusRequestMs) >= 400) {
    espNow.sendCommand(ESPNOW_CMD_REQUEST_STATUS);
    lastStatusRequestMs = nowMs;
    lastUserActivityMs = nowMs;
  }

  buttonWasDown = isDown;
}

void printMacAddress() {
  uint8_t mac[6] = {0};
  WiFi.macAddress(mac);
  Serial.printf("C3 MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

void setLastAction(const char* action) {
  if (action == nullptr) {
    return;
  }

  strncpy(remoteState.lastAction, action, sizeof(remoteState.lastAction) - 1);
  remoteState.lastAction[sizeof(remoteState.lastAction) - 1] = '\0';
}

void enterLightSleepIfIdle(uint32_t nowMs) {
  if ((uint32_t)(nowMs - lastUserActivityMs) < IDLE_SLEEP_TIMEOUT_MS) {
    return;
  }

  if (digitalRead(BUTTON_PIN) == LOW) {
    return;
  }

  display.sleep();

  gpio_wakeup_enable((gpio_num_t)BUTTON_PIN, GPIO_INTR_LOW_LEVEL);
  esp_sleep_enable_gpio_wakeup();

  // Sleep until button press wakes the remote.
  esp_light_sleep_start();

  display.wake();
  // WiFi-Stack nach dem Light-Sleep wieder hochfahren lassen, bevor der
  // Status-Request rausgeht (Review 04.08.2026).
  delay(150);
  lastUserActivityMs = millis();
  espNow.sendCommand(ESPNOW_CMD_REQUEST_STATUS);
}
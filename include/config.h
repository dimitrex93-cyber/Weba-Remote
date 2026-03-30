#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// QT Py ESP32-C3 pinout defaults.
#define I2C_SDA_PIN 5
#define I2C_SCL_PIN 6
#define BUTTON_PIN 10
#define OLED_ADDR 0x3C

#define ESPNOW_WIFI_CHANNEL 13
#define PACKET_MAGIC 0xA5
#define PACKET_VERSION 1

#define LINK_OK_TIMEOUT_MS 12000
#define STATUS_POLL_INTERVAL_MS 4000
#define IDLE_SLEEP_TIMEOUT_MS 10000

// ESP8266 heater controller MAC: 68:C6:3A:FC:CD:89
#define HEATER_PEER_MAC_0 0x68
#define HEATER_PEER_MAC_1 0xC6
#define HEATER_PEER_MAC_2 0x3A
#define HEATER_PEER_MAC_3 0xFC
#define HEATER_PEER_MAC_4 0xCD
#define HEATER_PEER_MAC_5 0x89

#define SERIAL_BAUD 115200

#endif

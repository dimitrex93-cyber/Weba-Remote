# Weba Remote (ESP32-C3)

Smarte Fernbedienung fuer Weba Thermo W-Link.

Die Firmware laeuft auf einem ESP32-C3 (Adafruit QT Py ESP32-C3) mit 0.96" SSD1306 OLED, zeigt Heizstatus und Sensordaten an und sendet Start/Stop-Befehle per ESP-NOW an den ESP8266-Controller.

## Features

* ESP-NOW Status-Empfang (Heizstatus, Temperatur, Batterie, Restzeit)
* ESP-NOW Befehle (Start, Stop, Status anfordern)
* 10x10 Link-Symbol (WLAN-aehnlich) fuer Funkverbindung
* Light-Sleep nach 10 Sekunden ohne Tasteraktivitaet
* Wake-up ueber Taster

## Hardware

* ESP32-C3: Adafruit QT Py ESP32-C3
* Display: SSD1306 128x64 (I2C, 0.96")
* Taster: gegen GND (interner Pull-up aktiv)

## Pinout

Konfiguration in `include/config.h`:

* `I2C_SDA_PIN` = `5`
* `I2C_SCL_PIN` = `6`
* `BUTTON_PIN` = `10`
* `OLED_ADDR` = `0x3C`

## ESP-NOW Konfiguration

In `include/config.h`:

* `ESPNOW_WIFI_CHANNEL` muss mit dem ESP8266 identisch sein
* `HEATER_PEER_MAC_*` muss die MAC des ESP8266 sein
* `PACKET_MAGIC` und `PACKET_VERSION` muessen auf beiden Seiten gleich bleiben

## Build & Flash

### Build

```bash
pio run --environment adafruit_qtpy_esp32c3
```

### Upload

```bash
pio run --target upload --environment adafruit_qtpy_esp32c3 --upload-port /dev/ttyACM0
```

### Serial Monitor

```bash
pio device monitor -b 115200 --port /dev/ttyACM0
```

## Bedienung

* Kurzer Druck: Start/Stop
* Langer Druck: Stop
* Keine Aktivitaet fuer 10s: Light-Sleep
* Wake aus Light-Sleep: Taster druecken

## Projektstruktur

* `include/config.h`: Pins, MAC, Timing
* `include/types.h`: Paket-/Status-Typen
* `include/espnow_bridge.h`: ESP-NOW Transport
* `include/display_manager.h`: OLED Rendering
* `include/Bitmaps.h`: zentrale Icons/Bitmaps
* `src/main.cpp`: schlanke Orchestrierung

## Release

Dieses Repository ist fuer den initialen gemeinsamen Stand mit ESP8266-Controller als `v1.0.0` vorgesehen.

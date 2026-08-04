#ifndef ESPNOW_BRIDGE_H
#define ESPNOW_BRIDGE_H

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

#include "config.h"
#include "types.h"

class EspNowBridge {
public:
  bool initialize() {
    instance_ = this;
    initialized_ = false;

    WiFi.mode(WIFI_STA);
    // Rückgabewert prüfen (Review 04.08.2026): sonst arbeitet ESP-NOW
    // bei Fehler still auf dem falschen Kanal.
    if (esp_wifi_set_channel(ESPNOW_WIFI_CHANNEL, WIFI_SECOND_CHAN_NONE) != ESP_OK) {
      Serial.println("! Channel set failed");
    }

    if (esp_now_init() != ESP_OK) {
      return false;
    }

    esp_now_register_recv_cb(onRecvStatic);
    esp_now_register_send_cb(onSendStatic);  // ACK-Auswertung (Review 04.08.2026)

    uint8_t mac[6] = {
      HEATER_PEER_MAC_0,
      HEATER_PEER_MAC_1,
      HEATER_PEER_MAC_2,
      HEATER_PEER_MAC_3,
      HEATER_PEER_MAC_4,
      HEATER_PEER_MAC_5
    };

    memcpy(peerMac_, mac, sizeof(peerMac_));

    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, peerMac_, 6);
    peerInfo.channel = ESPNOW_WIFI_CHANNEL;
    peerInfo.encrypt = false;

    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
      return false;
    }

    initialized_ = true;
    return true;
  }

  bool isInitialized() const {
    return initialized_;
  }

  bool sendCommand(EspNowCommandType cmd) {
    if (!initialized_) {
      return false;
    }

    EspNowCommandPacket pkt = {};
    pkt.magic = PACKET_MAGIC;
    pkt.version = PACKET_VERSION;
    pkt.command = static_cast<uint8_t>(cmd);
    pkt.unixTimeHint = 0;

    esp_err_t err = esp_now_send(peerMac_, reinterpret_cast<const uint8_t*>(&pkt), sizeof(pkt));
    if (err != ESP_OK) {
      return false;
    }

    // ESP_OK heißt nur „in die Warteschlange gestellt". Kurz auf die
    // asynchrone Send-Bestätigung warten (max. 100 ms), damit die UI
    // keine „gesendet"-Meldung zeigt, wenn das Paket verloren geht.
    lastSendOk_ = false;
    uint32_t deadline = millis() + 100;
    while (!lastSendOk_ && (int32_t)(millis() - deadline) < 0) {
      delay(1);
    }
    return lastSendOk_;
  }

  bool hasFreshStatus() const {
    return hasStatus_;
  }

  void copyState(RemoteState& out) const {
    // Spinlock gegen den Empfangs-Task (Review 04.08.2026): sonst
    // Misch-Snapshot aus altem und neuem Status möglich.
    portENTER_CRITICAL(&mux_);
    out.statusValid = hasStatus_;
    out.heatingActive = latestStatus_.heatingActive;
    out.innenTemperatur = latestStatus_.innenTemperatur;
    out.batterieSpannung = latestStatus_.batterieSpannung;
    out.batterieLeistung = latestStatus_.batterieLeistung;
    out.restzeitS = latestStatus_.restzeitS;
    out.uptimeS = latestStatus_.uptimeS;
    out.lastStatusMs = lastStatusMs_;
    portEXIT_CRITICAL(&mux_);
  }

private:
  struct CachedStatus {
    bool heatingActive;
    float innenTemperatur;
    float batterieSpannung;
    float batterieLeistung;
    uint16_t restzeitS;
    uint32_t uptimeS;
  };

  static EspNowBridge* instance_;

  static void onRecvStatic(const uint8_t* mac, const uint8_t* data, int len) {
    if (instance_ != nullptr) {
      instance_->onRecv(mac, data, len);
    }
  }

  static void onSendStatic(const uint8_t* mac, esp_now_send_status_t status) {
    (void)mac;
    if (instance_ != nullptr) {
      instance_->lastSendOk_ = (status == ESP_NOW_SEND_SUCCESS);
    }
  }

  void onRecv(const uint8_t* mac, const uint8_t* data, int len) {
    if (len != static_cast<int>(sizeof(EspNowStatusPacket))) {
      return;
    }

    // Quell-MAC-Prüfung (Review 04.08.2026): ohne Check kann jede
    // Station auf dem Kanal Statuspakete fälschen und den Kurzdruck-
    // Toggle in die falsche Richtung lenken.
    if (memcmp(mac, peerMac_, 6) != 0) {
      return;
    }

    EspNowStatusPacket pkt;
    memcpy(&pkt, data, sizeof(pkt));

    if (pkt.magic != PACKET_MAGIC || pkt.version != PACKET_VERSION) {
      return;
    }

    portENTER_CRITICAL(&mux_);
    latestStatus_.heatingActive = (pkt.heatingActive != 0);
    latestStatus_.innenTemperatur = pkt.innenTemperatur;
    latestStatus_.batterieSpannung = pkt.batterieSpannung;
    latestStatus_.batterieLeistung = pkt.batterieLeistung;
    latestStatus_.restzeitS = pkt.restzeitS;
    latestStatus_.uptimeS = pkt.uptimeS;
    lastStatusMs_ = millis();
    hasStatus_ = true;
    portEXIT_CRITICAL(&mux_);
  }

  bool initialized_ = false;
  bool hasStatus_ = false;
  uint8_t peerMac_[6] = {0};
  CachedStatus latestStatus_ = {false, 0.0f, 0.0f, 0.0f, 0, 0};
  uint32_t lastStatusMs_ = 0;
  mutable portMUX_TYPE mux_ = portMUX_INITIALIZER_UNLOCKED;
  volatile bool lastSendOk_ = false;   // Send-Callback-Kontext
};

EspNowBridge* EspNowBridge::instance_ = nullptr;

#endif

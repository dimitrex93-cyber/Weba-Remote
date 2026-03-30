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
    esp_wifi_set_channel(ESPNOW_WIFI_CHANNEL, WIFI_SECOND_CHAN_NONE);

    if (esp_now_init() != ESP_OK) {
      return false;
    }

    esp_now_register_recv_cb(onRecvStatic);

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

    return esp_now_send(peerMac_, reinterpret_cast<const uint8_t*>(&pkt), sizeof(pkt)) == ESP_OK;
  }

  bool hasFreshStatus() const {
    return hasStatus_;
  }

  void copyState(RemoteState& out) const {
    out.statusValid = hasStatus_;
    out.heatingActive = latestStatus_.heatingActive;
    out.innenTemperatur = latestStatus_.innenTemperatur;
    out.batterieSpannung = latestStatus_.batterieSpannung;
    out.batterieLeistung = latestStatus_.batterieLeistung;
    out.restzeitS = latestStatus_.restzeitS;
    out.uptimeS = latestStatus_.uptimeS;
    out.lastStatusMs = lastStatusMs_;
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

  void onRecv(const uint8_t* mac, const uint8_t* data, int len) {
    (void)mac;
    if (len != static_cast<int>(sizeof(EspNowStatusPacket))) {
      return;
    }

    EspNowStatusPacket pkt;
    memcpy(&pkt, data, sizeof(pkt));

    if (pkt.magic != PACKET_MAGIC || pkt.version != PACKET_VERSION) {
      return;
    }

    latestStatus_.heatingActive = (pkt.heatingActive != 0);
    latestStatus_.innenTemperatur = pkt.innenTemperatur;
    latestStatus_.batterieSpannung = pkt.batterieSpannung;
    latestStatus_.batterieLeistung = pkt.batterieLeistung;
    latestStatus_.restzeitS = pkt.restzeitS;
    latestStatus_.uptimeS = pkt.uptimeS;
    lastStatusMs_ = millis();
    hasStatus_ = true;
  }

  bool initialized_ = false;
  bool hasStatus_ = false;
  uint8_t peerMac_[6] = {0};
  CachedStatus latestStatus_ = {false, 0.0f, 0.0f, 0.0f, 0, 0};
  uint32_t lastStatusMs_ = 0;
};

EspNowBridge* EspNowBridge::instance_ = nullptr;

#endif

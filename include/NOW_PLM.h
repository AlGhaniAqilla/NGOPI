#pragma once

const uint8_t broadcastAddr[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

class ESP_NOW_Broadcast_Peer : public ESP_NOW_Peer {
public:
  // Constructor of the class using the broadcast address
  ESP_NOW_Broadcast_Peer(uint8_t channel, wifi_interface_t iface, const uint8_t *lmk) : ESP_NOW_Peer(broadcastAddr, channel, iface, lmk) {}

  // Destructor of the class
  ~ESP_NOW_Broadcast_Peer() {
    remove();
  }

  // Function to properly initialize the ESP-NOW and register the broadcast peer
  bool begin() {
    if (!ESP_NOW.begin()) {
      Serial.println("Failed to initialize ESP-NOW");
      return false;
    }
    return true;
  }

  bool add_peer() {
    if (!add()) {
      Serial.println("Failed to register the broadcast peer");
      return false;
    }
    return true;
  }

  // Function to send a message to all devices within the network
  bool send_message(const uint8_t *data, size_t len) {
    if (!send(data, len)) {
      Serial.println("Failed to broadcast message");
      return false;
    }
    return true;
  }
};

ESP_NOW_Broadcast_Peer broadcast_peer(0, WIFI_IF_STA, nullptr);

// Callback called when an unknown peer sends a message
void msgBroadcastRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len, void *arg) {
    if(data[0] == ping && info->rx_ctrl->rssi > sinyal){
        dataGroup dataIn;
        memcpy(&dataIn, data, len);
        memcpy(strucData.target, dataIn.sender, sizeof(strucData.target));
        sinyal = info->rx_ctrl->rssi > sinyal;

        time_t now; time(&now);
        if(dataIn.epoch > now){
            struct timeval tv;
            tv.tv_sec = dataIn.epoch; tv.tv_usec = 0;
            settimeofday(&tv, NULL);
        }
    }
}

void initNOW(){
    broadcast_peer.begin();
    broadcast_peer.add_peer();
    ESP_NOW.onNewPeer(msgBroadcastRecv, nullptr);
}
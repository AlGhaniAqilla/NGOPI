#pragma once

///////////////////////////// VARIABEL ////////////////////////
IPAddress ipPengirimVHMS;
AsyncUDP vhmsDld; uint16_t dVHMSport = 62026;

HardwareSerial& VHMS = Serial2; // Membuat nama samaran (alias) langsung ke objek HardwareSerial bawaan
uint32_t timer;
uint8_t vhmsData[200];
uint8_t vhmsLen = 0;

void VHMSinit(){
    VHMS.begin(19200, SERIAL_8N1, VHMS_RX, VHMS_TX);
}

void VHMS_READ(){
    while (VHMS.available()){
        vhmsData[vhmsLen++] = VHMS.read();
        timer = millis();
    }

    if(millis() - timer > 200 && vhmsLen > 0){
        vhmsDld.writeTo(vhmsData, vhmsLen, ipPengirimVHMS, dVHMSport);
        vhmsLen = 0;
    }
}
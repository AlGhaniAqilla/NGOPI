#pragma once

//////////////////////////////////// VARIABEL /////////////////////////////////////
AsyncUDP plmUDP; uint16_t PLMport = 62104;



void infoUDP() {
	plmUDP.broadcastTo((uint8_t *)unitID.c_str(), unitID.length(), PLMport);
    plmDwld.broadcastTo((uint8_t *)unitID.c_str(), unitID.length(), dPLMport);
    vhmsDld.broadcastTo((uint8_t *)unitID.c_str(), unitID.length(), dVHMSport);
}

// PLM DOWNLOAD
void DWN_UDP_PLM(){
    if(plmDwld.listen(dPLMport)) {
        plmDwld.onPacket([](AsyncUDPPacket packet) {
            if(!packet.isMulticast() && !packet.isBroadcast()){
                PLM.write(packet.data(), packet.length()); // print data UDP ke Serial VHMS
                ipPengirimPLM = packet.remoteIP();
            }
        });
	}
}

// VHMS DOWNLOAD
void DWN_VHMS_PLM(){
    if(vhmsDld.listen(dVHMSport)) {
        vhmsDld.onPacket([](AsyncUDPPacket packet) {
            if(!packet.isMulticast() && !packet.isBroadcast()){
                VHMS.write(packet.data(), packet.length()); // print data UDP ke Serial VHMS
                ipPengirimVHMS = packet.remoteIP();
            }
        });
	}
}

// PLMrealtime
void UDP_PLM(){
    if(plmUDP.listen(PLMport)) {
        plmUDP.onPacket([](AsyncUDPPacket packet) {
            if(!packet.isMulticast() && !packet.isBroadcast()){
                String dataIn = String((char *)packet.data()).substring(0, packet.length());
                if(dataIn.startsWith("CHANGE: ")){
                    unitID = dataIn.substring(8);

                    preferences.begin("unit", false);
                    preferences.putString("kodeUnit", unitID);
                    preferences.end();
                }

                // jika data beruba JSON
                if(dataIn.startsWith("{\"")){
                    preferences.begin("unit", false);

                    // 1. Parse string menjadi objek cJSON
                    cJSON *root = cJSON_Parse(dataIn.c_str());

                    // 2. Ambil data secara aman berdasarkan KEY menggunakan CaseSensitive
                    cJSON *idUnitJSON = cJSON_GetObjectItemCaseSensitive(root, "idUnit");
                    cJSON *ssidJSON = cJSON_GetObjectItemCaseSensitive(root, "ssid");
                    cJSON *pswdJSON = cJSON_GetObjectItemCaseSensitive(root, "pswd");

                    if (cJSON_IsString(idUnitJSON) && (idUnitJSON->valuestring != NULL)) {
                        unitID = idUnitJSON->valuestring;
                        preferences.putString("kodeUnit", unitID);
                    }

                    if (cJSON_IsString(ssidJSON) && (ssidJSON->valuestring != NULL)) {
                        SSID = ssidJSON->valuestring;
                        preferences.putString("ssid", SSID);
                    }

                    if (cJSON_IsString(pswdJSON) && (pswdJSON->valuestring != NULL)) {
                        PSWD = pswdJSON->valuestring;
                        preferences.putString("pswd", PSWD);
                    }
                    preferences.end();
                    cJSON_Delete(root);
                }
            }
        });
	}
}
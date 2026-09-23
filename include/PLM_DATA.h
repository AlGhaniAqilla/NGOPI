#pragma once

//////////////////////////////////////// VARIABEL ///////////////////////////////////
IPAddress ipPengirimPLM;
AsyncUDP plmDwld; uint16_t dPLMport = 61993;

HardwareSerial& PLM = Serial1; // Membuat nama samaran (alias) langsung ke objek HardwareSerial bawaan
enum FrameState { WAIT_STX, IN_DATA, WAIT_BCC };
FrameState state = WAIT_STX;

// Kontrol Kode Protokol Komatsu PLM
const uint8_t STX = 0x02;
const uint8_t ETX = 0x03;
const uint8_t DLE = 0x10;
const uint8_t ACK = 0x06;
const uint8_t NAK = 0x15;

uint8_t plmData[200];
int plmLen = 0;

bool inFrame = false;
bool isEscaped = false;
uint8_t calculatedBCC = 0;

/////////////////////////////////////////////////////////////////////////////////////////
void PLMinit(){
    PLM.begin(9600, SERIAL_8N1, PLM_RX, PLM_TX);
}

// Helper: Konversi BCD ke Desimal biasa
uint8_t bcdToDec(uint8_t val) {
  return ((val / 16 * 10) + (val % 16));
}

// Helper: Menggabungkan 2 byte (Lower & Upper Position) menjadi 16-bit uint
uint16_t combineBytes(uint8_t lower, uint8_t upper) {
  return (uint16_t)((upper << 8) | lower);
}

// Fungsi menghitung BCC (XOR)
uint8_t computeBCC(uint8_t* data, int len) {
  uint8_t bcc = 0;
  for (int i = 0; i < len; i++) {
    bcc ^= data[i];
  }
  return bcc;
}

// Fungsi mengirim balasan ACK/NAK kembali ke PLM Controller
void sendResponse(uint8_t statusByte) {
  if(plmData[1] == 'M' && plmData[2] == '2'){
    uint8_t respBuffer[4];
    respBuffer[0] = STX;
    respBuffer[1] = statusByte;
    respBuffer[3] = ETX;
    respBuffer[4] = computeBCC(respBuffer, 3);
    
    PLM.write(respBuffer, 4);
  } else {
    uint8_t respBuffer[5];
    respBuffer[0] = STX;
    respBuffer[1] = statusByte;
    respBuffer[2] = 0x31; //'1' (31h)
    respBuffer[3] = ETX;
    respBuffer[4] = computeBCC(respBuffer, 4);
    
    PLM.write(respBuffer, 5);
  }
  delay(10); // Jeda transmisi pendek
}

// PARSING 1: AUTOMATIC TRANSMISSION - TABEL L (39 Bytes)
void parseTableL(uint8_t* data, int len) {
  if (len < 39) return;
  
  uint8_t month  = bcdToDec(data[3]);
  uint8_t day    = bcdToDec(data[4]);
  uint8_t hour   = bcdToDec(data[5]);
  uint8_t minute = bcdToDec(data[6]);
  uint8_t truckID = data[7];
  uint8_t openID  = data[8];
  
  float loadMass            = combineBytes(data[9],  data[10]) / 10.0;
  float emptyTravelTime     = combineBytes(data[11], data[12]) / 10.0;
  float emptyTravelDistance = data[13] / 10.0;
  uint8_t emptyMaxSpeed     = data[14];
  uint8_t emptyAveSpeed     = data[15];
  float emptyStoppageTime   = combineBytes(data[16], data[17]) / 10.0;
  float loadStoppageTime    = combineBytes(data[18], data[19]) / 10.0;
  float loadedTravelTime    = combineBytes(data[20], data[21]) / 10.0;
  float loadedTravelDistance= data[22] / 10.0;
  uint8_t loadedMaxSpeed    = data[23];
  uint8_t loadedAveSpeed    = data[24];
  float loadedStoppageTime  = combineBytes(data[25], data[26]) / 10.0;
  float dumpingTime         = data[27] / 10.0;
  uint8_t limitedSpeed      = data[28];
  uint16_t warningCode      = combineBytes(data[29], data[30]);

   snprintf(strucData.teksDATA, sizeof(strucData.teksDATA), PSTR(
    "\n==================================================\n"
    "         PLM COMPLETED DUMPING DATA (TABEL L)      \n"
    "==================================================\n"
    "Waktu Record        : %02d/%02d jam %02d:%02d\n"
    "Truck ID / Open ID  : %d / %d\n"
    "Total Muatan        : %.1f Metric Ton\n"
    "------------------ KONDISI KOSONG ----------------\n"
    "Waktu Jalan Kosong  : %.1f Menit\n"
    "Jarak Jalan Kosong  : %.1f km\n"
    "Kecepatan Maks/Rata : %d km/h / %d km/h\n"
    "Waktu Berhenti      : %.1f Menit\n"
    "----------------- PROSES LOADING -----------------\n"
    "Waktu Tunggu Muat   : %.1f Menit\n"
    "----------------- KONDISI BERMUATAN --------------\n"
    "Waktu Jalan Isi     : %.1f Menit\n"
    "Jarak Jalan Isi     : %.1f km\n"
    "Kecepatan Maks/Rata : %d km/h / %d km/h\n"
    "Waktu Berhenti Isi  : %.1f Menit\n"
    "----------------- PROSES DUMPING -----------------\n"
    "Waktu Dumping       : %.1f Menit\n"
    "----------------- STATUS / LAINNYA ---------------\n"
    "Speed Limited       : %d km/h\n"
    "Cycle Warning Code  : 0x%04X\n"
    "==================================================\n"
  ), day, month, hour, minute, truckID, openID, loadMass,
     emptyTravelTime, emptyTravelDistance, emptyMaxSpeed, emptyAveSpeed, emptyStoppageTime,
     loadStoppageTime, loadedTravelTime, loadedTravelDistance, loadedMaxSpeed, loadedAveSpeed,
     loadedStoppageTime, dumpingTime, limitedSpeed, warningCode);
  Serial.print(strucData.teksDATA);

  // 1. Buat objek root cJSON kosong
  cJSON *root = cJSON_CreateObject();

  // 2. Masukkan data dari variabel ke objek JSON
  char timeRecord[100] = "time";
  snprintf(timeRecord, sizeof(timeRecord), PSTR("%02d/%02d %02d:%02d"), day, month, hour, minute);

  cJSON_AddStringToObject(root, "recordTime", timeRecord);
  cJSON_AddNumberToObject(root, "Dumping_Payload", loadMass);

  cJSON_AddNumberToObject(root, "emptyTravelTime", emptyTravelTime);
  cJSON_AddNumberToObject(root, "emptyTravelDistance", emptyTravelDistance);
  cJSON_AddNumberToObject(root, "emptyMaxSpeed", emptyMaxSpeed);
  cJSON_AddNumberToObject(root, "emptyAveSpeed", emptyAveSpeed);
  cJSON_AddNumberToObject(root, "emptyStoppageTime", emptyStoppageTime);

  cJSON_AddNumberToObject(root, "loadStoppageTime", loadStoppageTime);

  cJSON_AddNumberToObject(root, "loadedTravelTime", loadedTravelTime);
  cJSON_AddNumberToObject(root, "loadedTravelDistance", loadedTravelDistance);
  cJSON_AddNumberToObject(root, "loadedMaxSpeed", loadedMaxSpeed);
  cJSON_AddNumberToObject(root, "loadedAveSpeed", loadedAveSpeed);
  cJSON_AddNumberToObject(root, "loadedStoppageTime", loadedStoppageTime);

  cJSON_AddNumberToObject(root, "dumpingTime", dumpingTime);

  // 3. Cetak objek JSON menjadi data char utuh (string tanpa spasi/unformatted agar hemat memori)
  char *json_string_sementara = cJSON_PrintUnformatted(root);

  // 4. simpan ke struktur
  strncpy(strucData.teksDATA, json_string_sementara, sizeof(strucData.teksDATA) - 1);
  strucData.teksDATA[sizeof(strucData.teksDATA) - 1] = '\0';  // agar valid karakter terakhir adalah null terminator

  // 5. WAJIB: Bebaskan memori string dan cJSON
  free(json_string_sementara); cJSON_Delete(root);
}

/* PARSING 2: MMS - UNIT DISTINCTION SETTING (Header 'K')
// void parseMmsUnitDistinction(uint8_t* data, int len) {
//   if (len < 2) return;
//   uint8_t unitClassification = data[1];

//   Hasil = "\n--- [PARSING] MMS: UNIT DISTINCTION SETTING ---\n";
//   Hasil += "Fixed Code: " + String((char)data[0]) + "\n";
//   // Mengonversi nilai hex 0x02 ke bentuk teks string HEX
//   Hasil += "Unit Classification: 0x" + String(unitClassification, HEX) + " (Fixed to 02h for PLM2/PLM)\n";
//   Hasil += "-----------------------------------------------------\n";

//   Serial.print(Hasil);
// } 
*/

// PARSING 3: MMS - LOAD MASS & SUSPENSION PRESSURE (Header 'M', Sub '4')
void parseMmsSuspensionAndLoad(uint8_t* data, int len) {
  if (len < 15) return; 
  
  uint16_t rawLoad    = (data[5] << 8) | data[4]; 
  uint16_t rawEstLoad = (data[7] << 8) | data[6]; 
  
  float loadMass    = rawLoad / 10.0;     
  float estLoadMass = rawEstLoad / 10.0;  
  
  float pressFL = ((data[9]  << 8) | data[8])  / 10.0; 
  float pressFR = ((data[11] << 8) | data[10]) / 10.0; 
  float pressRL = ((data[13] << 8) | data[12]) / 10.0; 
  float pressRR = ((data[15] << 8) | data[14]) / 10.0; 

  snprintf(strucData.teksDATA, sizeof(strucData.teksDATA), PSTR(
    "\n==================================================\n"
    " PLM MMS: LOAD MASS & SUSPENSION PRESSURE (M4)\n"
    "==================================================\n"
    "Load Mass (B)          : %.1f Ton\n"
    "Estimated Load Mass (C): %.1f Ton\n"
    "--------------------------------------------------\n"
    "Suspension Pressure FL : %.1f kg/cm2\n"
    "Suspension Pressure FR : %.1f kg/cm2\n"
    "Suspension Pressure RL : %.1f kg/cm2\n"
    "Suspension Pressure RR : %.1f kg/cm2\n"
    "==================================================\n"
  ), loadMass, estLoadMass, pressFL, pressFR, pressRL, pressRR);
  Serial.print(strucData.teksDATA);

  // 1. Buat objek root cJSON kosong
  cJSON *root = cJSON_CreateObject();

  // 2. Masukkan data dari variabel ke objek JSON
  cJSON_AddNumberToObject(root, "Suspension_FL", pressFL);
  cJSON_AddNumberToObject(root, "Suspension_FR", pressFR);
  cJSON_AddNumberToObject(root, "Suspension_RL", pressRL);
  cJSON_AddNumberToObject(root, "Suspension_RR", pressRR);
  cJSON_AddNumberToObject(root, "Bucket_Payload", loadMass);

  // 3. Cetak objek JSON menjadi data char utuh (string tanpa spasi/unformatted agar hemat memori)
  char *json_string_sementara = cJSON_PrintUnformatted(root);

  // 4. simpan ke struktur
  strncpy(strucData.teksDATA, json_string_sementara, sizeof(strucData.teksDATA) - 1);
  strucData.teksDATA[sizeof(strucData.teksDATA) - 1] = '\0';  // agar valid karakter terakhir adalah null terminator

  // 5. WAJIB: Bebaskan memori string dan cJSON
  free(json_string_sementara); cJSON_Delete(root);
}

// PARSING 4: MMS - FINAL LOAD MASS (Header 'P', Sub '4')
void parseMmsFinalLoadMass(uint8_t* data, int len) {
  if (len < 5) return;
  float finalLoad = combineBytes(data[3], data[4]) / 10.0;

  snprintf(strucData.teksDATA, sizeof(strucData.teksDATA), PSTR(
    "\n---------- [PARSING] MMS: FINAL LOAD MASS ----------\n"
    "Final Hauled Load Mass : %.1f Ton\n"
    "-----------------------------------------------------\n"
  ), finalLoad);
  Serial.print(strucData.teksDATA);

  // 1. Buat objek root cJSON kosong
  cJSON *root = cJSON_CreateObject();

  // 2. Masukkan data dari variabel ke objek JSON
  cJSON_AddNumberToObject(root, "Hauled_Payload", finalLoad);

  // 3. Cetak objek JSON menjadi data char utuh (string tanpa spasi/unformatted agar hemat memori)
  char *json_string_sementara = cJSON_PrintUnformatted(root);

  // 4. simpan ke struktur
  strncpy(strucData.teksDATA, json_string_sementara, sizeof(strucData.teksDATA) - 1);
  strucData.teksDATA[sizeof(strucData.teksDATA) - 1] = '\0';  // agar valid karakter terakhir adalah null terminator

  // 5. WAJIB: Bebaskan memori string dan cJSON
  free(json_string_sementara); cJSON_Delete(root);
}

// PARSING 5: MMS - REAL-TIME DATA TRANSMISSION (Header 'M', Sub '2')
void parseMmsRealTimeData(uint8_t* data, int len) {
  if (len < 20) return; 
  
  // float pressFL = combineBytes(data[3], data[4]) / 10.0;
  // float pressFR = combineBytes(data[5], data[6]) / 10.0;
  // float pressRL = combineBytes(data[7], data[8]) / 10.0;
  // float pressRR = combineBytes(data[9], data[10]) / 10.0;
  
  // int16_t rawInc = (int16_t)combineBytes(data[11], data[12]);
  // float inclination = rawInc / 100.0; 
  
  // float rtLoadMass = combineBytes(data[13], data[14]) / 10.0;
  // uint8_t travelSpeed = data[15];
  
  // uint8_t digSignals = data[16];
  // bool nSignal1 = (digSignals & 0x01);
  // bool bodyFloat = (digSignals & 0x02);
  
  uint8_t dumpStatus = data[19];
  const char* dumpStatusStr = "Unknown";
  switch(dumpStatus) {
    case 0: dumpStatusStr = "Engine stopped"; break;        // ubah ke posisi Download
    case 1: dumpStatusStr = "Empty and stopped"; break;     // ubah ke posisi MMS
    case 2: dumpStatusStr = "Empty and traveling"; break;   // ubah ke posisi MMS
    case 3: dumpStatusStr = "Loading"; break;               // ubah ke posisi MMS
    case 4: dumpStatusStr = "Loaded and traveling"; break;  // ubah ke posisi auto
    case 5: dumpStatusStr = "Loaded and stopped"; break;    // ubah ke posisi auto
    case 6: dumpStatusStr = "Unloading"; break;             // ubah ke posisi auto
  }

  // snprintf(strucData.teksDATA, sizeof(strucData.teksDATA), PSTR(
  //   "\n--- [PARSING] MMS: REAL-TIME DATA VALUE ------------\n"
  //   "Suspension FL / FR  : %.1f / %.1f kg/cm2\n"
  //   "Suspension RL / RR  : %.1f / %.1f kg/cm2\n"
  //   "Inclination Angle   : %.2f degree\n"
  //   "Real-time Load Mass : %.1f Ton\n"
  //   "Current Speed       : %d km/h\n"
  //   "Digital Signal      : N-Signal1=%d, BodyFloat=%d\n"
  //   "Dump Truck Status   : [%d] %s\n"
  //   "-----------------------------------------------------\n"
  // ), pressFL, pressFR, pressRL, pressRR, inclination, rtLoadMass,
  //   travelSpeed, nSignal1, bodyFloat, dumpStatus, dumpStatusStr);
  // Serial.print(strucData.teksDATA);
  Serial.println(dumpStatusStr);

  // 1. Buat objek root cJSON kosong
  // cJSON *root = cJSON_CreateObject();

  // // 2. Masukkan data dari variabel ke objek JSON
  // cJSON_AddNumberToObject(root, "Suspension_FL", pressFL);
  // cJSON_AddNumberToObject(root, "Suspension_FR", pressFR);
  // cJSON_AddNumberToObject(root, "Suspension_RL", pressRL);
  // cJSON_AddNumberToObject(root, "Suspension_RR", pressRR);
  // cJSON_AddNumberToObject(root, "RealTime_Payload", rtLoadMass);
  // cJSON_AddNumberToObject(root, "Travel_Speed", travelSpeed);
  // cJSON_AddStringToObject(root, "HD_Status", dumpStatusStr);

  // // 3. Cetak objek JSON menjadi data char utuh (string tanpa spasi/unformatted agar hemat memori)
  // char *json_string_sementara = cJSON_PrintUnformatted(root);

  // // 4. simpan ke struktur
  // strncpy(strucData.teksDATA, json_string_sementara, sizeof(strucData.teksDATA) - 1);
  // strucData.teksDATA[sizeof(strucData.teksDATA) - 1] = '\0';  // agar valid karakter terakhir adalah null terminator

  // // 5. WAJIB: Bebaskan memori string dan cJSON
  // free(json_string_sementara); cJSON_Delete(root);
}

// MAIN PARSER SELECTION GATEWAY
void routeAndParsePayload(uint8_t* data, int len) {
  if (len == 0) return;
  
  char mainHeader = (char)data[0];
  
  if (mainHeader == 'L') {
    parseTableL(data, len);
  }
  else if (mainHeader == 'M') {
    if (len >= 2 && (char)data[1] == '4') {
      parseMmsSuspensionAndLoad(data, len);
    } 
    else if (len >= 2 && (char)data[1] == '2') {
      parseMmsRealTimeData(data, len);
    }
  } 
  else if (mainHeader == 'P') {
    if (len >= 2 && (char)data[1] == '4') {
      parseMmsFinalLoadMass(data, len);
    }
  } 
  else {
    Serial.printf("\n[PLM DOWNLOAD DATA]\n");
    plmDwld.writeTo(plmData, plmLen, ipPengirimPLM, dPLMport);
  }
}


void PLM_READ(){
  while (PLM.available()) {
    uint8_t b = PLM.read();

    if (state == WAIT_STX) {
      if (b == STX) {
        state = IN_DATA;
        plmLen = 0;
        calculatedBCC = STX; 
        plmData[plmLen++] = b;
      }
    } 
    else if (state == IN_DATA) {
      if (isEscaped) {
        calculatedBCC ^= b;
        plmData[plmLen++] = b;
        isEscaped = false;
        continue;
      }

      if (b == DLE) {
        calculatedBCC ^= b;
        isEscaped = true; 
        continue;
      }

      calculatedBCC ^= b;
      plmData[plmLen++] = b;

      // Jika mendeteksi akhir data (ETX), pindah state untuk menunggu 1 byte BCC berikutnya
      if (b == ETX) {
        state = WAIT_BCC;
      }
    } 
    else if (state == WAIT_BCC) {
      // Byte ini dijamin adalah byte setelah ETX (BCC dari pengirim)
      uint8_t receivedBCC = b; 
      plmData[plmLen++] = b;

      if (calculatedBCC == receivedBCC) {
        Serial.println("[BCC] PLM Valid!");
        routeAndParsePayload(&plmData[1], plmLen - 3); // Potong STX & ETX
        sendResponse(ACK); // Membalas data di terima komplit
      } else {
        Serial.printf("[BCC ERROR] Gagal. Hitung: 0x%02X, Terima: 0x%02X\n", calculatedBCC, receivedBCC);
        sendResponse(NAK); // membalas ada kehilangan data
      }

      // Kembali ke state awal mencari STX untuk frame baru
      state = WAIT_STX;
    }
  }
}
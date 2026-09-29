#include <CONFIG.h>
#include <PLM_DATA.h>
#include <VHMS_DATA.h>
#include <UDP_DATA.h>
#include <NOW_PLM.h>
#include <OTA.h>

const char *versi = "1.0.1";

void setup(){
  configInit();
  wifiInit();

  PLMinit();              // Serial PLM
  VHMSinit();             // Serial VHMS

  UDP_PLM();
  DWN_UDP_PLM();
  DWN_VHMS_PLM();

  initNOW();
  broadcastUDP.attach(1, infoUDP);
}

void loop(){
  PLM_READ();     // Cek PLM DATA
  VHMS_READ();    // Cek VHMS DATA

  if(sOnline){
    cekVersi();
  }

  delay(1);
}

// data in by serial
// cek data
// kirim ke espNOW, UDP server

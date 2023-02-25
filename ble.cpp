#define BLE
#ifdef BLE
#include "NimBLEDevice.h"

#include "globals.h"

// Global Define 
#define BLE_SERVICE_NAME "WR S4BL3"           // name of the Bluetooth Service 

#define FitnessMachineService       0x1826
#define FitnessMachineControlPoint  0x2AD9    // Beta Implementation
#define FitnessMachineFeature       0x2ACC    // CX Not implemented yet
#define FitnessMachineStatus        0x2ADA    // Beta Implementation
#define FitnessMachineRowerData     0x2AD1    // CX Main cx implemented

#define BatteryService              0x180F
#define batteryLevel                0x2A19    // Additionnal cx to battery service

#define Device_Information          0x180A

BLEServer *pServer = NULL;
NimBLEService *pRowerService;

BLECharacteristic * pControl_chr;
BLECharacteristic * pData_chr;
BLECharacteristic * pFMfeat_chr;
BLECharacteristic * pFMstat_chr;
BLECharacteristic * pBatt_chr;
BLECharacteristic * pInf24_chr;
BLECharacteristic * pInf25_chr;
BLECharacteristic * pInf26_chr;
BLECharacteristic * pInf27_chr;
BLECharacteristic * pInf28_chr;
BLECharacteristic * pInf29_chr;

BLEAdvertising *pAdvertising;

bool ble_enabled = false;

bool deviceConnected = false;
bool oldDeviceConnected = false;

uint16_t  rowerDataFlagsP1=0b0000011111110;
uint16_t  rowerDataFlagsP2=0b1111100000001;

  // fitnessMachineControlPointId
  // OPCODE     DESCRIPTION                                             PARAMETERS
  // 0x01       Reset                                                   N/A
  // 0x02       Fitness Machine Stopped or Paused by the User
  // 0x03       Fitness Machine Stopped by Safety Key                   N/A
  // 0x04       Fitness Machine Started or Resumed by the User          N/A
  // 0x05       Target Speed Changed
  // 0x06       Target Incline Changed
  // 0x07       Target Resistance Level Changed
  // 0x08       Target Power Changed
  // 0x09       Target Heart Rate Changed
  // 0x0B       Targeted Number of Steps Changed                        New Targeted Number of Steps Value (UINT16, in Steps with a resolution of 1)
  // 0x0C       Targeted Number of Strides Changed                      New Targeted Number of Strides (UINT16, in Stride with a resolution of 1)
  // 0x0D       Targeted Distance Changed                               New Targeted Distance (UINT24, in Meters with a resolution of 1)
  // 0x0E       Targeted Training Time Changed                          New Targeted Training Time (UINT16, in Seconds with a resolution of 1)

/* Handler class for server events */
class ServerCallbacks: public NimBLEServerCallbacks {
  void onConnect(NimBLEServer* pServer, ble_gap_conn_desc* desc) {
    Serial.printf("Client connected:: %s\n", NimBLEAddress(desc->peer_ota_addr).toString().c_str());
    deviceConnected = true;
  };

  void onDisconnect(NimBLEServer* pServer) {
    Serial.printf("Client disconnected\n");
    deviceConnected = false;
  };
};

/** Handler class for characteristic actions */
class CharacteristicCallbacks: public NimBLECharacteristicCallbacks {
  void onRead(NimBLECharacteristic* pCharacteristic){
    Serial.print(pCharacteristic->getUUID().toString().c_str());
    Serial.print(": onRead(), value: ");
    Serial.println(pCharacteristic->getValue().c_str());
  };

  void onWrite(NimBLECharacteristic* pCharacteristic) {
    Serial.print(pCharacteristic->getUUID().toString().c_str());
    Serial.print(": onWrite(), value: ");
    Serial.println(pCharacteristic->getValue().c_str());
  };
};

static CharacteristicCallbacks chrCallbacks;

void setup_BLE(){
  char cRower[8];

  // Initialise the module 
  // Create the BLE Device
  NimBLEDevice::init(BLE_SERVICE_NAME);
  
  // Create the BLE Server
  NimBLEServer *pServer = NimBLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks);

  // Create the BLE Service
  NimBLEService *pRowerService = pServer->createService(NimBLEUUID((uint16_t)FitnessMachineService));
  
  // Add the Fitness Machine Service definition 
  // Add the Fitness Machine Rower Data characteristic 
  pControl_chr = pRowerService->createCharacteristic(BLEUUID((uint16_t)FitnessMachineControlPoint),NIMBLE_PROPERTY::WRITE);
  pFMfeat_chr  = pRowerService->createCharacteristic(BLEUUID((uint16_t)FitnessMachineFeature),NIMBLE_PROPERTY::READ);
  pFMstat_chr  = pRowerService->createCharacteristic(BLEUUID((uint16_t)FitnessMachineStatus),NIMBLE_PROPERTY::NOTIFY);
  pData_chr    = pRowerService->createCharacteristic(BLEUUID((uint16_t)FitnessMachineRowerData),NIMBLE_PROPERTY::NOTIFY);

  pControl_chr->setCallbacks(&chrCallbacks);
  pData_chr->setCallbacks(&chrCallbacks);
  
  NimBLEService *pBattService = pServer->createService(NimBLEUUID((uint16_t)BatteryService));
  pBatt_chr = pBattService->createCharacteristic(BLEUUID((uint16_t)batteryLevel),NIMBLE_PROPERTY::NOTIFY);

  NimBLEService *pDevInfService = pServer->createService(NimBLEUUID((uint16_t)Device_Information));
  pInf24_chr = pDevInfService->createCharacteristic(BLEUUID((uint16_t)0x2A24),NIMBLE_PROPERTY::READ);
  pInf25_chr = pDevInfService->createCharacteristic(BLEUUID((uint16_t)0x2A25),NIMBLE_PROPERTY::READ);
  pInf26_chr = pDevInfService->createCharacteristic(BLEUUID((uint16_t)0x2A26),NIMBLE_PROPERTY::READ);
  pInf27_chr = pDevInfService->createCharacteristic(BLEUUID((uint16_t)0x2A27),NIMBLE_PROPERTY::READ);
  pInf28_chr = pDevInfService->createCharacteristic(BLEUUID((uint16_t)0x2A28),NIMBLE_PROPERTY::READ);
  pInf29_chr = pDevInfService->createCharacteristic(BLEUUID((uint16_t)0x2A29),NIMBLE_PROPERTY::READ);
  
  // Start the services
  pDevInfService->start();
  pRowerService->start();
  pBattService->start();
 
  cRower[0]=0x26; cRower[1]=0x56; cRower[2]=0x00; cRower[3]=0x00; cRower[4]=0x00; cRower[5]=0x00; cRower[6]=0x00; cRower[7]=0x00;
  pFMfeat_chr->setValue((uint8_t*)cRower, 8);

  pInf24_chr->setValue("4"); 
  pInf25_chr->setValue("0000");
  pInf26_chr->setValue("0.30");
  pInf27_chr->setValue("2.2BLE");
  pInf28_chr->setValue("4.3");
  pInf29_chr->setValue("Waterrower");

 //--------------------------------------------------------------------------------------------------------

  // // Start advertising
  // // uint8_t advdata[] { 0x02, 0x01, 0x06, 0x05, 0x02, 0x26, 0x18, 0x0a, 0x18 }; -- looks wrong!
  // //oAdvertisementData.addData(std::string{ 0x02, 0x01, 0x06, 0x04, 0x26, 0x18, 0x0f, 0x18});
  //
  // BLEDevice::startAdvertising();

  NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
  /** Add the services to the advertisment data **/
  pAdvertising->addServiceUUID(pDevInfService->getUUID());
  pAdvertising->addServiceUUID(pRowerService->getUUID());
  pAdvertising->addServiceUUID(pBattService->getUUID());

  pAdvertising->setScanResponse(true);
  pAdvertising->start();

  ble_enabled = true;
}

void stop_BLE() {
  ble_enabled = false;

  delay(500);

  pAdvertising->stop();
  pServer->stopAdvertising();

  delay(500);

  NimBLEDevice::deinit(true);
}

void check_BLE() {
  if (!ble_enabled) return;

  // connecting
  if (deviceConnected && !oldDeviceConnected) {
    // do stuff here on connecting
    oldDeviceConnected = deviceConnected;
    //delay(5000);    
  }

  // disconnecting
  if (!deviceConnected && oldDeviceConnected) {
    //delay(500); // give the bluetooth stack the chance to get things ready
    pServer->startAdvertising(); // restart advertising
    Serial.println("start advertising");
    oldDeviceConnected = deviceConnected;
  }
}

void setCxFitnessStatus(uint8_t data[],int len){
// OPCODE     DESCRIPTION                                             PARAMETERS
// 0x01       Reset                                                   N/A
// 0x02       Fitness Machine Stopped or Paused by the User
// 0x03       Fitness Machine Stopped by Safety Key                   N/A
// 0x04       Fitness Machine Started or Resumed by the User          N/A
// 0x05       Target Speed Changed
// 0x06       Target Incline Changed
// 0x07       Target Resistance Level Changed
// 0x08       Target Power Changed
// 0x09       Target Heart Rate Changed
// 0x0B       Targeted Number of Steps Changed                        New Targeted Number of Steps Value (UINT16, in Steps with a resolution of 1)
// 0x0C       Targeted Number of Strides Changed                      New Targeted Number of Strides (UINT16, in Stride with a resolution of 1)
// 0x0D       Targeted Distance Changed                               New Targeted Distance (UINT24, in Meters with a resolution of 1)
// 0x0E       Targeted Training Time Changed                          New Targeted Training Time (UINT16, in Seconds with a resolution of 1)
  pFMstat_chr->setValue(data, len);
}

void setCxLightRowerData(){

  // This function is a subset of field to be sent in one piece
  // An alternative to the sendBleData()
  uint16_t  rowerDataFlags=
  
   0b0100000111100; //0x7E
  // 0000000000001 - 1   - 0x001 + More Data 0 <!> WARNINNG <!> This Bit is working the opposite way, 0 means field is present, 1 means not present
  // 0000000000010 - 2   - 0x002 + Average Stroke present
  // 0000000000100 - 4   - 0x004 + Total Distance Present
  // 0000000001000 - 8   - 0x008 + Instantaneous Pace present
  // 0000000010000 - 16  - 0x010 + Average Pace Present
  // 0000000100000 - 32  - 0x020 + Instantaneous Power present
  // 0000001000000 - 64  - 0x040 + Average Power present
  // 0000010000000 - 128 - 0x080 - Resistance Level present
  // 0000100000000 - 256 - 0x080 + Expended Energy present
  // 0001000000000 - 512 - 0x080 - Heart Rate present
  // 0010000000000 - 1024- 0x080 - Metabolic Equivalent present
  // 0100000000000 - 2048- 0x080 - Elapsed Time present
  // 1000000000000 - 4096- 0x080 - Remaining Time present

  //  C1  Stroke Rate             uint8     Position    2  + (After the Flag 2bytes)
  //  C1  Stroke Count            uint16    Position    3  +
  //  C2  Average Stroke Rate     uint8     Position    5  +
  //  C3  Total Distance          uint24    Position    6  +
  //  C4  Instantaneous Pace      uint16    Position    9  +
  //  C5  Average Pace            uint16    Position    11 +
  //  C6  Instantaneous Power     sint16    Position    13 +
  //  C7  Average Power           sint16    Position    15 -
  //  C8  Resistance Level        sint16    Position    17 -
  //  C9  Total Energy            uint16    Position    19 +
  //  C9  Energy Per Hour         uint16    Position    21 -
  //  C9  Energy Per Minute       uint8     Position    23 -
  //  C10 Heart Rate              uint8     Position    24 -
  //  C11 Metabolic Equivalent    uint8     Position    25 -
  //  C12 Elapsed Time            uint16    Position    26 -
  //  C13 Remaining Time          uint16    Position    28 -
  int Value_size =0;
  int dist;
  char cRower[17];

  cRower[Value_size++]=rowerDataFlags & 0x000000FF;
  cRower[Value_size++]=(rowerDataFlags & 0x0000FF00) >> 8;
  
  cRower[Value_size++] = (int8_t) (stats.spm*2) & 0x000000FF;  // Bit 1 SPM is reported as 0.5 of this - allows integer to deliver 0.5 accuracy?!
  cRower[Value_size++] =  stats.stroke & 0x000000FF;
  cRower[Value_size++] = (stats.stroke & 0x0000FF00) >> 8;
  
  // cRower[Value_size++] = stats.spm & 0x000000FF;  // Bit 2 
  dist = (int) (stats.distance);

  cRower[Value_size++] =  dist & 0x000000FF; // Bit 3
  cRower[Value_size++] = (dist & 0x0000FF00) >> 8;
  cRower[Value_size++] = (dist & 0x00FF0000) >> 16;
  
  cRower[Value_size++] = stats.split_secs & 0x000000FF; //4
  cRower[Value_size++] = (stats.split_secs & 0x0000FF00) >> 8;
  
  cRower[Value_size++] = stats.asplit_secs & 0x000000FF; //5
  cRower[Value_size++] = (stats.asplit_secs & 0x0000FF00) >> 8;

  cRower[Value_size++] = stats.watts & 0x000000FF; //6
  cRower[Value_size++] = (stats.watts & 0x0000FF00) >> 8;

  // cRower[Value_size++] = stats.averagePower & 0x000000FF;
  // cRower[Value_size++] = (stats.averagePower & 0x0000FF00) >> 8;

  cRower[Value_size++] =  ((int) stats.elapsed) & 0x000000FF;
  cRower[Value_size++] = (((int) stats.elapsed) & 0x0000FF00) >> 8;

  pData_chr->setValue((uint8_t* )cRower, Value_size);
  pData_chr->notify();

}


void send_BLE() {
  if (!ble_enabled) return;

  if (deviceConnected) {        //** Send a value to protopie. The value is in txValue **//
    // setCxRowerData();
    setCxLightRowerData();
    //Serial.printf("core %d\n",xPortGetCoreID());
  } else
    check_BLE();
}

#endif

// void getCxFitnessControlPoint(){
  
//   unsigned char rData[32]; // Read Buffer Reading the specification MAX_SIZE is 1 Opcode & 18 Octets parameter (without potential header)
//   unsigned char wData[32]; // Write Buffer Reading the specification MAX_SIZE is 1 opcode & 17 Octets for parameter
//   int len=0;
  
//   unsigned char statusData[32];
//   char s4buffer[32];
//   char tmp[32];

//   len=gatt.getChar(fitnessMachineControlPointId, rData, 32);
//   if (len>0){
//     // A Message is received from the BLE Client
//     // 1 start getting the opcode
    
    
//     if (rData[0]!=0x80){
//       SerialDebug.print("getCxFitnessControlPoint() Data:");
//       for (int i=0;i<len;i++){
//         sprintf(tmp,"0x%02X;",rData[i]);
//         SerialDebug.print(tmp);
//       }
//       SerialDebug.print("len:");
//       SerialDebug.println(len);
//     }

//     switch(rData[0]){
      
//       case 0x00:        // Take Control Request
//         wData[0]=0x80;  // Response opcode 
//         wData[1]=0x01;  // for success
//         gatt.setChar(fitnessMachineControlPointId, wData, 2);
//         break;

//       case 0x01:        // RESET Command Request
//         wData[0]=0x80;  // Response opcode 
//         wData[1]=0x01;  // for success
//         gatt.setChar(fitnessMachineControlPointId, wData, 2);
        
//         // Send reset command to the WR S4
//         //writeCdcAcm((char*)"RESET");
//         setReset();

//         statusData[0]=0x01;
//         setCxFitnessStatus(statusData,1);

//         break;

//       case 0x07:        // Start / Resume Command Request
//         wData[0]=0x80;  // Response opcode 
//         wData[1]=0x01;  // for success
//         gatt.setChar(fitnessMachineControlPointId, wData, 2);

//         //Send start/resume command to S4
//         break;

//       case 0x08:        // Stop / Pause Command Request
//         wData[0]=0x80;  // Response opcode 
//         wData[1]=0x01;  // for success
//         gatt.setChar(fitnessMachineControlPointId, wData, 2);

//         //Send start/resume command to S4
//         break;

//       case 0x0C:{        
        
//         // set Target Distance follow by a UINT 24 in meter with a resolution of 1 m
//         // It is also recommended that the rowing computer is RESET prior to downloading any workout, a PING after a reset will indicate the rowing computer is ready again for data.
//         // S4 Command W SI+X+YYYY+0x0D0A
        
//         long distance= (rData[1] &  0x000000FF) + ((rData[2] << 8 ) & 0x0000FF00)  +  ((rData[3] << 16) & 0x00FF0000); // LSO..MO (Inversed)
//         SerialDebug.print("distance:");
//         SerialDebug.println(distance);

//         wData[0]=0x80;  // Response opcode 
//         wData[1]=0x01;  // for success
//         gatt.setChar(fitnessMachineControlPointId, wData, 2);
        
//         //writeCdcAcm((char*)"RESET");
//         // TODO Check the need of Reset at this stage
//         setReset();

//         // Assuming the coding is MSO..LSO (Normal)
//         sprintf(s4buffer,"WSI1%04X",distance);
//         SerialDebug.println(s4buffer);
//         writeCdcAcm((char*)s4buffer);

//         statusData[0]=0x0D;
//         statusData[1]=rData[1];
//         statusData[2]=rData[2];
//         statusData[3]=rData[3];

//         setCxFitnessStatus(statusData,4);
//         // Send new workout distance to the S4 
//         // todo add reset
//         break;
//       }

//       case 0x0D:{    
//         // Set Target time follow by a UINT16 in second with a resolution of 1 sec
//         //It is also recommended that the rowing computer is RESET prior to downloading any workout, a PING after a reset will indicate the rowing computer is ready again for data.
//         // S4 Command W SU + YYYY + 0x0D0A
        
//         long duration= (rData[1] &  0x000000FF) + ((rData[2] << 8 ) & 0x0000FF00);  // LSO..MO (Inversed)
        
//         wData[0]=0x80;  // Response opcode 
//         wData[1]=0x01;  // for success
//         gatt.setChar(fitnessMachineControlPointId, wData, 2);

//          //writeCdcAcm((char*)"RESET");
//          // TODO Check the need of Reset at this stage
//          setReset();
        
//         // Assuming the coding is MSO..LSO (Normal)
//         sprintf(s4buffer,"WSU%04X",duration);
//         SerialDebug.println(s4buffer);
//         writeCdcAcm((char*)s4buffer);

//         break;
//       }
//       default:
//         break;

//     }

//   }

// }



// void setCxRowerData(){
//   // Due the size limitation of the message in the BLE Stack of the NRF
//   // the message will be split in 2 parts with the according Bitfield (read the spec :) )
//   // rowerDataFlagsP1=0b0000011111110 = 0xFE
//   // rowerDataFlagsP2=0b1111100000001 = 0x1F01

//   char cRower[19]; // P1 is the biggest part whereas P2 is 13
  
//   // Send the P1 part of the Message
//   cRower[0]=rowerDataFlagsP1 & 0x000000FF;
//   cRower[1]=(rowerDataFlagsP1 & 0x0000FF00) >> 8;
  
//   cRower[2] = (int8_t) stats.spm & 0x000000FF;
  
//   cRower[3] =  stats.stroke & 0x000000FF;
//   cRower[4] = (stats.stroke & 0x0000FF00) >> 8;
  
//   cRower[5] = stats.averageStokeRate & 0x000000FF;
  
//   cRower[6] = stats.totalDistance &  0x000000FF;
//   cRower[7] = (stats.totalDistance & 0x0000FF00) >> 8;
//   cRower[8] = (stats.totalDistance & 0x00FF0000) >> 16;
  
//   cRower[9] = stats.instantaneousPace & 0x000000FF;
//   cRower[10] = (stats.instantaneousPace & 0x0000FF00) >> 8;
  
//   cRower[11] = stats.averagePace & 0x000000FF;
//   cRower[12] = (stats.averagePace & 0x0000FF00) >> 8;

//   cRower[13] = stats.instantaneousPower & 0x000000FF;
//   cRower[14] = (stats.instantaneousPower & 0x0000FF00) >> 8;

//   cRower[15] = stats.averagePower & 0x000000FF;
//   cRower[16] = (stats.averagePower & 0x0000FF00) >> 8;

//   cRower[17] = stats.resistanceLevel & 0x000000FF;
//   cRower[18] = (stats.resistanceLevel & 0x0000FF00) >> 8;

//   pData_chr->setValue((uint8_t* )cRower, 19);
//   pData_chr->notify();

//   // Send the P2 part of the Message
//   cRower[0]= rowerDataFlagsP2 & 0x000000FF;
//   cRower[1]=(rowerDataFlagsP2 & 0x0000FF00) >> 8;
         
//   cRower[2] =  stats.totalEnergy & 0x000000FF;
//   cRower[3] = (stats.totalEnergy & 0x0000FF00) >> 8;

//   cRower[4] =  stats.energyPerHour & 0x000000FF;
//   cRower[5] = (stats.energyPerHour & 0x0000FF00) >> 8;

//   cRower[6] = stats.energyPerMinute & 0x000000FF;

//   cRower[7] = stats.bpm & 0x000000FF;

//   cRower[8] = stats.metabolicEquivalent& 0x000000FF;

//   //stats.elapsedTime=stats.elapsedTimeSec+stats.elapsedTimeMin*60+stats.elapsedTimeHour*3600;
//   cRower[9] =   stats.elapsedTime & 0x000000FF;
//   cRower[10] = (stats.elapsedTime & 0x0000FF00) >> 8;

//   cRower[11] =  stats.remainingTime & 0x000000FF;
//   cRower[12] = (stats.remainingTime & 0x0000FF00) >> 8;
  
//   pData_chr->setValue((uint8_t* )cRower, 13);
//   pData_chr->notify();

// }





// void send_BLE() {
//   if (deviceConnected) {        //** Send a value to protopie. The value is in txValue **//

//     // long sec = 1000 * (millis() - start);
//     // stats.strokeRate = (int)round(spm + old_spm);
//     // stats.strokeCount = strokes;
//     // stats.averageStokeRate = (int)(strokes * 60 * 2 / sec);
//     // stats.totalDistance = meters;
//     // stats.instantaneousPace  = (int)round(500 / Ms); // pace for 500m
//     // float avrMs = meters / sec;
//     // stats.averagePace = (int)round(500 / avrMs);
//     // stats.instantaneousPower = (int)round(2.8 * Ms * Ms * Ms); //https://www.concept2.com/indoor-rowers/training/calculators/watts-calculator
//     // stats.averagePower = (int)round(2.8 * avrMs * avrMs * avrMs);
//     // stats.elapsedTime = sec;

//     // setCxRowerData();
//     setCxLightRowerData();
//     //delay(500); // bluetooth stack will go into congestion, if too many packets are sent
//     // Serial.println("Send data " + String(stats.strokeCount));

//     //Why coxswain need this notify for start???
//     // char cRower[3];
//     // cRower[0]=0x01;
//     // cRower[1]=0x00;
//     // cRower[2]=0x00;
//     // pData_chr->setValue((uint8_t* )cRower, 3);
//     // pData_chr->notify();
//   }
// }

// void check_BLE() {
//   // connecting
//   if (deviceConnected && !oldDeviceConnected) {
//     // do stuff here on connecting
//     oldDeviceConnected = deviceConnected;
//     //delay(5000);    
//   }

//   // disconnecting
//   if (!deviceConnected && oldDeviceConnected) {
//     //delay(500); // give the bluetooth stack the chance to get things ready
//     pServer->startAdvertising(); // restart advertising
//     Serial.println("start advertising");
//     oldDeviceConnected = deviceConnected;
//   }
// }

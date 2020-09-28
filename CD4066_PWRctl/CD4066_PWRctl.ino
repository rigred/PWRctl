 /*
   2x CD4066
   4 input pins per chip
   D16 = PWR Node 0
   D2  = RST Node 0

   D17 = PWR Node 1
   D3  = RST Node 1

   PWR0   A   1      14  Vdd
   PWR0   A   2      13  CA   D16
   RST0   B   3      12  CD   D17
   RST0   B   4      11   D   PWR1
   D2    CB   5      10   D   PWR1
   D3    CC   6      9    C   RST1
        Vss   7      8    C   RST1

   -------------------------

   PWR2   A   1      14  Vdd
   PWR2   A   2      13  CA   D18
   RST2   B   3      12  CD   D19
   RST2   B   4      11   D   PWR3
   D5     CB  5      10   D   PWR3
   D4     CC  6      9    C   RST3
         Vss  7      8    C   RST3

   D18 = PWR Node 2
   D0  = RST Node 2

   D19 = PWR Node 3
   D1  = RST Node 3


   Arduino PWRctrl binary protocol

   Request Packets: start+devid+action+parity

   sxxxyyzz 8bit byte
   [s]  = 1 bit type checkbit (1 = request, 0 = response)
   [xxx] = devID  = 3bit
   [yyy] = action/response = 3bit
   [z]  = parity = 1 bit

   devId codes
   000 = Dev 1
   001 = Dev 2
   010 = Dev 3
   011 = Dev 4
   etc

   action codes
   (0) 000 = Power Off
   (1) 001 = Power On
   (2) 010 = Force Off
   (3) 011 = Reset
   (4) 100 = Status
   (5) 101 = Query Number of Devices (Changes DevID response into number of devices)

   response codes
   (0) 000 = Powered Off
   (1) 001 = Powered On
   (2) 010 = Power On failed
   (3) 011 = Power Off failed
   (4) 100 = Action Success
   (5) 101 = Request Error
   (6) 110 = Parity Error
   (7) 111 = Bad Data received
*/

#define DEBUG true
// Action codes
#define ACT_ON   0
#define ACT_OFF  1
#define ACT_KILL 2
#define ACT_RST  3
#define ACT_STAT 4
#define ACT_QRY  5

// Response codes
#define STAT_OFF      0
#define STAT_ON       1
#define STAT_ON_FAIL  2
#define STAT_OFF_FAIL 3
#define STAT_OK       4
#define STAT_FAIL     5
#define STAT_PARITY   6
#define STAT_DATABAD  7

#define PWR0 16
#define PWR1 17
#define PWR2 18
#define PWR3 19
#define RST0 2
#define RST1 3
#define RST2 5
#define RST3 4

#define LED0 9
#define LED1 10
#define LED2 11
#define LED3 12

#define numDev 4         //Number of Devices this arduino controls
const unsigned int clickSpeed = 125;
const unsigned int holdSpeed = 4000;

byte rxByte = 0;        // rxByte holds the received command.

bool parity_test(byte rxByte) {
  return true;
}

void response(byte devId, byte response) {
  byte rsData = (devId << 4) & B01110000; //get next 4 bits devId byte and shift right 4 places
  rsData = (response << 1) & B00001110; //get the action bits and shift right 1 place
  
  
  if (DEBUG)
    Serial.print("Response Code");

}

bool devStatus(unsigned int devId) {
  return digitalRead(devId);
}



void setup() {
  Serial.begin(9600);   // Open serial port (9600 bauds) to be used for sending byte data
  // pins used for reading PC board PWR LED (Hi/Low) status
  pinMode(LED0, INPUT);
  pinMode(LED1, INPUT);
  pinMode(LED2, INPUT);
  pinMode(LED3, INPUT);
  //outputs used for driving CD4066 control lines of power and reset lines
  pinMode(PWR0, OUTPUT);
  pinMode(PWR1, OUTPUT);
  pinMode(PWR2, OUTPUT);
  pinMode(PWR3, OUTPUT);
  pinMode(RST0, OUTPUT);
  pinMode(RST1, OUTPUT);
  pinMode(RST2, OUTPUT);
  pinMode(RST3, OUTPUT);
  //Drive control lines low to set all switches to open state
  digitalWrite(PWR0, LOW);
  digitalWrite(PWR1, LOW);
  digitalWrite(PWR2, LOW);
  digitalWrite(PWR3, LOW);
  digitalWrite(RST0, LOW);
  digitalWrite(RST1, LOW);
  digitalWrite(RST2, LOW);
  digitalWrite(RST3, LOW);
  Serial.flush();       // Clear receive buffer.
}

//--------------- loop -----------------------------------------------
void loop() {
  if (Serial.available() > 0) {        // Check receive buffer.
    rxByte = Serial.read();            // Save character received.
    Serial.flush();                    // Clear receive buffer.

    //int rxData =(rxByte & B11111110) >> 1
    bool ecc  = (rxByte & B00000001);
    if (DEBUG)
      Serial.print("Data Received");

    if (parity_test(rxByte)) {
      byte type = (rxByte & B10000000) >> 7; //get first bit of byte and shift right 7 places
      byte devId = (rxByte & B01110000) >> 4; //get next 4 bits devId byte and shift right 4 places
      byte actId = (rxByte & B00001110) >> 1; //get the action bits and shift right 1 place

      if (type == HIGH && devId < numDev) {
        if (DEBUG)
          Serial.print("Matched Check");

        const int myPWR[] = {PWR0, PWR1, PWR2, PWR3};
        const int myRST[] = {RST0, RST1, RST3, RST3};
        const int mySTS[] = {LED0, LED1, LED2, LED3};

        switch (actId) {
          case ACT_OFF: // Power Off
            if (devStatus(mySTS[devId]) == true) {
              digitalWrite(myPWR[devId], HIGH);
              delay(clickSpeed);
              digitalWrite(myPWR[devId], LOW);
            }
            delay(clickSpeed);
            if (devStatus(mySTS[devId])) {
              response(devId, STAT_OFF_FAIL);
            } else {
              response(devId, STAT_OFF);
            }
            if (DEBUG)
              Serial.print("Power Off");
            break;

          case ACT_ON: // Power On
            Serial.write("Power ON");
            if (devStatus(mySTS[devId]) == false) {
              digitalWrite(myPWR[devId], HIGH);
              delay(clickSpeed);
              digitalWrite(myPWR[devId], LOW);
            }
            delay(clickSpeed);
            if (devStatus(mySTS[devId])) {
              response(devId, STAT_ON);
            } else {
              response(devId, STAT_ON_FAIL);
            }
            if (DEBUG)
              Serial.print("Power On");
            break;

          case ACT_KILL:
            // Force Off
            if (devStatus(mySTS[devId]) == false) {
              digitalWrite(myPWR[devId], HIGH);
              delay(holdSpeed);
              digitalWrite(myPWR[devId], LOW);
            }
            delay(clickSpeed);
            if (devStatus(mySTS[devId])) {
              response(devId, STAT_OFF_FAIL);
            } else {
              response(devId, STAT_OFF);
            }
            if (DEBUG)
              Serial.print("Power Force Off");

            break;

          case ACT_RST: // Reset
            if (devStatus(mySTS[devId]) == true) {
              digitalWrite(myRST[devId], HIGH);
              delay(clickSpeed);
              digitalWrite(myRST[devId], LOW);
            }
            response(devId, STAT_OK);
            if (DEBUG)
              Serial.print("Reset");
            break;

          case ACT_STAT: //Status
            response(devId, devStatus(mySTS[devId])); // Return the Status of the LED input line (Lo/Hi) automatically encoded to 0=off/1=On
            if (DEBUG)
              Serial.print("Status");
            break;

          case ACT_QRY:
            response(numDev, STAT_OK);

          default:
            response(devId, STAT_FAIL);
            if (DEBUG)
              Serial.print("Error");

        }

      } else {
        response(devId, STAT_DATABAD);
        if (DEBUG)
          Serial.print("Failed Check");
      }



    } else {
      response(0, STAT_PARITY);
      if (DEBUG)
        Serial.print("Parity Error");
    }
  }
}
// End of the Sketch.
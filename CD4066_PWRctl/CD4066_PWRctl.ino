/*
  2x CD4066
  4 input pins per chip
  D16 = PWR Node 0
  D2  = RST Node 0

  D17 = PWR Node 1
  D3  = RST Node 1

  PWR0   A   1      14  Vdd
  PWR0   A   2      13  CA   D6
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
*/
// Action codes
#define ACT_OFF  0 // 000 = Power Off
#define ACT_ON   1 // 001 = Power On
#define ACT_KILL 2 // 010 = Force Off
#define ACT_RST  3 // 011 = Reset
#define ACT_STAT 4 // 100 = Status
#define ACT_QRY  5 // 101 = Query Number of Devices (Changes response code into number of devices)

// Response codes
#define STAT_OFF      0 // 000 = Powered Off
#define STAT_ON       1 // 001 = Powered On
#define STAT_ON_FAIL  2 // 010 = Power On failed
#define STAT_OFF_FAIL 3 // 011 = Power Off failed
#define STAT_OK       4 // 100 = Action Success
#define STAT_FAIL     5 // 101 = Request Error
#define STAT_PARITY   6 // 110 = Parity Error
#define STAT_DATABAD  7 // 111 = Bad Data received

#define PWR0 6
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

#define numDev 4         //Number of Devices this arduino controls (max 8)
byte myPWR[numDev] = {PWR0, PWR1, PWR2, PWR3};
byte myRST[numDev] = {RST0, RST1, RST3, RST3};
byte mySTS[numDev] = {LED0, LED1, LED2, LED3};
const unsigned int clickSpeed = 125;
const unsigned int holdSpeed = 4000;

byte rxByte = 0;        // rxByte holds the received command.

bool parity_test(byte rxByte) {
  return bool((rxByte & 0b10000000) >> 7) && bool(rxByte & 0b00000001);
}

void response(byte devId, byte respId) {
  byte sbit = 1 << 7;
  devId = (devId << 4) & 0b01110000; //get next 4 bits devId byte and shift left 4 places
  respId = (respId << 1) & 0b00001110; //get the action bits and shift left 1 place

  byte ebit = 1;

  byte message = byte(sbit | devId | respId | ebit);

  Serial.write(message);
  Serial.flush();

}

int devStatus(int devId) {
  return digitalRead(devId);
}

void setup() {
  Serial.begin(115200);   // Open serial port (9600 bauds) to be used for sending byte data
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
    rxByte = Serial.read();
    Serial.flush();

    if (parity_test(rxByte)) {
      byte type = (rxByte & 0b10000000) >> 7; //get first bit of byte and shift right 7 places
      byte devId = (rxByte & 0b01110000) >> 4; //get next 4 bits devId byte and shift right 4 places
      byte actId = (rxByte & 0b00001110) >> 1; //get the action bits and shift right 1 place

      if (type == HIGH && devId < numDev && actId < 6) {

        switch (actId) {
          case ACT_OFF: {// Power Off
              if (devStatus(mySTS[devId]) == HIGH) {
                digitalWrite(myPWR[devId], HIGH);
                delay(clickSpeed);
                digitalWrite(myPWR[devId], LOW);
              }
              if (devStatus(mySTS[devId]) == LOW) {
                response(devId, STAT_OFF_FAIL);
              } else {
                response(devId, STAT_OFF);
              }
              break;
            }

          case ACT_ON: { // Power On
              if (devStatus(int(mySTS[devId])) == LOW) {
                digitalWrite(myPWR[devId], HIGH);
                delay(clickSpeed);
                digitalWrite(myPWR[devId], LOW);
              }
              if (devStatus(mySTS[devId]) == HIGH) {
                response(devId, STAT_ON);
              } else {
                response(devId, STAT_ON_FAIL);
              }
              break;
            }

          case ACT_KILL: {
              // Force Off
              if (devStatus(mySTS[devId]) == false) {
                digitalWrite(myPWR[devId], HIGH);
                delay(holdSpeed);
                digitalWrite(myPWR[devId], LOW);
              }
              if (devStatus(mySTS[devId])) {
                response(devId, STAT_OFF_FAIL);
              } else {
                response(devId, STAT_OFF);
              }
              break;
            }

          case ACT_RST: { // Reset
              if (devStatus(mySTS[devId]) == true) {
                digitalWrite(myRST[devId], HIGH);
                delay(clickSpeed);
                digitalWrite(myRST[devId], LOW);
                response(devId, STAT_OK);
              }
              
              break;
            }

          case ACT_STAT: { //Status
              response(devId, devStatus(mySTS[devId])); // Return the Status of the LED input line (Lo/Hi) automatically encoded to 0=off/1=On
              break;
            }

          case ACT_QRY: {
              response(0, numDev);
              break;
            }

          default: {
              response(devId, STAT_FAIL);
            }

        }

      } else {
        response(devId, STAT_DATABAD);
      }


    } else {
      response(0, STAT_PARITY);
    }
  }
}
// End of the Sketch.

//-----------------------------------------------------------------------------------------------------------//
//                                                                                                           //
//  Slave_ELEC1601_Student_2019_v3                                                                           //
//  The Instructor version of this code is identical to this version EXCEPT it also sets PIN codes           //
//  20191008 Peter Jones                                                                                     //
//                                                                                                           //
//  Bi-directional passing of serial inputs via Bluetooth                                                    //
//  Note: the void loop() contents differ from "capitalise and return" code                                  //
//                                                                                                           //
//  This version was initially based on the 2011 Steve Chang code but has been substantially revised         //
//  and heavily documented throughout.                                                                       //
//                                                                                                           //
//  20190927 Ross Hutton                                                                                     //
//  Identified that opening the Arduino IDE Serial Monitor asserts a DTR signal which resets the Arduino,    //
//  causing it to re-execute the full connection setup routine. If this reset happens on the Slave system,   //
//  re-running the setup routine appears to drop the connection. The Master is unaware of this loss and      //
//  makes no attempt to re-connect. Code has been added to check if the Bluetooth connection remains         //
//  established and, if so, the setup process is bypassed.                                                   //
//                                                                                                           //
//-----------------------------------------------------------------------------------------------------------//

#i
clude <SoftwareSerial.h>   //Software Serial Port
#include <Servo.h>

Servo sL;
Servo sR;

#define RxD 7
#define TxD 6
#define ConnStatus A1

#define DEBUG_ENABLED  1

// ##################################################################################
// ### EDIT THE LINES BELOW TO MATCH YOUR SHIELD NUMBER AND CONNECTION PIN OPTION ###
// ##################################################################################

int shieldPairNumber = 12;

// CAUTION: If ConnStatusSupported = true you MUST NOT use pin A1 otherwise "random" reboots will occur
// CAUTION: If ConnStatusSupported = true you MUST set the PIO[1] switch to A1 (not NC)

boolean ConnStatusSupported = true;   // Set to "true" when digital connection status is available on Arduino pin

// #######################################################

// The following two string variable are used to simplify adaptation of code to different shield pairs

String slaveNameCmd = "\r\n+STNA=Slave";   // This is concatenated with shieldPairNumber later

SoftwareSerial blueToothSerial(RxD,TxD);


void setup()
{
    Serial.begin(9600);
    blueToothSerial.begin(38400);                    // Set Bluetooth module to default baud rate 38400
    pinMode(10, INPUT); pinMode(9, OUTPUT); // LHS IR
    pinMode(3, INPUT); pinMode(2, OUTPUT); // RHS IR 
    pinMode(RxD, INPUT); // 7
    pinMode(TxD, OUTPUT); // 6
    pinMode(ConnStatus, INPUT);

    //  Check whether Master and Slave are already connected by polling the ConnStatus pin (A1 on SeeedStudio v1 shield)
    //  This prevents running the full connection setup routine if not necessary.

    if(ConnStatusSupported) Serial.println("Checking Slave-Master connection status.");

    if(ConnStatusSupported && digitalRead(ConnStatus)==1)
    {
        Serial.println("Already connected to Master - remove USB cable if reboot of Master Bluetooth required.");
    }
    else
    {
        Serial.println("Not connected to Master.");
        
        setupBlueToothConnection();   // Set up the local (slave) Bluetooth module

        delay(1000);                  // Wait one second and flush the serial buffers
        Serial.flush();
        blueToothSerial.flush();
    }
}


void loop()
{
    char recvChar;

    while(1)
    {
        if(blueToothSerial.available())   // Check if there's any data sent from the remote Bluetooth shield
        {
            String allMoves = "";
            recvChar = blueToothSerial.read();
            Serial.print(recvChar); // print to local terminal
            // blueToothSerial.print(recvChar);
            sR.attach(12);
            sL.attach(13);
            delay(50);
              if (recvChar == 'w') {
                // move fwd
                allMoves += recvChar;
                sL.writeMicroseconds(1700);
                sR.writeMicroseconds(1300);
              }
              else if (recvChar == 's') {
                // move bwd
                allMoves += recvChar;
                sL.writeMicroseconds(1300);
                sR.writeMicroseconds(1700);
              } else if (recvChar == 'd') {
                // spin right
                sL.writeMicroseconds(1700);
                sR.writeMicroseconds(1700);
                allMoves += recvChar;
              } else if (recvChar == 'a') {
                // spin left
                sL.writeMicroseconds(1300);
                sR.writeMicroseconds(1300);
                allMoves += recvChar;
              } else if (recvChar == 'q') {
                // initiate IR sensor autonav
                Serial.println("Ir nav");
                irNav();
              } else if (recvChar == 'm') {
                  backwards(allMoves);
              }
              delay(50);
              sL.detach();
              sR.detach();
        }

        if(Serial.available())            // Check if there's any data sent from the local serial terminal. You can add the other applications here.
        {
            recvChar  = Serial.read();
            Serial.print(recvChar);
            blueToothSerial.print(recvChar);
        }
    }
}
  

void setupBlueToothConnection()
{
    Serial.println("Setting up the local (slave) Bluetooth module.");

    slaveNameCmd += shieldPairNumber;
    slaveNameCmd += "\r\n";

    blueToothSerial.print("\r\n+STWMOD=0\r\n");      // Set the Bluetooth to work in slave mode
    blueToothSerial.print(slaveNameCmd);             // Set the Bluetooth name using slaveNameCmd
    blueToothSerial.print("\r\n+STAUTO=0\r\n");      // Auto-connection should be forbidden here
    blueToothSerial.print("\r\n+STOAUT=1\r\n");      // Permit paired device to connect me
    
    //  print() sets up a transmit/outgoing buffer for the string which is then transmitted via interrupts one character at a time.
    //  This allows the program to keep running, with the transmitting happening in the background.
    //  Serial.flush() does not empty this buffer, instead it pauses the program until all Serial.print()ing is done.
    //  This is useful if there is critical timing mixed in with Serial.print()s.
    //  To clear an "incoming" serial buffer, use while(Serial.available()){Serial.read();}

    blueToothSerial.flush();
    delay(2000);                                     // This delay is required

    blueToothSerial.print("\r\n+INQ=1\r\n");         // Make the slave Bluetooth inquirable
    
    blueToothSerial.flush();
    delay(2000);                                     // This delay is required
    
    Serial.println("The slave bluetooth is inquirable!");
}

void irNav() {
  // Serial.println("ir Nav called");
  while (1) {
    int readingLeft = irDetect(9, 10, 38000);
    int readingRight = irDetect(2, 3, 38000);
    if (readingLeft == 0) {
      // spin right
      sL.writeMicroseconds(1700);
      sR.writeMicroseconds(1700);
    } else if (readingRight == 0) {
      // spin left
      sL.writeMicroseconds(1300);
      sR.writeMicroseconds(1300);
    }
    else if (readingRight == 0 && readingLeft == 0) {
      // pick up ball
      // reverse (for now)
      sL.writeMicroseconds(1300);
      sR.writeMicroseconds(1700);
    }
    else if (readingRight == 1 && readingLeft == 1) {
      // move fwd
      sL.writeMicroseconds(1700);
      sR.writeMicroseconds(1300);
    }
  }
}

int irDetect(int irLedPin, int irRecvPin, long freq) {
  tone(irLedPin, freq, 8);
  delay(1);
  int ir = digitalRead(irRecvPin);
  delay(1);
  return ir;
}

void backwards(String allMoves) {
    for (int i = 0; i < allMoves.length(); i++) {
        char currentMove = allMoves.charAt(i);
        if (currentMove == 'd') {
            sR.write(1300);
            sL.write(1300);
        } else if (currentMove == 'a') {
            sR.write(1700);
            sL.write(1700);
        } else if (currentMove == 'w') {
            sL.write(1700);
            sR.write(1300);
        }
    }
}



// include the library
#include <RadioLib.h>
#include "Arduino_LED_Matrix.h"

ArduinoLEDMatrix matrix;

// RF69 radio = new Module(CS_Pin, DIO0, RESET);
RF69 radio = new Module(10, 2, 6);

#include "HX711.h"

HX711 scale;

uint8_t dataPin = 3;
uint8_t clockPin = 7;




// save transmission state between loops
int transmissionState = RADIOLIB_ERR_NONE;

// flag to indicate that a packet was sent
volatile bool transmittedFlag = false;

// this function is called when a complete packet
// is transmitted by the module
// IMPORTANT: this function MUST be 'void' type
//            and MUST NOT have any arguments!
#if defined(ESP8266) || defined(ESP32)
  ICACHE_RAM_ATTR
#endif
void setFlag(void) {
  // we sent a packet, set the flag
  transmittedFlag = true;
}

unsigned long happyframe[] = { // Hex code for a happy face to be displayed
  0x3184a444,
  0x42081100,
  0xa0040000
};
unsigned long sadframe[] = { // Hex code for a nothing to be displayed
  0x0,
  0x0,
  0x0
};


void setup() {
  Serial.begin(9600); // Standard Arduino Setup
  matrix.begin(); // LED Matrix Start

  
  // ---- LORA SETUP ------
  // initialize RF69 with default settings
  Serial.print(F("[RF69] Initializing ... "));
  int state = radio.begin();
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true) { delay(10); }
  }

  // set the function that will be called
  // when packet transmission is finished
  radio.setPacketSentAction(setFlag);
  
    Serial.print(F("[RF69] Setting high power module ... "));
    //state = radio.setOutputPower(20, true);
    state = radio.setOutputPower(-2,true);
    if (state == RADIOLIB_ERR_NONE) {
      Serial.println(F("success!"));
    } else {
      Serial.print(F("failed, code "));
      Serial.println(state);
      while (true) { delay(10); }
    }
  

  // start transmitting the first packet
  //Serial.print(F("[RF69] Sending first packet ... "));
  // you can transmit C-string or Arduino string up to
  // 64 characters long
  //transmissionState = radio.startTransmit("Hello World!");


  // ----- END LORA SETUP -----
  

  Serial.println("LORA setup Finished");
  // ----- BEGIN SCALE SETUP -----
  scale.begin(dataPin, clockPin); // start the scale

  //calibrate();
  // On the LoRa Power Line
  //scale.set_offset(-14255);  
  //scale.set_scale(13.961957);

  // On its own power line:
  //scale.set_offset(-2850); // set offset (y intercept)
  //scale.set_scale(16.140833); // set slope

  //scale.set_offset(-3292);
  //scale.set_scale(16.259150);

  //scale.set_offset(-3293);
  //scale.set_scale(16.133404);
  
  scale.set_offset(-3135); 
  scale.set_scale(16.398724);

  scale.tare(); // assume no load at start


  // ----- END SCALE SETUP -----
  Serial.println("Scale Setup Finished");


  Serial.println("Setup Finished");
}

// counter to keep track of transmitted packets
int count = 0;

int cycle = 0;

int state = 0;

float LC_Reading = 0;

float ground_calibration = 0;

void loop() {
  if (state == 0) {
    ground_calibration = ground_calibration+scale.get_units(5);
    //Serial.println(ground_calibration);
    transmit_something("Calibrating");
    if(cycle == 10){
      
      ground_calibration = ground_calibration/11;
      //ground_calibration = 0;
      //Serial.println("State Translating from 0 to 1");
      //Serial.println("With ground_calibration = "+String(ground_calibration));

      //scale.tare(20);
      //int32_t offset = scale.get_offset();
      //scale.set_offset((offset));
      state ++;
      //delay(999999999999);
    }
    cycle++;

  }
  if (state == 1 && transmittedFlag) {
    // ----- READ THE LOAD CELL -----
    //LC_Reading = scale.get_units(5)-ground_calibration;
    LC_Reading = scale.get_units(5);
    Serial.println("LC Reading without Ground Calibration: "+String(LC_Reading)+" Ground Calibration "+String(ground_calibration) + " With Ground Calibration " + String(LC_Reading-ground_calibration));
    //Serial.println("Ground Calibration: "+String(ground_calibration));

    // ----- END READ LOAD CELL -----

    // ------ FORMAT STRING -----
    int temp = cast_INT(LC_Reading-ground_calibration);
    String str =  String(count++)+","+String(temp);

    // ----- END FORMAT STRING
    transmit_something(str);
    

  }
  
  
}
int transmit_something(String str){
  //radio.finishTransmit();
  transmissionState = radio.startTransmit(str);
  delay(1000);
  radio.finishTransmit();
  
  
}
int transmit_something_old(String str){
  // return(0);
  
  // reset flag
  transmittedFlag = false;
  // clean up after transmission is finished
  radio.finishTransmit();

  matrix.loadFrame(happyframe);
  delay(1000);
  matrix.loadFrame(sadframe);
  // send another one
  transmissionState = radio.startTransmit(str);
  
}
int cast_INT(float val)
{
  return val;
}


void calibrate()
{
  Serial.println("\n\nCALIBRATION\n===========");
  Serial.println("remove all weight from the loadcell");
  //  flush Serial input
  while (Serial.available()) Serial.read();

  Serial.println("and press enter\n");
  while (Serial.available() == 0);

  Serial.println("Determine zero weight offset");
  //  average 20 measurements.
  scale.tare(20);
  int32_t offset = scale.get_offset();

  Serial.print("OFFSET: ");
  Serial.println(offset);
  Serial.println();


  Serial.println("place a weight on the loadcell");
  //  flush Serial input
  while (Serial.available()) Serial.read();

  Serial.println("enter the weight in (whole) grams and press enter");
  uint32_t weight = 0;
  while (Serial.peek() != '\n')
  {
    if (Serial.available())
    {
      char ch = Serial.read();
      if (isdigit(ch))
      {
        weight *= 10;
        weight = weight + (ch - '0');
      }
    }
  }
  Serial.print("WEIGHT: ");
  Serial.println(weight);
  scale.calibrate_scale(weight, 20);
  float scale2 = scale.get_scale();

  Serial.print("SCALE:  ");
  Serial.println(scale2, 6);

  Serial.print("\nuse scale.set_offset(");
  Serial.print(offset);
  Serial.print("); and scale.set_scale(");
  Serial.print(scale2, 6);
  Serial.print(");\n");
  Serial.println("in the setup of your project");

  Serial.println("\n\n");
}


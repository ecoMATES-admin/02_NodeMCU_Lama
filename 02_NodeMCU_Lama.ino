
#include "ThingSpeak.h"
#include <ESP8266WiFi.h>

/* READ ME
   All data is preceeded by a 'V' character
   The sensor data is preceeded by an 'X' character

*/
#define DEBUG 1

int x = 0;
int i = 0;



// Sensor data
char tempTank[6] = {0};
char ph[6] = {0};
char weight[6] = {0};



//----------- Wi-Fi Details Ketterstr. ---------//
char ssid[] = "Anna+Franz";   // your network SSID (name)
char pass[] = "71277402108590341747";   // your network password
//-------------------------------------------//

/*
  //----------- Wi-Fi Details Rotwandstr. ---------//
  char ssid[] = "FRITZ!Box 7490";   // your network SSID (name)
  char pass[] = "Wilcd2sMeE&T!";   // your network password
  //-------------------------------------------//*/

//----------- Channel Details -------------//
unsigned long Channel_ID1 = 1595496; // Channel ID 
const int Field_number = 1; // Don't change
const char * WriteAPIKey1 = "4E64G1XNYT42LQBR"; 

// ----------------------------------------//

// integrated frequency
unsigned long time_state = 0; // auxiliary variable
int period_state = 100; // period indicating how often SM is called in ms

WiFiClient  client;

// state machine
enum State {INTERNET, WAIT, READ_VALUE1, GET_VALUE1, READ_VALUE2, GET_VALUE2, READ_VALUE3,
            GET_VALUE3, UPLOAD_ALL, SERIAL_FLUSH
           };
State currentState = WAIT;


void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  ThingSpeak.begin(client);
  Serial.println("STARTING");
  Serial.println("-------------------------------------------");
  internet();

}

void loop() {
  if (millis() - time_state >= period_state) { // frequency of program execution{
    time_state = millis();

    SM_Wifi(); // state machine
  }
}

void SM_Wifi() {
  char c;
  switch (currentState) {
    case INTERNET:
      if (DEBUG)
        Serial.println("Checking Connection...");
      internet();
      currentState = SERIAL_FLUSH;
      break;

    case READ_VALUE1: // hum bottom
      if (Serial.find("a")) {
        currentState = GET_VALUE1;
      }
      break;

    case GET_VALUE1:
      if (Serial.available()) {
        if (Serial.peek() == 13) {
          currentState = READ_VALUE2;
          if (DEBUG) {
            Serial.print("VALUE 1: ");
            Serial.println(tempTank);
          }
          i = 0;
          break;
        }
        if (Serial.peek() != 10) {
          tempTank[i] = Serial.read();
          i++;
        }
      }
      break;

    case READ_VALUE2: // temp bottom
      if (Serial.find("b")) {
        currentState = GET_VALUE2;
      }
      break;

    case GET_VALUE2:
      if (Serial.available()) {
        if (Serial.peek() == 13) {
          currentState = READ_VALUE3;
          i = 0;
          if (DEBUG) {
            Serial.print("VALUE 2: ");
            Serial.println(ph);
          }
          break;
        }
        if (Serial.peek() != 10) {
          ph[i] = Serial.read();
          i++;
        }
      }
      break;

    case READ_VALUE3: // hum top
      if (Serial.find("c")) {
        currentState = GET_VALUE3;
      }
      break;

    case GET_VALUE3:
      if (Serial.available()) {
        if (Serial.peek() == 13) {
          currentState = UPLOAD_ALL;
          i = 0;
          if (DEBUG) {
            Serial.print("VALUE 3: ");
            Serial.println(weight);
          }
          break;
        }
        if (Serial.peek() != 10) {
          weight[i] = Serial.read();
          i++;
        }
      }
      break;

    case SERIAL_FLUSH: //Function to distinguish between sensor data and tank capacity data AND (in a future implementation) to not loose any data if both data sets arrive at the same time
      //Note for future implementation: Thing Speak's 15 second upload limitation must be taken into account in the state machine
      if (DEBUG)
        Serial.println("SERIAL_FLUSH");
      if (Serial.find('X')) {
          currentState = READ_VALUE1;
          if (DEBUG){
            Serial.println("SERIAL_FLUSH: sensor data input found");
          }
          break;
      }
      currentState = WAIT;
      break;

    case UPLOAD_ALL:
      if (DEBUG)
        Serial.println("UPLOAD_ALL");
      upload_all();
      currentState = SERIAL_FLUSH;
      break;

    case WAIT:
      if (DEBUG)
        Serial.println("WAIT");
      if (Serial.available()) {
        if (DEBUG){
          Serial.println("Serial input available...");
        }
        currentState = INTERNET;
      }
      break;
  }
}

void internet() {
  if (WiFi.status() != WL_CONNECTED) {
    if (DEBUG) {
      Serial.print("Attempting to connect to SSID: ");
      Serial.println(ssid);
    }
    while (WiFi.status() != WL_CONNECTED) {
      WiFi.begin(ssid, pass);
      if (DEBUG)
        Serial.print(".");
      delay(5000);
    }
  }
  if (DEBUG)
    Serial.println("Connected");
}


void upload_all() {
  ThingSpeak.setField(1, tempTank);
  ThingSpeak.setField(2, ph);
  ThingSpeak.setField(3, weight);


  x = ThingSpeak.writeFields(Channel_ID1, WriteAPIKey1);
  //delay(1000);

  if (DEBUG) {
    if (x == 200)
      Serial.println("Data sent. Code: " + String(x));
    if (x != 200) {
      Serial.println("Problem updating channel. HTTP error code " + String(x));
    }
  }
  
  
  memset(tempTank, 0, 6);
  memset(ph, 0, 6);
  memset(weight, 0, 6);
}

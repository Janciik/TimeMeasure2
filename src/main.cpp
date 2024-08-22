#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <Arduino.h>
#include <AsyncTCP.h>
#include <ArduinoJson.h>
#include <esp_task_wdt.h>
#define WDT_TIMEOUT 5

char buffer[15]; //char array for storing floating point measured time

//Wi-Fi credentials
const char* ssid = "RUT901_2291";
const char* password = "Uw15Fje3";

unsigned long relayActivationTime = 0;
int pulseDuration = 1000;
bool relayActive = false;
bool actionLocked = false;
int activeRelay = 0; // 1 for relay 1, 2 for relay 2

//INPUTS
const int D12 = 12;
const int D13 = 13;
const int D14 = 14;
const int D27 = 27;
const int stanWylacznika = 15;

//OUTPUTS
const int D26 = 26;
const int D25 = 25;
const int D32 = 32;
const int D33 = 33;
const int outputPin = 2;

//Floating point variables for storing precision time
float new_time1 = 0;
float new_time2 = 0;

unsigned long startTime = 0;
unsigned long time1 = 0;
unsigned long time2 = 0;

String status = "";

AsyncWebServer server(80);

void pinSetup(){
  pinMode(outputPin, OUTPUT); 
  pinMode(D32, OUTPUT);
  pinMode(D33, OUTPUT);
  pinMode(D25, OUTPUT);
  pinMode(D26, OUTPUT);

  //Set pins 12, 13, 14, 27 to INPUTS
  pinMode(D25, INPUT);
  pinMode(D26, INPUT);
  pinMode(D14, INPUT);
  pinMode(D27, INPUT);
  pinMode(stanWylacznika, INPUT);

 //Set pins 32, 33, 25, 26 states
  digitalWrite(D32, LOW);
  digitalWrite(D33, HIGH);
  digitalWrite(D25, HIGH);
  digitalWrite(D26, HIGH);
}

//Set Wi-fi connection
void wifiInit(){
  WiFi.begin(ssid, password);
  while(WiFi.status() != WL_CONNECTED){
    Serial.println("Connecting to Wi-Fi");
    delay(1000);
  }
  Serial.println(WiFi.localIP());
}
/* 
Opening relay for specific amount of time

Function checks if there is any ongoing action
It sets the relay pin to HIGH and records the time.
activeRelay flag is set to 1 to indicate relay 1 state change.
relayActive is used to indicate which relay is active.
*/
void openRelay(int duration){
  if (!actionLocked){
    actionLocked = true; //If true, you need to wait for action to finish
    digitalWrite(D32, HIGH);
    relayActivationTime = millis();
    pulseDuration = duration * 1000; //Set to miliseconds
    activeRelay = 1; //Relay 1 state change flag
    relayActive = true;
  }
  else{
    Serial.println("Aktualnie wykonywana jest inna operacja!");
  }
}
/*
Closing relay for specific amount of time

Function checks if there is any ongoing action
It sets the relay pin to LOW and records the time.
activeRelay flag is set to 2 to indicate relay 2 state change.
relayActive is used to indicate which relay is active.
 */
void closeRelay(int duration){
  if (!actionLocked){
    actionLocked = true; //If true, you need to wait for action to finish
    digitalWrite(D33, LOW);
    relayActivationTime = millis(); 
    pulseDuration = duration * 1000; //Set to miliseconds
    activeRelay = 2; //Relay 2 state change flag
    relayActive = true;
  }
  else{
    Serial.println("Aktualnie wykonywana jest inna operacja!");
  }
}

/*
Updates the state of the relay based on specified conditions.

This function checks if the relay is active if the specified pulse duration has elapsed since the relay was activated.
If the conditions are met, function switches off active relay and update state variables.

*/
void updateRelayState(){
  if(relayActive){
    unsigned long currentTime = millis();
    if(currentTime - relayActivationTime >= pulseDuration){
      switch(activeRelay){
        case 1:
          digitalWrite(D32, LOW);
          break;
        case 2:
          digitalWrite(D33, HIGH);
          break;
        default:
          break;
      }
      relayActive = false;
      activeRelay = 0;
      actionLocked = false;
    }
  }
}

/*
This function checks the relay state and measure time it takes to state change

If main switch is LOW then print out the message about main switch state
If main is HIGH, measure time it takes to change state on relay
Convert measured time to floating point number and print it on website
 */
String closeRelayTest() {
  Serial.println("Stan wylacznika: " + String(digitalRead(stanWylacznika)));
  digitalWrite(D33, HIGH);
  if (digitalRead(stanWylacznika) == LOW) {
    Serial.println("Wylacznik jest otwarty - zamknij wylacznik");
    status = "Wyłącznik jest otwarty - zamknij wyłącznik";
    return status;
  }
  else if (digitalRead(stanWylacznika) == HIGH) {
    status = "";
    startTime = micros(); // Capture start time when button is pressed
    Serial.println("Czas rozpoczecia: " + String(startTime));
    while (digitalRead(D25) == LOW); // Wait for pin state to change
    unsigned long currentTime = micros(); // Capture end time when pin state changes
    time1 = currentTime - startTime;
    Serial.println("Czas zakonczenia: " + String(currentTime));
    Serial.println("Czas: " + String(time2));
    digitalWrite(D33, HIGH);
    new_time1 = time1 / 1000.0; // Convert to milliseconds
    dtostrf(new_time1, 3, 3, buffer); //Set number to floating point number
  }
  return String(buffer) + " ms";
}
/*
This function checks the relay state and measure time it takes to state change

If main switch is HIGH then print out the message about main switch state
If main is LOW, measure time it takes to change state on relay
Convert measured time to floating point number and print it on website
 */
String openRelayTest() {
  Serial.println("Stan wylacznika: " + String(digitalRead(stanWylacznika)));
  digitalWrite(D32, LOW);
  if (digitalRead(stanWylacznika) == HIGH) {
    Serial.println("Wylacznik jest zamkniety - otworz wylacznik");
    status = "Wyłącznik jest zamknięty - otwórz wyłącznik";
    return status;
  } 
  else if (digitalRead(stanWylacznika) == LOW) {
    status = "";
    startTime = micros(); // Capture start time when button is pressed
    Serial.println("Czas rozpoczecia: " + String(startTime));
    while (digitalRead(D26) == LOW);
    unsigned long currentTime = micros();
    time2 = currentTime - startTime;
    Serial.println("Czas zakonczenia: " + String(currentTime));
    Serial.println("Czas: " + String(time2));
    digitalWrite(D32, HIGH);
    new_time2 = time2 / 1000.0; // Convert to milliseconds
    dtostrf(new_time2, 3, 3, buffer); //Set number to floating point number
  }
  return String(buffer) + " ms";
}

void setup(){
  Serial.begin(115200);
  pinSetup();
  wifiInit();
  esp_task_wdt_init(WDT_TIMEOUT, true);
  if(!SPIFFS.begin(true)){
    Serial.println("SPIFFS Error");
    return;
  }
  Serial.println(WiFi.localIP());
  server.begin();
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html", String()); //Send HTML file to ESP memory
  });
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/style.css", "text/css"); //Send CSS file to ESP memory
  });
  server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/script.js", "text/javascript"); //Send JS file to ESP memory
  });
  server.on("/opened.png", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/opened.png", "image/png"); //Send switch opened image to ESP memory
  });
  server.on("/closed.png", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/closed.png", "image/png"); //Send switch closed image to ESP memory
  });
  server.on("/open", HTTP_GET, [](AsyncWebServerRequest *request){
    if(request->hasParam("duration")){ 
      int duration = request->getParam("duration")->value().toInt(); //Get pulse duration from pulseValue input
      openRelay(duration);
      request->send(200, "text/html", "Relay opened for " + String(duration) + " s");
    }
  });
  server.on("/close", HTTP_GET, [](AsyncWebServerRequest *request){
    if(request->hasParam("duration")){
      int duration = request->getParam("duration")->value().toInt(); //Get pulse duration from pulseValue input
      closeRelay(duration);
      request->send(200, "text/plain", "Relay closed for " + String(duration) + " s");
    }
  });
  server.on("/openrelaytest", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", openRelayTest());
  });
  server.on("/closerelaytest", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", closeRelayTest());
  });
  server.on("/pinState", HTTP_GET, [](AsyncWebServerRequest *request){
    int pinState = digitalRead(stanWylacznika);
    request->send(200, "text/plain", String(pinState));
  });
}

void loop(){
  updateRelayState(); //Refresh to check if switch state changes
}
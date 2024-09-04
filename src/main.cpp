#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <Arduino.h>
#include <AsyncTCP.h>
#include <ArduinoJson.h>
#include <esp_task_wdt.h>

//Tablice do przechowywania wartości zmiennoprzecinkowych
char buffer1[12];
char buffer2[12];
char buffer3[12];
char localBuffer[12];

//SSID i hasło do Wi-Fi
const char* ssid = "Enerlink";
const char* password = "enerlink12345";
unsigned long relayActivationTime = 0;
int pulseDuration = 1000;
bool relayActive = false; //Flaga do sprawdzenia, czy przekaznik jest aktualnie w innym stanie
bool actionLocked = false; //Flaga do zablokowania wykonania innej akcji, gdy wykonywana jest inna akcja
int activeRelay = 0; //Flaga do sprawdzenia, ktory przekaznik jest aktualnie w innym stanie

//Wejścia 
const int D19 = 19; //Otworzenie L1
const int D18 = 18; //Zamkniecie L1
const int D5 = 5; //Otwarcie L2
const int D17 = 17; //Zamkniecie L2
const int D16 = 16; //Otwarcie L3
const int D4 = 4; //Zamkniecie L3

const int D35 = 35; //Otwarcie lokalnego przekaznika
const int D34 = 34; //Zamkniecie lokalnego przekaznika
const int stanWylacznika = 15; //Stan glownego wylacznika

//OUTPUTS
const int D32 = 32; //Wyjscie do otwarcia lokalnego przekaznika
const int D33 = 33; //Wyjscia do zamkniecia lokalnego przekaznika
const int D26 = 26; //Wyjscie do otwarcia glownego wylacznika
const int D27 = 27; //Wyjscie do zamkniecia glownego wylacznika

//Zmienne zmiennoprzecinkowe do przechowywania czasu z wartosciami dziesietnymi
float new_time1;
float new_time2;
float new_time3;
float localTime;

//Zmienne do przechwytywania czasu po zmianie stanu przekaznika
unsigned long time1;
unsigned long time2;
unsigned long time3;
unsigned long time4;
String status = "";
AsyncWebServer server(80);

void pinSetup(){
  //Ustawienie pinow 32, 33, 26, 27 na wyjscie
  pinMode(D32, OUTPUT);
  pinMode(D33, OUTPUT);
  pinMode(D26, OUTPUT);
  pinMode(D27, OUTPUT);

  //Ustawienie pinow 19, 18, 17, 16, 5, 4, 34, 35 i stanu wylacznika na wejscie
  pinMode(D19, INPUT);
  pinMode(D18, INPUT);
  pinMode(D17, INPUT);
  pinMode(D16, INPUT);
  pinMode(D5, INPUT);
  pinMode(D4, INPUT);
  pinMode(D34, INPUT);
  pinMode(D35, INPUT);
  pinMode(stanWylacznika, INPUT);

 //Ustawienie w stan wysoki pinow odpowiadajacych za wyjscie na przekaznik i glowny wylacznik
  digitalWrite(D26, HIGH);
  digitalWrite(D27, HIGH);
  digitalWrite(D33, HIGH);
  digitalWrite(D32, HIGH);
}
//Uruchomienie polaczenia Wi-Fi
void wifiInit(){
  WiFi.begin(ssid, password);
  while(WiFi.status() != WL_CONNECTED){
    Serial.println("Connecting to Wi-Fi");
    delay(1000);
  }
  Serial.println(WiFi.localIP());
}

/* 
  Funkcja odpowiedzialna za otwarcie wylacznika na podany przez uzytkownika czas
  actionLocked odpowiada za zablokowanie wykonywania innej operacji w tym momencie
  Ustawia piny 26 i 18 w stan niski na podany przez uzytkownika czas
  Ustawia flage, czy i ktory przekaznik jest aktualnie zamkniety
  Po uplynieciu czasu zmienia stan przekaznika w domyslny otwarty
*/
void openRelay(int duration){
  if (!actionLocked){
    actionLocked = true; //If true, you need to wait for action to finish
    digitalWrite(D26, LOW);
    digitalWrite(D18, LOW);
    relayActivationTime = millis();
    pulseDuration = duration * 1000;
    activeRelay = 1; //Ustawienie flagi aktywnego przekaznika na 1
    relayActive = true;
  }
  else{
    Serial.println("Aktualnie wykonywana jest inna operacja!");
  }
}

/*
  Funkcja odpowiedzialna za zamkniecie wylacznika na podany przez uzytkownika czas
  actionLocked odpowiada za zablokowanie wykonywania innej operacji w tym momencie
  Ustawia piny 27 i 19 w stan niski na podany przez uzytkownika czas
  Ustawia flage, czy i ktory przekaznik jest aktualnie zamkniety
  Po uplynieciu czasu zmienia stan przekaznika w domyslny otwarty
*/
void closeRelay(int duration){
  if (!actionLocked){
    actionLocked = true; //Jesli actionLocked = true, stan na przekazniku musi byc niski, aby przeprowadzić inną akcję
    digitalWrite(D27, LOW);
    digitalWrite(D19, LOW);
    relayActivationTime = millis(); 
    pulseDuration = duration * 1000; //Set to miliseconds
    activeRelay = 2; //Ustawienie flagi aktywnego przekaznika na 2
    relayActive = true;
  }
  else{
    Serial.println("Aktualnie wykonywana jest inna operacja!");
  }
}

/*
  Funkcja ktora cyklicznie odswieza stan wylacznika
  Sprawdza czy uplynal czas stanu w ktorym byl wylacznik od momentu jego przelaczenia
  Jesli ten czas uplynal, funkcja przelacza wylacznik w stan niski i aktualizuje jego stan
*/
void updateRelayState(){
  if(relayActive){
    unsigned long currentTime = millis();
    if(currentTime - relayActivationTime >= pulseDuration){
      switch(activeRelay){
        case 1:
          digitalWrite(D26, HIGH);
          break;
        case 2:
          digitalWrite(D27, HIGH);
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
  Funkcja, ktora mierzy czas zamkniecia lokalnego przekaznika (polaczonego bezposrednio do ESP32)
  Przekaznik ustawiany jest w stan wysoki i mierzony jest czas wykrycia stanu wysokiego przez ESP32
  Czas nastepnie jest zwracany w milisekundach i wyswietlany na stronie
*/
String localCloseTest(){
  if(digitalRead(stanWylacznika) == LOW){
    digitalWrite(D33, LOW);
    unsigned long startTime = micros();
    while(digitalRead(D34) == LOW); //Poczekaj na wykrycie zmiany stanu
    time4 = micros() - startTime;
    digitalWrite(D33, HIGH);
    localTime = time4 / 1000.0;
    dtostrf(localTime, 3, 3, localBuffer);
  }
  return String(localBuffer) + " ms";
  memset(localBuffer, 0, sizeof(localBuffer)); //Czyszczenie buforu przechowujacego czasu
}

/*
  Funkcja, ktora mierzy czas otwarcia przekaznika (polaczonego bezposrednio do ESP32)
  Przekaznik ustawiany jest w stan wysoki i mierzony jest czas wykrycia stanu wysokiego przez ESP32
  Czas nastepnie jest zwracany w milisekundach i wyswietlany na stronie
*/
String localOpenTest(){
  if(digitalRead(stanWylacznika) == HIGH){
    digitalWrite(D32, LOW);
    unsigned long startTime = micros();
    while(digitalRead(D35) == LOW); //Poczekaj na wykrycie zmiany stanu
    time4 = micros() - startTime;
    digitalWrite(D32, HIGH);
    localTime = time4 / 1000.0;
    dtostrf(localTime, 3, 3, localBuffer);
  }
  return String(localBuffer) + " ms";
  memset(localBuffer, 0, sizeof(localBuffer)); //Czyszczenie buforu przechowujacego czas
}
/*
  Funkcja sprawdza stan wylacznika i mierzy czas wykrycia stanu wysokiego
  Jesli glowny wylacznik jest OTWARTY to wypisywana jest odpowiednia wiadomosc na port szeregowy
  Jesli glowny wylacznik jest ZAMKNIETY to mierzony jest czas wykrycia stanu wysokiego
  Uplyniety czas jest nastepnie konwertowany z mikrosekund na milisekundy z trzema miejscami po przecinku
*/
String closeRelayTest(int duration) {
  if(actionLocked){
    Serial.println("Aktualnie wykonywana jest inna operacja!");
  }
  else{
    actionLocked = true;
    Serial.println("Stan wylacznika: " + String(digitalRead(stanWylacznika)));
    if (digitalRead(stanWylacznika) == HIGH) {
      Serial.println("Wylacznik jest zamkniety - otworz wylacznik");
      status = "Wyłącznik jest zamknięty - otwórz wyłącznik";
      actionLocked = false;
      return status;
    }
    else if (digitalRead(stanWylacznika) == LOW) {
      digitalWrite(D27, LOW);
      status = "";
      bool pin1High = false, pin2High = false, pin3High = false;
      unsigned long startTime = micros(); // Capture start time when button is pressed
      while (!pin1High || !pin2High || !pin3High) {
          if (!pin1High && digitalRead(D18) == LOW) {
              time1 = micros() - startTime;
          } else {
              pin1High = true;
          }
          if (!pin2High && digitalRead(D17) == LOW) {
              time2 = micros() - startTime;
          } else {
              pin2High = true;
          }
          if (!pin3High && digitalRead(D4) == LOW) {
              time3 = micros() - startTime;
          } else {
              pin3High = true;
          }
      }
      delay(duration * 1000);
      digitalWrite(D27, HIGH); // Zmiana stanu na wysoki
      new_time1 = time1 / 1000.0; // Konwersja na milisekundy
      new_time2 = time2 / 1000.0;
      new_time3 = time3 / 1000.0;

      dtostrf(new_time1, 3, 3, buffer1); //Zapisanie czasu do zmiennej typu String
      dtostrf(new_time2, 3, 3, buffer2);
      dtostrf(new_time3, 3, 3, buffer3);

      Serial.println("Czas L1: " + String(time1));
      Serial.println("Czas L2: " + String(time2));
      Serial.println("Czas L3: " + String(time3));
      actionLocked = false;
    }
  }
  return "Czas L1: " + String(buffer1) + " ms" + "\n";
  return "Czas L2: " + String(buffer2) + " ms" + "\n";
  return "Czas L3: " + String(buffer3) + " ms" + "\n";

  memset(buffer1, 0, sizeof(buffer1)); //Czyszczenie buforu przechowujacego czas na pierwszej fazie
  memset(buffer2, 0, sizeof(buffer2)); //Czyszczenie buforu przechowujacego czas na drugiej fazie
  memset(buffer3, 0, sizeof(buffer3)); //Czyszczenie buforu przechowujacego czas na trzeciej fazie
}
/*
  Funkcja sprawdza stan wylacznika i mierzy czas wykrycia stanu wysokiego
  Jesli glowny wylacznik jest ZAMKNIETY to wypisywana jest odpowiednia wiadomosc na port szeregowy
  Jesli glowny wylacznik jest OTWARTY to mierzony jest czas wykrycia stanu wysokiego
  Uplyniety czas jest nastepnie konwertowany z mikrosekund na milisekundy z trzema miejscami po przecinku
*/
String openRelayTest(int duration) {
  if(actionLocked){
    Serial.println("Aktualnie wykonywana jest inna operacja!");
  }
  else{
    actionLocked = true;
    Serial.println("Stan wylacznika: " + String(digitalRead(stanWylacznika)));
    if (digitalRead(stanWylacznika) == LOW) {
      Serial.println("Wylacznik jest otwarty - zamknij wylacznik");
      status = "Wyłącznik jest otwarty - zamknij wyłącznik";
      actionLocked = false;
      return status;
    }
    else if (digitalRead(stanWylacznika) == HIGH) {
      digitalWrite(D26, LOW);
      status = "";
      bool pin1High = false, pin2High = false, pin3High = false;
      unsigned long startTime2 = micros(); //Zapisz czas otwarcia w zmiennej
      while (!pin1High || !pin2High || !pin3High){
          if (!pin1High && digitalRead(D19) == LOW) {
              time1 = micros() - startTime2;
          } else {
              pin1High = true;
          }
          if (!pin2High && digitalRead(D5) == LOW) {
              time2 = micros() - startTime2;
          } else {
              pin2High = true;
          }
          if (!pin3High && digitalRead(D16) == LOW) {
              time3 = micros() - startTime2;
          } else {
              pin3High = true;
          }
      }
      // time1 = micros() - startTime2; //Wynik tego odejmowania to czas wykrycia zmiany stanu
      // time2 = micros() - startTime2;
      // time3 = micros() - startTime2;
      delay(duration * 1000);
      digitalWrite(D26, HIGH); //Zmiana stanu na wysoki

      new_time1 = time1 / 1000.0;
      new_time2 = time2 / 1000.0;
      new_time3 = time3 / 1000.0;

      dtostrf(new_time1, 3, 3, buffer1); //Konwersja czasu na liczbę zmiennoprzecinkową
      dtostrf(new_time2, 3, 3, buffer2);
      dtostrf(new_time3, 3, 3, buffer3);

      Serial.println("Czas L1: " + String(time1));
      Serial.println("Czas L2: " + String(time2));
      Serial.println("Czas L3: " + String(time3));
      actionLocked = false;
    }
  }

  return "Czas L1: " + String(buffer1) + " ms" + "\n";
  return "Czas L2: " + String(buffer2) + " ms" + "\n";
  return "Czas L3: " + String(buffer3) + " ms" + "\n";

  memset(buffer1, 0, sizeof(buffer1)); //Czyszczenie buforu przechowujacego czas na pierwszej fazie
  memset(buffer2, 0, sizeof(buffer2)); //Czyszczenie buforu przechowujacego czas na drugiej fazie
  memset(buffer3, 0, sizeof(buffer3)); //Czyszczenie buforu przechowujacego czas na trzeciej fazie

}

void setup(){
  Serial.begin(115200);
  pinSetup();
  wifiInit();
  if(!SPIFFS.begin(true)){
    Serial.println("SPIFFS Error");
    return;
  }
  Serial.println(WiFi.localIP());
  server.begin();
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html", String()); //Przeslij plik HTML do pamieci ESP32
  });
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/style.css", "text/css"); //Przeslij plik CSS do pamieci ESP32
  });
  server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/script.js", "text/javascript"); //Przeslij plik JS do pamieci ESP32
  });
  server.on("/favicon.png", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/favicon.png", "image/png"); //Przeslij plik favicon do pamieci ESP32
  });
  server.on("/opened.png", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/opened.png", "image/png"); //Przeslij obrazek stanu niskiego wylacznika do pamieci ESP32
  });
  server.on("/closed.png", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/closed.png", "image/png"); //Przeslij obrazek stanu wysokiego wylacznika do pamieci ESP32
  });
  server.on("/logo.png", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/logo.png", "image.png"); //Przeslij logo do pamieci ESP32
  });
  server.on("/open", HTTP_GET, [](AsyncWebServerRequest *request){
    if(request->hasParam("duration")){ 
      int duration = request->getParam("duration")->value().toInt(); //Pobierz wartosc dlugosci pulsu z pola tekstowego
      openRelay(duration);
      request->send(200, "text/html", "Relay opened for " + String(duration) + " seconds");
    }
  });
  server.on("/close", HTTP_GET, [](AsyncWebServerRequest *request){
  if(request->hasParam("duration")){
    int duration = request->getParam("duration")->value().toInt(); //Pobierz wartosc dlugosci pulsu z pola tekstowego
    closeRelay(duration);
    request->send(200, "text/html", "Relay opened for " + String(duration) + " seconds");
  }
  });
  server.on("/localclosetime", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(200, "text/html", localCloseTest());
  });
  server.on("/localopentime", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/html", localOpenTest());
  });
  server.on("/openrelaytest", HTTP_GET, [](AsyncWebServerRequest *request){
  if (request->hasParam("duration")) {
    int duration = request->getParam("duration")->value().toInt(); //Pobierz wartosc dlugosci pulsu z pola tekstowego
    openRelayTest(duration);
    request->send(200, "text/plain", "<br />Czas L1: " + String(buffer1)+ " ms" + "<br />" + "Czas L2: " + String(buffer2) + " ms" + "<br />" + "Czas L3: " + String(buffer3) + " ms"); //Przeslij obliczony czas na stronie internetowa
  }
  });
  server.on("/closerelaytest", HTTP_GET, [](AsyncWebServerRequest *request){
  if (request->hasParam("duration")) {
    int duration = request->getParam("duration")->value().toInt(); //Pobierz wartosc dlugosci pulsu z pola tekstowego
    closeRelayTest(duration);
    request->send(200, "text/plain", "<br />Czas L1: " + String(buffer1) + " ms" + "<br />" + "Czas L2: " + String(buffer2) + " ms" + "<br />" + "Czas L3: " + String(buffer3) + " ms"); //Przeslij obliczony czas na strone internetowa
  }
  });
  server.on("/pinState", HTTP_GET, [](AsyncWebServerRequest *request){
    int pinState = digitalRead(stanWylacznika);
    request->send(200, "image/png", String(pinState)); //Przeslij stan wylacznika na strone internetowa
  });
}
void loop(){
  updateRelayState(); //Odswiezanie stanu przekaznikow
}
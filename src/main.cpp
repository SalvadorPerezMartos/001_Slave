//HTTP CLIENT

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Servo.h>

#define LED1      4
#define LED2      0
#define LED3      2


struct MagnSensor {
  const uint8_t PIN;
  bool onHigh;
};

MagnSensor magnSensor1 = {33, false};
MagnSensor magnSensor2 = {32, false};
MagnSensor magnSensor3 = {12, false};


const char* ssid = "Tenda_877700";
const char* password = "12345678";
const char* hostname = "ESP32_1";

// Set your Static IP address
IPAddress local_IP(192, 168, 2, 201);
// Set your Gateway IP address
IPAddress gateway(192, 168, 1, 1);

IPAddress subnet(255, 255, 0, 0);
IPAddress primaryDNS(8, 8, 8, 8);   // optional
IPAddress secondaryDNS(8, 8, 4, 4); // optional

const char* url_server = "http://192.168.1.200/device1";

String answer = "Estado 02";
uint8_t status = 2;
uint8_t prev_status = 2;

unsigned long previousMillis1 = 0;
unsigned long interval1 = 10000;

unsigned long previousMillis2 = 0;
unsigned long interval2 = 30000;

bool seguir = false;

Servo servo;


String getRequest (const char* serverName){
  HTTPClient http;
  http.begin(serverName);

  int httpResponseCode = http.GET(); //Enviamos peticion HTTP

  String payload = "...";

  if (httpResponseCode == 200){
    Serial.print("\nHTTP Response code: ");
    Serial.println(httpResponseCode);
    payload = http.getString();
  }
  else {
    Serial.print("\nError code:");
    Serial.println(httpResponseCode);
  }

  http.end(); //liberamos

  return payload;
}

void IRAM_ATTR isr1() {
  magnSensor1.onHigh = true;
}

void IRAM_ATTR isr2() {
  magnSensor2.onHigh = true;
}

void IRAM_ATTR isr3() {
  magnSensor3.onHigh = true;
}


void abierto(){
  if (prev_status != 3){
    servo.writeMicroseconds(1384);  //para arriba
    seguir = true;
  }
  else{
    servo.writeMicroseconds(1372);  //parado
  }
}

void cerrado(){
  if (prev_status != 1){
    servo.writeMicroseconds(1360);  //para abajo
    seguir = true;
  }
  else{
    servo.writeMicroseconds(1372);  //parado
  }
}


void medio(){
  switch(prev_status){
    case 1:
    servo.writeMicroseconds(1384);  //para arriba
    seguir = false;
    break;
    case 2:
    servo.writeMicroseconds(1372);  //parado
    seguir = true;
    break;
    case 3:
    servo.writeMicroseconds(1360);  //para abajo
    seguir = false;
    break;
  }
}


void checkStatus(){

  switch(status){
    case 1:
    digitalWrite(LED1, HIGH);
    digitalWrite(LED2, LOW);
    digitalWrite(LED3, LOW);
    cerrado();
    break;
    case 2:
    digitalWrite(LED1, LOW);
    digitalWrite(LED2, HIGH);
    digitalWrite(LED3, LOW);
    medio();
    break;
    case 3:
    digitalWrite(LED1, LOW);
    digitalWrite(LED2, LOW);
    digitalWrite(LED3, HIGH);
    abierto();
    break;
  }

  prev_status = status;

}

void initGPIO(){
  pinMode(magnSensor1.PIN, INPUT);
  pinMode(magnSensor2.PIN, INPUT);
  pinMode(magnSensor3.PIN, INPUT);
  attachInterrupt(magnSensor1.PIN, isr1, RISING);
  attachInterrupt(magnSensor2.PIN, isr2, RISING);
  attachInterrupt(magnSensor3.PIN, isr3, RISING);
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);
  digitalWrite(LED1, LOW);
  digitalWrite(LED2, LOW);
  digitalWrite(LED3, LOW);
  servo.attach(27);
  servo.writeMicroseconds(1372);  //parado
}

void checkMagnSensor(){

  if (magnSensor1.onHigh) {
    servo.writeMicroseconds(1372);  //parado
    delay(750);
    magnSensor1.onHigh=false;
  }

  if (magnSensor2.onHigh) {
    if(seguir==false){
      servo.writeMicroseconds(1372);  //parado
    }
    delay(750);
    magnSensor2.onHigh = false;
  }

  if (magnSensor3.onHigh) {
    servo.writeMicroseconds(1372);  //parado
    delay(750);
    magnSensor3.onHigh=false;
  }
}


void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("\nConnecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }

  Serial.print("\nRRSI: ");
  Serial.println(WiFi.RSSI());

  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
    Serial.println("STA Failed to configure");
  }

  Serial.print("\nConectando a la red IP: ");
  Serial.println(WiFi.localIP());

}


void setup() {

  Serial.begin(9600);

  initGPIO();

  initWiFi();
}


void loop() {

  unsigned long currentMillis = millis();

  checkMagnSensor();

  if (currentMillis - previousMillis1 >=interval1) {
    answer = getRequest(url_server);
    Serial.println("\nRespuesta del server :");
    Serial.print(answer);

    if (answer == "Estado 01"){
      status = 1;
    } else if (answer == "Estado 02"){
      status = 2;
    } else if(answer == "Estado 03"){
      status = 3;
    } else {
      status = 0;
    }
    checkStatus();
    previousMillis1 = currentMillis;
  }
  
  // Reconectar
  if ((WiFi.status() != WL_CONNECTED) && (currentMillis - previousMillis2 >=interval2)) {
   Serial.print(millis());
    Serial.println("Reconnecting to WiFi...");
    WiFi.disconnect();
   WiFi.reconnect();
   previousMillis2 = currentMillis;
  }

}

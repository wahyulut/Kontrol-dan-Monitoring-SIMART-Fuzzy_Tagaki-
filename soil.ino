#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#define RELAY_PIN D1

LiquidCrystal_I2C lcd(0x27,16,2);
//input ssid dan password wifi
const char* ssid = "SYAKIRA";
const char* password = "jagungmanis";
//input server dan port mqtt broker
const char* host = "privatelutvi.cloud.shiftr.io";
const int port = 1883;

//koneksi ke perpustakaan WiFiClient dan PubSubClient
WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE	(50)
char msg[MSG_BUFFER_SIZE];
int value = 0;


void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Switch on the LED if an 1 was received as first character
  // if ((char)payload[0] == '1') {
  //   digitalWrite(BUILTIN_LED, LOW);   // Turn the LED on (Note that LOW is the voltage level
  //   // but actually the LED is on; this is because
  //   // it is active low on the ESP-01)
  // } else {
  //   digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
  // }

}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str(), "privatelutvi", "wZPr1nSsx5TrdB2T")) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("suhu", "selamat anda berhasil");
      // ... and resubscribe
      client.subscribe("terima");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(2000);
    }
  }
}

unsigned long lastMsgTime = 0;
const unsigned long publishInterval = 2000;
const char* sensorValueTopic = "sensorValue";
const char* moisturePercentageTopic = "moisturePercentage";

const int soilMoisturePin = A0; // Ubah ke pin yang Anda gunakan
const int N_sangat_kering = 900; // > ini = kering
const int N_kering = 700; // > ini = kering
const int N_netral = 450; // > ini = kering
const int N_lembab = 300; // > ini = kering
const int N_sangat_lembab = 100; //< ini = basah

const int v_kering = 700;
const int v_netral = 450;
const int v_basah = 300;

int sensorValue = 0;
int moisturePercentage = 0;

void setup() {

    Serial.begin(9600);

    WiFi.begin(ssid, password);


    while (WiFi.status() != WL_CONNECTED) {
      delay(1000);
      Serial.println("connecting to wifi...");
    }

    Serial.println("WiFi connected");
    Serial.print("ssid: ");
    Serial.println(ssid);
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    //lcd 
    lcd.init();                      // initialize the lcd 
    lcd.backlight();
    lcd.setCursor(0,0);
    lcd.print("N_Sensor : ");
    lcd.print(sensorValue);
    lcd.setCursor(0,1);
    lcd.print("kelembapan : ");
    lcd.print(moisturePercentage);
    lcd.print("%");
    client.setServer(host, port);
    client.setCallback(callback);
    //relay
    pinMode(RELAY_PIN, OUTPUT);
}

void loop() {

  if (!client.connected()) {
    reconnect();
  }

  unsigned long now = millis();

  if (now - lastMsgTime > publishInterval) {
    lastMsgTime = now;

    sensorValue = analogRead(soilMoisturePin);
    moisturePercentage = map(sensorValue, 1023, 0, 0, 100);

    char sensorValueStr[10];
    char moisturePercentageStr[10];

    itoa(sensorValue, sensorValueStr, 10);
    itoa(moisturePercentage, moisturePercentageStr, 10);

    client.publish(sensorValueTopic, sensorValueStr);
    client.publish(moisturePercentageTopic, moisturePercentageStr);

    Serial.print("Mempublish NilaiSensor: ");
    Serial.println(sensorValue);
    Serial.print("Mempublish Kelembapan: ");
    Serial.println(moisturePercentage);
  }

  client.loop();

    //fuzzyfikasi varibel nilai sensor
    //mencari nilai untuk derajat keanggotaan sangat lembab
    int sangat_lembab;
    if(N_lembab < sensorValue){
        
      sangat_lembab = 0;

    }else if(N_sangat_lembab < sensorValue && sensorValue < N_lembab){

    const float hasil_sangat_lembab = (float)(N_lembab - sensorValue) / (N_lembab - N_sangat_lembab);
      sangat_lembab = hasil_sangat_lembab;

    }else if(sensorValue < N_lembab){
      sangat_lembab = 1;

    }

    //mencari nilai untuk derajat keanggotaan lembab
    int lembab;
    if(N_netral <= sensorValue || sensorValue <= N_sangat_lembab){
        
      lembab = 0;

    }else if(N_sangat_lembab < sensorValue && sensorValue < N_lembab){

    const float hasil_lembab = (float)(sensorValue - N_sangat_lembab) / (N_lembab - N_sangat_lembab);
      lembab = hasil_lembab;

    }else if(sensorValue < N_netral || sensorValue > N_lembab){
      lembab = 1;

    }

    //mencari nilai untuk derajat keanggotaan netral
    int netral;
    if(N_kering < sensorValue || sensorValue < N_lembab){
        
      netral = 0;

    }else if(N_lembab < sensorValue && sensorValue < N_netral){

    const float hasil_netral = (float)(sensorValue - N_lembab) / (N_netral - N_lembab);
      netral = hasil_netral;

    }else if(sensorValue > N_netral || sensorValue < N_kering){
      lembab = 1;

    }

    //mencari nilai untuk derajat keanggotaan kering 
    int kering; 
    if(sensorValue < N_netral && N_sangat_kering < sensorValue){
        
      kering = 0;

    }else if(N_netral < sensorValue && sensorValue < N_kering){

    const float hasil_kering = (float)(sensorValue - N_netral) / (N_kering - N_netral);
      kering = hasil_kering;

    }else if(N_kering < sensorValue && sensorValue < N_sangat_kering){
      kering = 1;

    }

    //mencari nilai untuk derajat keanggotaan sangat kering
    int sangat_kering; 
    if(sensorValue <= N_kering){
        
      sangat_kering = 0;

    }else if(N_kering <= sensorValue && sensorValue <= N_sangat_kering){

    const float hasil_sangat_kering = (float)(sensorValue - N_kering) / (N_sangat_kering - N_kering);
      sangat_kering = hasil_sangat_kering;

    }else if(N_sangat_kering <= sensorValue){
      sangat_kering = 1;

    }

    //aturan rule 
    //[R1] jika sangat kering , waktu penyiraman = 2 menit
    //[R2] jika kering , waktu penyiraman =  1 menit
    //[R3] jika netral , waktu penyiraman = 40 detik
    //[R4] jika basah , waktu penyiraman = 20 detik
    //[R5] jika sangat basah , waktu penyiraman = 0 detik
    const int w1 = 120000;
    const int w2 = 60000;
    const int w3 = 40000;
    const int w4 = 20000;
    const int w5 = 0;
    

    //defuzifikasi
    const float z1 = (sangat_lembab * w5) + (lembab * w4) + (netral * w3) + (kering * w2) + (sangat_kering * w1);
    const float z2 = sangat_lembab + lembab + netral + kering + sangat_kering;
    const int x =  z1 / z2;
    int hasil = abs(x);
    Serial.print("Jika Nilai Sensor ");
    Serial.print(sensorValue); // Mencetak nilai dari variabel sensor
    Serial.print(" dan kelembapannya ");
    Serial.print(moisturePercentage); // Mencetak nilai dari variabel kelembapan
    Serial.print(" maka waktu yang diperlukan adalah : ");
    Serial.println(hasil);


    // // // nilai kelembapan dengan ambang batas

    if (sensorValue > v_kering) {
          digitalWrite(RELAY_PIN, HIGH);
          delay(1000);
  
          digitalWrite(RELAY_PIN, LOW);
          delay(10000);

    } else if (sensorValue < v_kering && sensorValue > v_netral) {
          digitalWrite(RELAY_PIN, HIGH);
          delay(1000);
  
          digitalWrite(RELAY_PIN, LOW);
          delay(6000);

    } else if (sensorValue < v_netral && sensorValue > v_basah){
          digitalWrite(RELAY_PIN, HIGH);
          delay(1000);
  
          digitalWrite(RELAY_PIN, LOW);
          delay(2000);
    }else {
          digitalWrite(RELAY_PIN, HIGH);
          delay(1000);
  
          digitalWrite(RELAY_PIN, LOW);
          delay(1000);
    }

  delay(2000);
}
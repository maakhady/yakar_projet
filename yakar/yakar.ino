#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <IRremote.h>
#include <ArduinoJson.h>

// Pins
#define DHTPIN 2
#define DHTTYPE DHT11
#define FAN_PIN 7
#define BUZZER_PIN 8
#define RED_LED 9
#define GREEN_LED 10
#define IR_PIN 6

// Objects
DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);
IRrecv irrecv(IR_PIN);
decode_results results;

// Keypad setup
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
 {'1', '2', '3', 'A'},
 {'4', '5', '6', 'B'},
 {'7', '8', '9', 'C'},
 {'*', '0', '#', 'D'}
};
byte rowPins[ROWS] = {A0, A1, A2, A3};
byte colPins[COLS] = {A4, A5, A6, A7};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

String enteredCode = "";
String secretCode = "1234";
bool accessGranted = false;
bool fanManualControl = false;

unsigned long lastUpdateTime = 0;
const long updateInterval = 5000; // 5 secondes

void setup() {
 Serial.begin(9600);
 dht.begin();
 lcd.init();
 lcd.backlight();
 irrecv.enableIRIn();

 pinMode(FAN_PIN, OUTPUT);
 pinMode(BUZZER_PIN, OUTPUT);
 pinMode(RED_LED, OUTPUT);
 pinMode(GREEN_LED, OUTPUT);

 digitalWrite(FAN_PIN, LOW);
 digitalWrite(BUZZER_PIN, LOW);
 digitalWrite(RED_LED, LOW);
 digitalWrite(GREEN_LED, HIGH);

 lcd.setCursor(0, 0);
 lcd.print("Entrez code :");
}

void sendSensorData(float temp, float hum, bool fanStatus) {
  DynamicJsonDocument doc(128);
  doc["temperature"] = temp;
  doc["humidite"] = hum;
  doc["etatVentilateur"] = fanStatus;
  
  String jsonString;
  serializeJson(doc, Serial);
  Serial.println(); // Nouvelle ligne après le JSON
  Serial.flush();   // S'assurer que tout est envoyé
}

void loop() {
 char key = keypad.getKey();
 if (key) {
   handleKeypadInput(key);
 }

 if (accessGranted) {
   unsigned long currentTime = millis();
   
   // Lecture et envoi périodique des données
   if (currentTime - lastUpdateTime >= updateInterval) {
     float temp = dht.readTemperature();
     float hum = dht.readHumidity();

     if (isnan(temp) || isnan(hum)) {
       lcd.setCursor(0, 0);
       lcd.print("Erreur capteur!");
       return;
     }

     // Mise à jour LCD
     updateLCDDisplay(temp, hum);
     
     // Contrôle automatique
     bool fanStatus = controlSystemStatus(temp);
     
     // Envoi des données
     sendSensorData(temp, hum, fanStatus || fanManualControl);

     lastUpdateTime = currentTime;
   }
 }

 // Contrôle IR
 handleIRControl();
}

void handleKeypadInput(char key) {
 if (key == '#') {
   if (enteredCode == secretCode) {
     accessGranted = true;
     lcd.clear();
     lcd.print("Code correct !");
     delay(1000);
     lcd.clear();
   } else {
     lcd.clear();
     lcd.print("Code incorrect!");
     delay(1000);
     lcd.clear();
     lcd.print("Entrez code :");
   }
   enteredCode = "";
 } else if (key == '*') {
   enteredCode = "";
   lcd.setCursor(0, 1);
   lcd.print("                ");
 } else {
   enteredCode += key;
   lcd.setCursor(0, 1);
   lcd.print(enteredCode);
 }
}

void updateLCDDisplay(float temp, float hum) {
 lcd.setCursor(0, 0);
 lcd.print("T:");
 lcd.print(temp);
 lcd.print("C ");
 lcd.setCursor(0, 1);
 lcd.print("H:");
 lcd.print(hum);
 lcd.print("%");
}

bool controlSystemStatus(float temp) {
 bool fanStatus = temp > 27 || fanManualControl;
 digitalWrite(FAN_PIN, fanStatus);
 digitalWrite(BUZZER_PIN, temp > 27);
 digitalWrite(RED_LED, temp > 27);
 digitalWrite(GREEN_LED, temp <= 27);
 return fanStatus;
}

void handleIRControl() {
 if (irrecv.decode()) {
   unsigned long command = irrecv.decodedIRData.command;
   
   if (command == 0xFFA25D) {
     fanManualControl = !fanManualControl;
     digitalWrite(FAN_PIN, fanManualControl);
     
     // Envoyer mise à jour immédiate
     float temp = dht.readTemperature();
     float hum = dht.readHumidity();
     if (!isnan(temp) && !isnan(hum)) {
       sendSensorData(temp, hum, fanManualControl);
     }
   }
   irrecv.resume();
 }
}

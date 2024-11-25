#include <DHT.h>
#include <LiquidCrystal_I2C.h>
#include <IRremote.hpp>

#define DHTPIN 2
#define DHTTYPE DHT11
#define BUZZER_PIN 4
#define FAN_PIN 5
#define LED_RED 6
#define LED_GREEN 7
#define IR_PIN 8

DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);
float temperature = 0;
float humidity = 0;
bool fanManual = false;
unsigned long lastReadTime = 0;
const int readInterval = 60000;

void setup() {
  Serial.begin(9600);
  dht.begin();
  lcd.init();
  lcd.backlight();
  
  IrReceiver.begin(IR_PIN);  // Nouvelle syntaxe IRremote
  
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(FAN_PIN, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
}

void loop() {
  checkIRRemote();
  
  if (millis() - lastReadTime > readInterval) {
    readSensors();
    updateDisplay();
    checkAlerts();
    lastReadTime = millis();
  }
}

void readSensors() {
  delay(2000);
  temperature = dht.readTemperature();
  humidity = dht.readHumidity();
  
  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("Erreur de lecture DHT11!");
    return;
  }
  
  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.print("°C, Humidity: ");
  Serial.print(humidity);
  Serial.println("%");
}

void updateDisplay() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Temp: ");
  lcd.print(temperature, 1);
  lcd.print("C");
  lcd.setCursor(0, 1);
  lcd.print("Hum: ");
  lcd.print(humidity, 1);
  lcd.print("%");
}

void checkAlerts() {
  if (temperature > 27) {
    digitalWrite(LED_RED, HIGH);
    digitalWrite(LED_GREEN, LOW);
    digitalWrite(BUZZER_PIN, HIGH);
    if (!fanManual) {
      digitalWrite(FAN_PIN, HIGH);
    }
  } else {
    digitalWrite(LED_RED, LOW);
    digitalWrite(LED_GREEN, HIGH);
    digitalWrite(BUZZER_PIN, LOW);
    if (!fanManual) {
      digitalWrite(FAN_PIN, LOW);
    }
  }
}

void checkIRRemote() {
  if (IrReceiver.decode()) {
    fanManual = !fanManual;
    digitalWrite(FAN_PIN, fanManual);
    IrReceiver.resume();
  }
}

#include <DHT.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <RTClib.h>
#include <ArduinoJson.h>

#define DHTPIN 2
#define DHTTYPE DHT11
#define LED_PIN 4
#define MOTOR_PIN 8
#define BUZZER_PIN 5

const float TEMP_SEUIL = 27.0;
bool isVentilationActive = false;
bool commandeManuelle = false;
unsigned long tempsCommandeManuelle = 0;
const unsigned long DUREE_MODE_MANUEL = 300000;

DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);
RTC_DS3231 rtc;

void setup() {
  Serial.begin(9600);
  
  if (!rtc.begin()) {
    Serial.println("{\"error\": \"RTC non trouvé\"}");
    while (1);
  }
  
  if (rtc.lostPower()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  
  dht.begin();
  lcd.init();
  lcd.backlight();
  
  pinMode(LED_PIN, OUTPUT);
  pinMode(MOTOR_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  
  StaticJsonDocument<200> startupDoc;
  startupDoc["status"] = "Système démarré";
  startupDoc["version"] = "1.1";
  serializeJson(startupDoc, Serial);
  Serial.println();
}

void loop() {
  if (commandeManuelle && (millis() - tempsCommandeManuelle >= DUREE_MODE_MANUEL)) {
    commandeManuelle = false;
    envoyerNotification("Retour au mode automatique");
  }

  DateTime now = rtc.now();
  float humidite = dht.readHumidity();
  float temperature = dht.readTemperature();

  if (isnan(humidite) || isnan(temperature)) {
    gererErreurLecture();
    return;
  }

  bool alerteTemp = temperature > TEMP_SEUIL;
  gererSysteme(temperature, humidite, alerteTemp);
  afficherDonnees(now, temperature, humidite);
  envoyerDonnees(now, temperature, humidite, alerteTemp);
  
  checkSerialCommands();
  
  delay(500);
}

void gererSysteme(float temperature, float humidite, bool alerteTemp) {
  if (!commandeManuelle) {
    if (alerteTemp && !isVentilationActive) {
      setVentilation(true);
    } else if (!alerteTemp && isVentilationActive) {
      setVentilation(false);
    }
  }

  digitalWrite(LED_PIN, alerteTemp ? HIGH : LOW);
  if (alerteTemp) {
    tone(BUZZER_PIN, 300);
  } else {
    noTone(BUZZER_PIN);
  }
}

void afficherDonnees(DateTime now, float temperature, float humidite) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(now.hour());
  lcd.print(':');
  if (now.minute() < 10) lcd.print('0');
  lcd.print(now.minute());
  lcd.print(" T:");
  lcd.print(temperature, 1);
  lcd.print("C");
  
  lcd.setCursor(0, 1);
  lcd.print(now.day());
  lcd.print('/');
  lcd.print(now.month());
  lcd.print(" H:");
  lcd.print(humidite, 1);
  lcd.print("%");
}

void checkSerialCommands() {
  if (Serial.available() > 0) {
    StaticJsonDocument<200> command;
    DeserializationError error = deserializeJson(command, Serial);

    if (!error && command.containsKey("ventilation")) {
      commandeManuelle = true;
      tempsCommandeManuelle = millis();
      setVentilation(command["ventilation"].as<bool>());
    }
  }
}

void setVentilation(bool activate) {
  if (activate != isVentilationActive) {
    analogWrite(MOTOR_PIN, activate ? 255 : 0);
    isVentilationActive = activate;
    envoyerEtatVentilation();
  }
}

void envoyerDonnees(DateTime now, float temperature, float humidite, bool alerteTemp) {
  StaticJsonDocument<200> doc;
  doc["temperature"] = temperature;
  doc["humidite"] = humidite;
  doc["date"] = now.unixtime();
  doc["alerte"] = alerteTemp;
  doc["systeme"]["ventilation"] = isVentilationActive;
  doc["systeme"]["mode"] = commandeManuelle ? "manuel" : "auto";
  
  serializeJson(doc, Serial);
  Serial.println();
}

void envoyerEtatVentilation() {
  StaticJsonDocument<100> doc;
  doc["ventilation"] = isVentilationActive;
  doc["mode"] = commandeManuelle ? "manuel" : "auto";
  serializeJson(doc, Serial);
  Serial.println();
}

void envoyerNotification(const char* message) {
  StaticJsonDocument<100> doc;
  doc["type"] = "notification";
  doc["message"] = message;
  serializeJson(doc, Serial);
  Serial.println();
}

void gererErreurLecture() {
  StaticJsonDocument<100> doc;
  doc["error"] = "Erreur lecture capteur";
  serializeJson(doc, Serial);
  Serial.println();
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Erreur lecture!");
}

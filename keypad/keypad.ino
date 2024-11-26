#include <Keypad.h>
#include <ArduinoJson.h>

// Configuration existante du keypad reste identique
const byte ROW_NUM = 4;
const byte COLUMN_NUM = 4;
char keys[ROW_NUM][COLUMN_NUM] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte pin_rows[ROW_NUM] = {A0, A1, A2, A3};
byte pin_column[COLUMN_NUM] = {A4, A5, A6, A7};
Keypad keypad = Keypad(makeKeymap(keys), pin_rows, pin_column, ROW_NUM, COLUMN_NUM);

void setup() {
  Serial.begin(9600);
}

void loop() {
  char key = keypad.getKey();
  if (key) {
    // Créer un objet JSON pour envoyer la touche
    StaticJsonDocument<64> doc;
    doc["type"] = "keypad";
    doc["key"] = String(key);
    
    // Sérialiser et envoyer
    String jsonString;
    serializeJson(doc, jsonString);
    Serial.println(jsonString);
  }
}

#include <FirebaseESP8266.h>  // Firebase biblioteka za ESP8266
#include <ESP8266WiFi.h>
#include <DHT.h>
// link za pakete esp mcu 8266 : http://arduino.esp8266.com/stable/package_esp8266com_index.json

// Firebase podaci
#define WIFI_SSID "KK STUDENT"         
#define WIFI_PASSWORD "student123"      
#define DATABASE_URL "https://smartair-835f0-default-rtdb.firebaseio.com" 
#define API_KEY "AIzaSyBgC_IGy_kM_4QksoFzG756UnU4StqYMrU"                      

// Firebase objekti
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
bool signupOK = false;

// Pinovi za komponente
#define DHTPIN 0      
#define DHTTYPE DHT11 
int GREEN_LED = 5;   
int RED_LED = 4;    
#define RELAY 2     

// Temperature thresholds
#define TEMP_OK 15     // Normalna temperatura
#define TEMP_HIGH 27   // Visoka temperatura

DHT dht(DHTPIN, DHTTYPE);

// Promenljive za stanje
String mode = "manual"; 
bool fanState = false;  // Po defaultu ventilator isključen

void setup() {
  Serial.begin(115200);

  // Inicijalizacija DHT senzora
  dht.begin();

  // Inicijalizacija pinova
  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(RELAY, OUTPUT);

  digitalWrite(GREEN_LED, LOW);
  digitalWrite(RED_LED, LOW);
  digitalWrite(RELAY, HIGH); // Ventilator isključen po defaultu

  Serial.println("System initialized.");

  // WiFi povezivanje
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Povezivanje na Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println("\nWi-Fi povezan.");
  Serial.print("IP adresa: ");
  Serial.println(WiFi.localIP());

  // Firebase konfiguracija
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("Firebase autentifikacija uspješna!");
    signupOK = true;
  } else {
    Serial.print("Greška pri prijavi: ");
    Serial.println(config.signer.tokens.error.message.c_str());
  }
}

void loop() {
  delay(2100);

  // Čitanje temperature sa senzora
  float temperature = dht.readTemperature();

  if (isnan(temperature)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  Serial.print("Temperatura: ");
  Serial.print(temperature);
  Serial.println(" *C");

  // **Ažurirano: LED-ice sada prate temperaturu**
  if (temperature >= TEMP_OK && temperature < TEMP_HIGH) {
    digitalWrite(GREEN_LED, HIGH);
    digitalWrite(RED_LED, LOW);
  } else if (temperature >= TEMP_HIGH) {
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(RED_LED, HIGH);
  } else {
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(RED_LED, LOW);
  }

  // **Čitanje trenutnog režima rada iz Firebase-a**
  if (Firebase.getString(fbdo, "/mode")) {
    mode = fbdo.stringData();
    Serial.print("Trenutni režim: ");
    Serial.println(mode);
  } else {
    Serial.print("Greška pri preuzimanju režima rada: ");
    Serial.println(fbdo.errorReason());
  }

  // **Čitanje `fanState` iz Firebase-a samo ako je manuelni režim**
  if (mode == "manual") {
    if (Firebase.getBool(fbdo, "/fanState")) {
      fanState = fbdo.boolData();
    } else {
      Serial.print("Greška pri preuzimanju stanja ventilatora: ");
      Serial.println(fbdo.errorReason());
    }
  }

  // **Ako je AUTOMATSKI režim, Arduino sam odlučuje kada da upali ventilator**
  if (mode == "automatic") {
    if (temperature >= TEMP_HIGH) {
      fanState = true;  // Uključi ventilator
    } else {
      fanState = false; // Isključi ventilator
    }

    // **Ažuriraj Firebase samo ako je automatski režim aktivan**
    if (Firebase.setBool(fbdo, "/fanState", fanState)) {
      Serial.println("Automatski režim: Stanje ventilatora poslato na Firebase.");
    } else {
      Serial.print("Greška pri slanju statusa ventilatora: ");
      Serial.println(fbdo.errorReason());
    }
  }

  // **Primeni fanState na ventilator**
  digitalWrite(RELAY, fanState ? LOW : HIGH);
  Serial.println(fanState ? "Ventilator UKLJUČEN." : "Ventilator ISKLJUČEN.");

  // **Slanje temperature na Firebase**
  if (signupOK) {
    if (Firebase.setFloat(fbdo, "/temperature", temperature)) {
      Serial.println("Temperatura poslata na Firebase.");
    } else {
      Serial.print("Greška pri slanju temperature: ");
      Serial.println(fbdo.errorReason());
    }
  }

  /*
   if (Firebase.getString(fbdo, "/buttonState")) {
    String resetState = fbdo.stringData();

    if (resetState == "on") {  
      
        digitalWrite(GREEN_LED, HIGH);
        delay(1000);
        digitalWrite(GREEN_LED, LOW);

        delay(1000); // Kratka pauza između LED promjena

       
        digitalWrite(RED_LED, HIGH);
        delay(1000);
        digitalWrite(RED_LED, LOW);

        delay(5000); // Pauza prije slanja novog statusa

       
        Firebase.setString(fbdo, "/buttonState", "off");  
        Serial.println("LED sekvenca završena, status postavljen na OFF.");
    }
}

 */

}

#define BLYNK_TEMPLATE_ID "TMPL6A-G6X_SD"
#define BLYNK_TEMPLATE_NAME "project"
#define BLYNK_AUTH_TOKEN "hNCOjskc9Y3CTpOl0jEysgG8nVTteiJs"

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <BlynkSimpleEsp8266.h>  // or your platform variant

// ---------- Blynk Setup ----------
char auth[] = "hNCOjskc9Y3CTpOl0jEysgG8nVTteiJs";
#define WIFI_SSID "NuwangieA12"
#define WIFI_PASS "20021106"

// ---------- Pin Configuration ----------
#define HALL_PIN D6
#define DHT_PIN D5
#define DHTTYPE DHT11
#define RAIN_ANALOG_PIN A0

#define BUZZER_RAIN D8
#define BUZZER_WIND D7

#define IR_North D0
#define IR_South D2
#define IR_East  D3
#define IR_West  D4

// ---------- Objects ----------
LiquidCrystal_I2C lcd(0x27, 16, 2);
DHT dht(DHT_PIN, DHTTYPE);

// ---------- Wind speed ----------
volatile unsigned int pulseCount = 0;
unsigned long lastCalcTime = 0;
const float radius = 0.09;
const float pi = 3.1416;

// ---------- Interrupt ----------
void IRAM_ATTR countPulse() {
  pulseCount++;
}

void setup() {
  Serial.begin(115200);

  Blynk.begin(auth, WIFI_SSID, WIFI_PASS);

  dht.begin();

  pinMode(HALL_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(HALL_PIN), countPulse, FALLING);

  pinMode(IR_North, INPUT);
  pinMode(IR_South, INPUT);
  pinMode(IR_East, INPUT);
  pinMode(IR_West, INPUT);

  pinMode(BUZZER_RAIN, OUTPUT);
  pinMode(BUZZER_WIND, OUTPUT);
  digitalWrite(BUZZER_RAIN, LOW);
  digitalWrite(BUZZER_WIND, LOW);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Weather Station");
  delay(1500);
  lcd.clear();
}

void loop() {
  Blynk.run();

  unsigned long currentTime = millis();

  if (currentTime - lastCalcTime >= 2000) {
    noInterrupts();
    unsigned int count = pulseCount;
    pulseCount = 0;
    interrupts();

    float rpm = count * 60.0;
    float windSpeedMs = (2 * pi * radius * rpm) / 60.0;
    float windSpeedKmh = windSpeedMs * 3.6;

    bool n = digitalRead(IR_North) == LOW;
    bool s = digitalRead(IR_South) == LOW;
    bool e = digitalRead(IR_East)  == LOW;
    bool w = digitalRead(IR_West)  == LOW;

    String direction = "ND";
    if (e && n)      direction = "NE";
    else if (e && s) direction = "SE";
    else if (w && s) direction = "SW";
    else if (w && n) direction = "NW";
    else if (n)      direction = "N";
    else if (s)      direction = "S";
    else if (w)      direction = "W";
    else if (e)      direction = "E";

    float humidity = dht.readHumidity();
    float temp = dht.readTemperature();

    int rainValue = analogRead(RAIN_ANALOG_PIN);
    String rainStatus;

    if (rainValue < 300) {
      rainStatus = "Heavy";
      digitalWrite(BUZZER_RAIN, HIGH);
    } else {
      digitalWrite(BUZZER_RAIN, LOW);
      rainStatus = (rainValue < 600) ? "Light" : "Dry";
    }

    if (windSpeedKmh >= 40.0) {
      digitalWrite(BUZZER_WIND, HIGH);
    } else {
      digitalWrite(BUZZER_WIND, LOW);
    }

    // Update LCD
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WS:");
    lcd.print(windSpeedKmh, 1);
    lcd.print("km/h WD:");
    lcd.print(direction);

    lcd.setCursor(0, 1);
    lcd.print("H:");
    lcd.print((int)humidity);
    lcd.print("% T:");
    lcd.print((int)temp);
    lcd.print(" R:");
    lcd.print(rainStatus.charAt(0));

    // --- Blynk virtual pins update ---
    Blynk.virtualWrite(V0, windSpeedKmh);       // Wind Speed (km/h)
    Blynk.virtualWrite(V1, direction);          // Wind Direction (string)
    Blynk.virtualWrite(V2, temp);                // Temperature (°C)
    Blynk.virtualWrite(V3, humidity);            // Humidity (%)
    Blynk.virtualWrite(V4, rainValue);           // Rain analog value
    Blynk.virtualWrite(V5, rainStatus);          // Rain status string
    Blynk.virtualWrite(V6, digitalRead(BUZZER_RAIN)); // Rain buzzer state
    Blynk.virtualWrite(V7, digitalRead(BUZZER_WIND)); // Wind buzzer state

    lastCalcTime = currentTime;
  }
}

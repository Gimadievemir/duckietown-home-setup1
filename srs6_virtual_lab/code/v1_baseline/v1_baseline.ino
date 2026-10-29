// СРС №6 — исходная (базовая) версия скетча, v1
// Arduino UNO + HC-SR04 + 3 LED + кнопка (D2 -> GND, INPUT_PULLUP)
// Эта версия сохраняется без изменений как точка отсчёта для сравнения до/после.

const int TRIG_PIN   = 9;
const int ECHO_PIN   = 10;
const int BUTTON_PIN = 2;
const int RED_PIN    = 13;  // < 20 см
const int YELLOW_PIN = 12;  // 20..50 см включительно
const int GREEN_PIN  = 11;  // > 50 см

void allOff() {
  digitalWrite(RED_PIN, LOW);
  digitalWrite(YELLOW_PIN, LOW);
  digitalWrite(GREEN_PIN, LOW);
}

void setup() {
  Serial.begin(9600);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);  // отпущена = HIGH, нажата = LOW
  pinMode(RED_PIN, OUTPUT);
  pinMode(YELLOW_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  allOff();
  Serial.println("v1 baseline started");
}

void loop() {
  int button = digitalRead(BUTTON_PIN);

  if (button == HIGH) {             // кнопка отпущена: измерения прекращены
    allOff();
    Serial.println("D2=HIGH released -> LEDs OFF");
    delay(300);
    return;
  }

  // Импульс запуска 10 мкс
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  unsigned long duration = pulseIn(ECHO_PIN, HIGH);  // тайм-аут по умолчанию 1 с

  if (duration == 0) {              // 0 = эхо не получено, а НЕ расстояние 0 см
    allOff();
    Serial.println("D2=LOW ERROR: no echo -> LEDs OFF");
    delay(300);
    return;
  }

  float distance = duration * 0.0343 / 2;

  allOff();
  const char *led;
  if (distance < 20) {
    digitalWrite(RED_PIN, HIGH);
    led = "RED";
  } else if (distance <= 50) {
    digitalWrite(YELLOW_PIN, HIGH);
    led = "YELLOW";
  } else {
    digitalWrite(GREEN_PIN, HIGH);
    led = "GREEN";
  }

  Serial.print("D2=LOW duration=");
  Serial.print(duration);
  Serial.print("us distance=");
  Serial.print(distance, 2);
  Serial.print("cm LED=");
  Serial.println(led);

  delay(300);
}

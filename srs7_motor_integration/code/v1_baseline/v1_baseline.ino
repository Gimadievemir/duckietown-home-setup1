// СРС №7 — исходная (базовая) версия, v1
// Arduino UNO + HC-SR04 + 3 LED + кнопка + L298N + DC-мотор.
// Логика датчика взята из v1 СРС №6 (блокирующий pulseIn, delay) плюс урок
// СРС №6: пороги сравниваются с расстоянием, округлённым до целого сантиметра.
// Эта версия сохраняется без изменений как точка отсчёта для сравнения до/после.

const int TRIG_PIN   = 9;
const int ECHO_PIN   = 10;
const int BUTTON_PIN = 2;
const int RED_PIN    = 13;  // < 20 см
const int YELLOW_PIN = 12;  // 20..50 см включительно
const int GREEN_PIN  = 11;  // > 50 см
const int IN1_PIN    = 5;
const int IN2_PIN    = 6;
const int ENA_PIN    = 3;   // PWM (Timer2)

const int SPEED_SLOW = 120; // ≈47 %
const int SPEED_FAST = 255; // 100 %

void allOff() {
  digitalWrite(RED_PIN, LOW);
  digitalWrite(YELLOW_PIN, LOW);
  digitalWrite(GREEN_PIN, LOW);
}

void motorStop() {
  digitalWrite(IN1_PIN, LOW);
  digitalWrite(IN2_PIN, LOW);
  analogWrite(ENA_PIN, 0);
}

void motorForward(int pwm) {
  digitalWrite(IN1_PIN, HIGH);
  digitalWrite(IN2_PIN, LOW);
  analogWrite(ENA_PIN, pwm);
}

void setup() {
  Serial.begin(9600);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);  // отпущена = HIGH, нажата = LOW
  pinMode(RED_PIN, OUTPUT);
  pinMode(YELLOW_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(IN1_PIN, OUTPUT);
  pinMode(IN2_PIN, OUTPUT);
  pinMode(ENA_PIN, OUTPUT);
  allOff();
  motorStop();
  Serial.println("v1 baseline started");
}

void loop() {
  if (digitalRead(BUTTON_PIN) == HIGH) {   // кнопка отпущена: всё остановлено
    allOff();
    motorStop();
    Serial.println("D2=HIGH released -> MOTOR=STOP LEDs OFF");
    delay(300);
    return;
  }

  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  unsigned long duration = pulseIn(ECHO_PIN, HIGH);  // тайм-аут по умолчанию 1 с

  if (duration == 0) {                     // эха нет: это не 0 см и не «свободно»
    allOff();
    motorStop();
    Serial.println("D2=LOW ERROR: no echo -> MOTOR=STOP LEDs OFF");
    delay(300);
    return;
  }

  float distance = duration * 0.0343 / 2;
  long cm = lround(distance);

  allOff();
  const char *led;
  const char *motor;
  int pwm;
  if (cm < 20) {
    digitalWrite(RED_PIN, HIGH);
    motorStop();
    led = "RED"; motor = "STOP"; pwm = 0;
  } else if (cm <= 50) {
    digitalWrite(YELLOW_PIN, HIGH);
    motorForward(SPEED_SLOW);
    led = "YELLOW"; motor = "FWD"; pwm = SPEED_SLOW;
  } else {
    digitalWrite(GREEN_PIN, HIGH);
    motorForward(SPEED_FAST);
    led = "GREEN"; motor = "FWD"; pwm = SPEED_FAST;
  }

  Serial.print("D2=LOW distance=");
  Serial.print(distance, 2);
  Serial.print("cm LED=");
  Serial.print(led);
  Serial.print(" MOTOR=");
  Serial.print(motor);
  Serial.print(" PWM=");
  Serial.println(pwm);

  delay(300);
}

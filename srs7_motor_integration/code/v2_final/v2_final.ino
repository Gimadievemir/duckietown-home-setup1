// СРС №7 — финальная исправленная версия, v2
// Изменения относительно v1 (см. REPORT.md):
//  1) явный тайм-аут pulseIn(ECHO_PIN, HIGH, 30000) и признак validMeasurement;
//  2) первый же неудачный пинг сразу останавливает мотор (fail-safe), движение
//     возобновляется только после полной серии корректных замеров;
//  3) антидребезг кнопки на millis() без delay(), loop() не блокируется;
//  4) медиана из 5 замеров + мажоритарная проверка (нужно >= 3 корректных);
//  5) ШИМ на ENA (D3, Timer2) поднят с ~490 Гц до ~3.9 кГц.

const int TRIG_PIN   = 9;
const int ECHO_PIN   = 10;
const int BUTTON_PIN = 2;
const int RED_PIN    = 13;  // < 20 см
const int YELLOW_PIN = 12;  // 20..50 см включительно
const int GREEN_PIN  = 11;  // > 50 см
const int IN1_PIN    = 5;
const int IN2_PIN    = 6;
const int ENA_PIN    = 3;   // PWM (Timer2, OC2B)

const int SPEED_SLOW = 120; // ≈47 %
const int SPEED_FAST = 255; // 100 %

const unsigned long ECHO_TIMEOUT_US = 30000;
const unsigned long DEBOUNCE_MS     = 30;
const unsigned long PING_PERIOD_MS  = 60;
const unsigned long REPORT_MS       = 300;
const int   SAMPLES       = 5;
const int   MIN_VALID     = 3;
const float MIN_RANGE_CM  = 2.0;
const float MAX_RANGE_CM  = 400.0;

// --- кнопка ---
int lastRaw = HIGH;
int stableState = HIGH;
unsigned long lastEdgeMs = 0;

// --- измерения ---
float samples[SAMPLES];
int sampleCount = 0;
int validCount = 0;
unsigned long lastPingMs = 0;
unsigned long lastReportMs = 0;

bool haveResult = false;
bool validMeasurement = false;
float distanceCm = 0;

// --- текущее состояние выходов (для отчёта в Serial) ---
const char *ledState = "OFF";
const char *motorState = "STOP";
int motorPwm = 0;

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

// Безопасное состояние: мотор стоит, LED выключены.
void safeStop() {
  allOff();
  motorStop();
  ledState = "OFF";
  motorState = "STOP";
  motorPwm = 0;
}

void applyOutputs() {
  if (!validMeasurement) {
    safeStop();
    return;
  }
  long cm = lround(distanceCm);
  allOff();
  if (cm < 20) {
    digitalWrite(RED_PIN, HIGH);
    motorStop();
    ledState = "RED"; motorState = "STOP"; motorPwm = 0;
  } else if (cm <= 50) {
    digitalWrite(YELLOW_PIN, HIGH);
    motorForward(SPEED_SLOW);
    ledState = "YELLOW"; motorState = "FWD"; motorPwm = SPEED_SLOW;
  } else {
    digitalWrite(GREEN_PIN, HIGH);
    motorForward(SPEED_FAST);
    ledState = "GREEN"; motorState = "FWD"; motorPwm = SPEED_FAST;
  }
}

void updateButton(unsigned long now) {
  int raw = digitalRead(BUTTON_PIN);
  if (raw != lastRaw) {
    lastRaw = raw;
    lastEdgeMs = now;
  }
  if (now - lastEdgeMs >= DEBOUNCE_MS && raw != stableState) {
    stableState = raw;
  }
}

bool ping(float &cm) {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  unsigned long duration = pulseIn(ECHO_PIN, HIGH, ECHO_TIMEOUT_US);
  if (duration == 0) return false;            // тайм-аут: эха нет
  cm = duration * 0.0343 / 2;
  return cm >= MIN_RANGE_CM && cm <= MAX_RANGE_CM;
}

float median(float *a, int n) {
  for (int i = 1; i < n; i++) {
    float v = a[i];
    int j = i - 1;
    while (j >= 0 && a[j] > v) { a[j + 1] = a[j]; j--; }
    a[j + 1] = v;
  }
  return (n % 2) ? a[n / 2] : (a[n / 2 - 1] + a[n / 2]) / 2;
}

void resetMeasurement() {
  sampleCount = 0;
  validCount = 0;
  haveResult = false;
  validMeasurement = false;
}

void setup() {
  Serial.begin(9600);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(RED_PIN, OUTPUT);
  pinMode(YELLOW_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(IN1_PIN, OUTPUT);
  pinMode(IN2_PIN, OUTPUT);
  pinMode(ENA_PIN, OUTPUT);
  // Timer2: делитель 64 -> 8, частота ШИМ на D3 и D11 ~490 Гц -> ~3.9 кГц.
  TCCR2B = (TCCR2B & 0xF8) | 0x02;
  safeStop();
  Serial.println("v2 final started");
}

void loop() {
  unsigned long now = millis();
  updateButton(now);

  if (stableState == HIGH) {                  // отпущена: стоп, замеры приостановлены
    resetMeasurement();
    safeStop();
  } else if (now - lastPingMs >= PING_PERIOD_MS) {
    lastPingMs = now;
    float cm;
    if (ping(cm)) {
      samples[validCount++] = cm;
    } else {
      validMeasurement = false;               // fail-safe: не ждём конца серии
      safeStop();
    }
    sampleCount++;
    if (sampleCount == SAMPLES) {
      haveResult = true;
      validMeasurement = validCount >= MIN_VALID;
      if (validMeasurement) distanceCm = median(samples, validCount);
      applyOutputs();
      sampleCount = 0;
      validCount = 0;
    }
  }

  if (now - lastReportMs >= REPORT_MS) {
    lastReportMs = now;
    Serial.print(stableState == HIGH ? "D2=HIGH" : "D2=LOW");
    if (stableState == HIGH) {
      Serial.print(" idle");
    } else if (!haveResult) {
      Serial.print(" measuring...");
    } else if (validMeasurement) {
      Serial.print(" valid=1 median=");
      Serial.print(distanceCm, 2);
      Serial.print("cm");
    } else {
      Serial.print(" valid=0 ERROR: no valid echo -> SAFE STOP");
    }
    Serial.print(" LED=");
    Serial.print(ledState);
    Serial.print(" MOTOR=");
    Serial.print(motorState);
    Serial.print(" PWM=");
    Serial.println(motorPwm);
  }
}

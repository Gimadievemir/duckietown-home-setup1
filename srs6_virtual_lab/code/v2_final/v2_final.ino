// СРС №6 — финальная исправленная версия скетча, v2
// Изменения относительно v1 (см. REPORT.md, этап 5):
//  1) явный тайм-аут pulseIn(ECHO_PIN, HIGH, 30000);
//  2) признак validMeasurement вместо подмены ошибки числом (0 / 999 см);
//  3) антидребезг кнопки на millis() без delay();
//  4) медиана из 5 замеров + мажоритарная проверка (нужно >= 3 корректных);
//  5) сравнение с порогами по расстоянию, округлённому до целого сантиметра.
// Ошибка датчика переводит систему в безопасное состояние: все LED выключены,
// для будущего двигателя это команда STOP, а не «путь свободен».

const int TRIG_PIN   = 9;
const int ECHO_PIN   = 10;
const int BUTTON_PIN = 2;
const int RED_PIN    = 13;  // < 20 см
const int YELLOW_PIN = 12;  // 20..50 см включительно
const int GREEN_PIN  = 11;  // > 50 см

const unsigned long ECHO_TIMEOUT_US = 30000;  // ~5 м туда-обратно, дальше HC-SR04 не измеряет
const unsigned long DEBOUNCE_MS     = 30;
const unsigned long PING_PERIOD_MS  = 60;     // пауза между пингами, чтобы не ловить старое эхо
const unsigned long REPORT_MS       = 300;
const int   SAMPLES       = 5;
const int   MIN_VALID     = 3;                // большинство из 5
const float MIN_RANGE_CM  = 2.0;
const float MAX_RANGE_CM  = 400.0;

// --- кнопка ---
int lastRaw = HIGH;
int stableState = HIGH;
unsigned long lastEdgeMs = 0;
unsigned long rawEdges = 0;        // все сырые переключения (включая дребезг)
unsigned long debouncedPresses = 0;

// --- измерения ---
float samples[SAMPLES];
int sampleCount = 0;
int validCount = 0;
unsigned long lastPingMs = 0;
unsigned long lastReportMs = 0;

bool haveResult = false;           // хотя бы одна серия из 5 замеров завершена
bool validMeasurement = false;
float distanceCm = 0;

void allOff() {
  digitalWrite(RED_PIN, LOW);
  digitalWrite(YELLOW_PIN, LOW);
  digitalWrite(GREEN_PIN, LOW);
}

void updateButton(unsigned long now) {
  int raw = digitalRead(BUTTON_PIN);
  if (raw != lastRaw) {
    lastRaw = raw;
    lastEdgeMs = now;
    rawEdges++;
  }
  if (now - lastEdgeMs >= DEBOUNCE_MS && raw != stableState) {
    stableState = raw;
    if (stableState == LOW) debouncedPresses++;
  }
}

// Один пинг. Возвращает true и расстояние, если эхо корректное.
bool ping(float &cm) {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  unsigned long duration = pulseIn(ECHO_PIN, HIGH, ECHO_TIMEOUT_US);
  if (duration == 0) return false;            // тайм-аут: эха нет, это НЕ 0 см
  cm = duration * 0.0343 / 2;
  return cm >= MIN_RANGE_CM && cm <= MAX_RANGE_CM;
}

float median(float *a, int n) {
  for (int i = 1; i < n; i++) {               // сортировка вставками, n <= 5
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

void showLeds() {
  allOff();
  if (!validMeasurement) return;              // безопасное состояние
  long cm = lround(distanceCm);               // пороги заданы в целых сантиметрах
  if (cm < 20)        digitalWrite(RED_PIN, HIGH);
  else if (cm <= 50)  digitalWrite(YELLOW_PIN, HIGH);
  else                digitalWrite(GREEN_PIN, HIGH);
}

const char *ledName() {
  if (!validMeasurement) return "OFF";
  long cm = lround(distanceCm);
  if (cm < 20) return "RED";
  if (cm <= 50) return "YELLOW";
  return "GREEN";
}

void setup() {
  Serial.begin(9600);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);          // отпущена = HIGH, нажата = LOW
  pinMode(RED_PIN, OUTPUT);
  pinMode(YELLOW_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  allOff();
  Serial.println("v2 final started");
}

void loop() {
  unsigned long now = millis();
  updateButton(now);

  if (stableState == HIGH) {                  // отпущена: измерения прекращены
    resetMeasurement();
    allOff();
  } else if (now - lastPingMs >= PING_PERIOD_MS) {
    lastPingMs = now;
    float cm;
    if (ping(cm)) samples[validCount++] = cm;
    sampleCount++;
    if (sampleCount == SAMPLES) {
      haveResult = true;
      validMeasurement = validCount >= MIN_VALID;
      if (validMeasurement) distanceCm = median(samples, validCount);
      showLeds();
      sampleCount = 0;
      validCount = 0;
    }
  }

  if (now - lastReportMs >= REPORT_MS) {
    lastReportMs = now;
    Serial.print(stableState == HIGH ? "D2=HIGH" : "D2=LOW");
    Serial.print(" edges=");
    Serial.print(rawEdges);
    Serial.print(" presses=");
    Serial.print(debouncedPresses);
    if (stableState == HIGH) {
      Serial.println(" idle LED=OFF");
    } else if (!haveResult) {
      Serial.println(" measuring... LED=OFF");
    } else if (validMeasurement) {
      Serial.print(" valid=1 median=");
      Serial.print(distanceCm, 2);
      Serial.print("cm LED=");
      Serial.println(ledName());
    } else {
      Serial.println(" valid=0 ERROR: no valid echo -> SAFE STOP, LED=OFF");
    }
  }
}

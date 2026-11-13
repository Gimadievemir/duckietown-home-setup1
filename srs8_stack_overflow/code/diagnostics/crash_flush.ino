// СРС №8 — диагностика: crash.ino + Serial.flush() после каждого вывода.

volatile uint8_t controlValue = 0;

__attribute__((noinline))
void deepRecursion(uint16_t depth) {
  volatile uint8_t localData[16];
  localData[0] = depth;

  if (depth % 10 == 0) {
    Serial.println(depth);
    Serial.flush();  // ДИАГНОСТИКА: дождаться отправки, пока буфер TX цел
  }

  deepRecursion(depth + 1);

  // Действие после вызова препятствует замене рекурсии циклом
  controlValue ^= localData[0];
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("START");
  deepRecursion(1);
}

void loop() {}

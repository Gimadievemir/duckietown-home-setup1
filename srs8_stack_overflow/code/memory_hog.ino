// СРС №8 — Memory Hog: вложенные вызовы, у каждого локальный буфер ~600 байт.
// Одновременно существуют кадры memoryHog(START_LEVEL) … memoryHog(0),
// всего START_LEVEL + 1 буферов. Эксперимент проводился для START_LEVEL = 0…4.
// Запускать ТОЛЬКО в Wokwi.

const uint8_t START_LEVEL = 4;

volatile int controlSum = 0;

__attribute__((noinline))
void memoryHog(uint8_t level) {
  volatile int buffer[300];  // 300 × 2 байта ≈ 600 байт

  for (int i = 0; i < 300; i++) {
    buffer[i] = i + level;
  }

  Serial.print("level = ");
  Serial.println(level);

  if (level > 0) {
    memoryHog(level - 1);
  }

  controlSum += buffer[level];
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("MEMORY HOG START");
  memoryHog(START_LEVEL);
  Serial.println("MEMORY HOG FINISH");
}

void loop() {}

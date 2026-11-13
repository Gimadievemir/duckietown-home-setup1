// СРС №8 — дополнительная диагностика (не входит в обязательные файлы).
// Измеряет реальное расположение памяти и размер кадра deepRecursion():
// печатает указатель стека SP на нескольких уровнях рекурсии, ограниченной PROBE_DEPTH.
// Тело функции повторяет crash.ino, но глубина ограничена, поэтому сбоя нет.

extern int __data_start, __bss_end, __heap_start;
extern char *__brkval;

volatile uint8_t controlValue = 0;
const uint16_t PROBE_DEPTH = 5;
uint16_t spAt[PROBE_DEPTH + 1];

__attribute__((noinline))
void deepRecursion(uint16_t depth) {
  volatile uint8_t localData[16];
  localData[0] = depth;
  spAt[depth] = SP;

  if (depth < PROBE_DEPTH) {
    deepRecursion(depth + 1);
  }

  controlValue ^= localData[0];
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("STACK PROBE");
  Serial.print("RAMSTART=");      Serial.println((uint16_t)RAMSTART);
  Serial.print("RAMEND=");        Serial.println((uint16_t)RAMEND);
  Serial.print("__data_start=");  Serial.println((uint16_t)&__data_start);
  Serial.print("__bss_end=");     Serial.println((uint16_t)&__bss_end);
  Serial.print("__heap_start=");  Serial.println((uint16_t)&__heap_start);
  Serial.print("__brkval=");      Serial.println((uint16_t)__brkval);
  Serial.print("static bytes=");  Serial.println((uint16_t)&__bss_end - (uint16_t)RAMSTART);
  Serial.print("SP in setup=");   Serial.println(SP);

  deepRecursion(1);
  for (uint16_t d = 1; d <= PROBE_DEPTH; d++) {
    Serial.print("depth ");
    Serial.print(d);
    Serial.print(" SP=");
    Serial.print(spAt[d]);
    if (d > 1) {
      Serial.print(" frame=");
      Serial.print(spAt[d - 1] - spAt[d]);
    }
    Serial.println();
  }
  Serial.println("PROBE FINISH");
}

void loop() {}

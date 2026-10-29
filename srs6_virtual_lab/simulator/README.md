# Проект Wokwi

- `diagram.json`: исправная базовая схема (HC-SR04: VCC→5V, GND→GND, TRIG→D9, ECHO→D10; кнопка D2↔GND; LED D13/D12/D11 через свои 220 Ом).
- Публичный проект (скетч v2): https://wokwi.com/projects/479239677376697345

Как воспроизвести: wokwi.com → New Project → Arduino Uno. Вставить `diagram.json` и скетч из `../code/v2_final/v2_final.ino` (или v1), нажать Start. Чтобы задать расстояние, кликнуть по HC-SR04 во время симуляции.

Варианты для экспериментов (вносить по одному):
- A: в скетче `ECHO_PIN = 8`.
- B: `pinMode(BUTTON_PIN, INPUT)`.
- C: в `attrs` кнопки добавить `"bounce": "0"` и сравнить с исходным вариантом.
- D: удалить строку `[ "sonar:ECHO", "uno:10", ... ]`.
- E: поменять местами условия `< 20` и `<= 50`.

Серийные логи всех прогонов: [serial_log_excerpts.txt](serial_log_excerpts.txt).

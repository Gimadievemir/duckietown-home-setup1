# СРС № 8. Ограничения стека: рекурсия и переполнение SRAM

- **ФИО:** Эмир Гимадиев
- **Группа:** БПЛА-25-6

**Назначение работы:** исследовать на Arduino Uno в Wokwi, как ограниченная SRAM (2 КБ) влияет на рекурсию. Задачи: распознать признаки переполнения стека, заменить рекурсию циклом с постоянным расходом стека и показать на Memory Hog, что память суммируется только для одновременно существующих кадров.

- Отчёт: [REPORT.md](REPORT.md)
- Прогноз до запуска: [PREDICTION.md](PREDICTION.md)
- Проект Wokwi: https://wokwi.com/projects/480595536079967233
- Логи Serial: [serial_logs.txt](serial_logs.txt)

```
srs8_stack_overflow/
├── README.md
├── REPORT.md
├── PREDICTION.md
├── serial_logs.txt
├── code/
│   ├── crash.ino
│   ├── safe.ino
│   ├── memory_hog.ino
│   └── diagnostics/        (дополнительно: stack_probe, crash_flush, crash_flush_sp)
└── screenshots/
    ├── crash_result.png
    ├── safe_result.png
    ├── memory_hog_result.png
    ├── memory_hog_level2_hang.png
    └── memory_hog_level3_reset_loop.png
```

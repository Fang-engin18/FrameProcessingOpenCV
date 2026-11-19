Схема: Python = камера + bounding boxes. C++ = сектора + зона + битовая маска.

1) Сделайте резервную копию исходных файлов проекта.
2) Скопируйте main.cpp, sectors.cpp, sectors.h, spray_zone.*, CMakeLists.txt,
   spray_config.txt, hand_tracking.py в корень weed_box_parser.
3) Окно PowerShell #1: .\go.bat (сначала C++ сервер).
4) Окно PowerShell #2: .\.venv\Scripts\Activate.ps1 ; python hand_tracking.py
5) Изменения геометрии: spray_config.txt, затем перезапустить C++.

UDP порт: 5005, только loopback 127.0.0.1.
Python -> C++: FRAME width height count x y w h [x y w h ...]
C++ -> Python: STATE sector_count zone_top zone_bottom overlap_percent mask_decimal

Сектора нумеруются слева направо с нуля, бит 0 — левый сектор.
Для 3 секторов: 001=левый, 010=центр, 100=правый, 111=все.

ВАЖНО: COM/ESP32 не подключены на этом этапе. Тайм-аут C++ обнуляет
маску в памяти, а Python показывает OFFLINE при отсутствии ответа.
Для реальных клапанов понадобится отдельный watchdog на ESP32.

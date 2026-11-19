#pragma once
#include <cstdint>
#include <string>

// Старая функция сохранена без изменения интерфейса (3 сектора).
std::string getSectors(int frameWidth, int x, int y, int width, int height,
                       double minOverlapPercent);

// Новая функция: произвольное количество равных секторов от 1 до 16.
// Бит i = 1, если площадь пересечения рамки с сектором i
// не меньше minOverlapPercent процентов площади рамки.
std::uint32_t getSectorMask(int frameWidth, int x, int y, int width, int height,
                            double minOverlapPercent, int sectorCount);

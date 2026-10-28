
// Определяет, в каких вертикальных секторах находится объект.
// Если пересечение с сектором меньше 5% площади объекта,
// этот сектор не включается в результат.
//
// Возвращает, например: "left", "left;center" или "right".

#pragma once

#include <string>

std::string getSectors(
    int frameWidth,
    int x,
    int y,
    int width,
    int height
);


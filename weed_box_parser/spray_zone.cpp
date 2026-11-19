#include "spray_zone.h"

// Исходная функция пользователя сохранена.
int isInSprayZone(
    int frameHeight, int y, int objectHeight,
    double topPercent, double bottomPercent
) {
    if (frameHeight <= 0 || objectHeight <= 0) return 0;
    if (topPercent < 0.0 || topPercent > 100.0 ||
        bottomPercent < 0.0 || bottomPercent > 100.0 ||
        topPercent >= bottomPercent) return 0;

    const double y0 = y + objectHeight;
    const double zoneTop = frameHeight * topPercent / 100.0;
    const double zoneBottom = frameHeight * bottomPercent / 100.0;
    return (y0 >= zoneTop && y0 <= zoneBottom) ? 1 : 0;
}

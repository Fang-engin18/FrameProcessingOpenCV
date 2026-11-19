#include "sectors.h"
#include <algorithm>
#include <string>

// Исходная функция пользователя сохранена.
std::string getSectors(
    int frameWidth,
    int x,
    int y,
    int width,
    int height,
    double minOverlapPercent
) {
    if (frameWidth <= 0 || width <= 0 || height <= 0) return "";
    if (minOverlapPercent < 0.0 || minOverlapPercent > 100.0) return "";

    const double bounds[] = {
        0.0, frameWidth / 3.0, 2.0 * frameWidth / 3.0,
        static_cast<double>(frameWidth)
    };
    const double boxLeft = x;
    const double boxRight = x + width;
    const double boxArea = static_cast<double>(width) * height;
    const char* names[] = {"left", "center", "right"};
    std::string result;

    for (int i = 0; i < 3; ++i) {
        const double overlapLeft = std::max(boxLeft, bounds[i]);
        const double overlapRight = std::min(boxRight, bounds[i + 1]);
        const double overlapWidth = std::max(0.0, overlapRight - overlapLeft);
        const double overlapArea = overlapWidth * height;
        const double percentage = (overlapArea / boxArea) * 100.0;
        if (percentage >= minOverlapPercent) {
            if (!result.empty()) result += ";";
            result += names[i];
        }
    }
    return result;
}

std::uint32_t getSectorMask(int frameWidth, int x, int y, int width, int height,
                            double minOverlapPercent, int sectorCount) {
    if (frameWidth <= 0 || width <= 0 || height <= 0 ||
        sectorCount < 1 || sectorCount > 16 ||
        minOverlapPercent < 0.0 || minOverlapPercent > 100.0) {
        return 0;
    }

    const double boxLeft = static_cast<double>(x);
    const double boxRight = boxLeft + static_cast<double>(width);
    const double boxArea = static_cast<double>(width) * height;
    std::uint32_t mask = 0;

    for (int i = 0; i < sectorCount; ++i) {
        const double sectorLeft = static_cast<double>(frameWidth) * i / sectorCount;
        const double sectorRight = static_cast<double>(frameWidth) * (i + 1) / sectorCount;
        const double overlapWidth = std::max(0.0,
            std::min(boxRight, sectorRight) - std::max(boxLeft, sectorLeft));
        const double percentage = overlapWidth * height / boxArea * 100.0;
        // Требуем ненулевое реальное пересечение даже при пороге 0%.
        if (overlapWidth > 0.0 && percentage >= minOverlapPercent) {
            mask |= (std::uint32_t{1} << i);
        }
    }
    return mask;
}

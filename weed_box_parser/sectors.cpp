
#include "sectors.h"

#include <algorithm>

std::string getSectors(
    int frameWidth,
    int x,
    int y,
    int width,
    int height
) {
    if (frameWidth <= 0 || width <= 0 || height <= 0) {
        return "";
    }

    // Границы трех математически равных секторов.
    const double bounds[] = {
        0.0,
        frameWidth / 3.0,
        2.0 * frameWidth / 3.0,
        static_cast<double>(frameWidth)
    };

    const double boxLeft = x;
    const double boxRight = x + width;

    const double boxArea =
        static_cast<double>(width) * height;

    const char* names[] = {
        "left",
        "center",
        "right"
    };

    std::string result;

    for (int i = 0; i < 3; ++i) {
        const double overlapLeft =
            std::max(boxLeft, bounds[i]);

        const double overlapRight =
            std::min(boxRight, bounds[i + 1]);

        const double overlapWidth =
            std::max(0.0, overlapRight - overlapLeft);

        const double overlapArea =
            overlapWidth * height;

        const double percentage =
            overlapArea / boxArea;

        if (percentage >= 0.05) {
            if (!result.empty()) {
                result += ";";
            }

            result += names[i];
        }
    }

    return result;
}

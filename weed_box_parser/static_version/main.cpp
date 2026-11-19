#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/geometry/2d.hpp>

#include <iostream>
#include <filesystem>
#include <fstream>
#include <vector>
#include <algorithm>
#include <string>
#include <cmath>

#include "sectors.h"
#include "spray_zone.h"

namespace fs = std::filesystem;


// ======================================================
// БЛОК НАСТРОЕК
// ======================================================

// Минимальный процент площади объекта,
// который должен попадать в сектор.
const double SECTOR_OVERLAP_PERCENT = 5.0;

// Верхняя граница зоны опрыскивания
// в процентах от высоты изображения.
const double SPRAY_ZONE_TOP_PERCENT = 40.0;

// Нижняя граница зоны опрыскивания
// в процентах от высоты изображения.
const double SPRAY_ZONE_BOTTOM_PERCENT = 60.0;

// ======================================================


struct DetectionBox {
    int x;
    int y;
    int width;
    int height;
    double confidence;
};


int main() {

    const fs::path inputDir = "input";
    const fs::path outputDir = "output";
    const fs::path csvPath = outputDir / "coordinates.csv";

    if (!fs::exists(inputDir)) {
        std::cerr << "Input directory not found!\n";
        return 1;
    }

    fs::create_directories(outputDir);

    std::ofstream csv(csvPath);

    if (!csv.is_open()) {
        std::cerr << "Cannot open CSV file!\n";
        return 1;
    }

    csv << "image,x,y,width,height,x2,y2,confidence,sectors,y0,spray\n";

    int processedImages = 0;

    for (const auto& entry : fs::directory_iterator(inputDir)) {

        if (!entry.is_regular_file()) {
            continue;
        }

        const fs::path imagePath = entry.path();

        std::string extension = imagePath.extension().string();

        std::transform(
            extension.begin(),
            extension.end(),
            extension.begin(),
            [](unsigned char c) {
                return static_cast<char>(std::tolower(c));
            }
        );

        if (extension != ".jpg" &&
            extension != ".jpeg" &&
            extension != ".png" &&
            extension != ".bmp" &&
            extension != ".webp") {
            continue;
        }

        cv::Mat image = cv::imread(imagePath.string());

        if (image.empty()) {
            std::cerr << "Cannot read image: "
                      << imagePath.filename() << '\n';
            continue;
        }

        const std::string filename =
            imagePath.filename().string();

        std::cout << "Processing: " << filename << '\n';

        // --------------------------------------------------
        // 1. ПЕРЕВОД ИЗОБРАЖЕНИЯ В HSV
        // --------------------------------------------------

        cv::Mat hsv;
        cv::cvtColor(image, hsv, cv::COLOR_BGR2HSV);

        // --------------------------------------------------
        // 2. МАСКИ ЦВЕТОВ
        // --------------------------------------------------

        cv::Mat turquoiseMask;
        cv::Mat blueMask;
        cv::Mat mask;

        cv::inRange(
            hsv,
            cv::Scalar(75, 60, 60),
            cv::Scalar(105, 255, 255),
            turquoiseMask
        );

        cv::inRange(
            hsv,
            cv::Scalar(90, 60, 60),
            cv::Scalar(135, 255, 255),
            blueMask
        );

        cv::bitwise_or(
            turquoiseMask,
            blueMask,
            mask
        );

        // --------------------------------------------------
        // 3. МОРФОЛОГИЧЕСКАЯ ОБРАБОТКА
        // --------------------------------------------------

        cv::Mat kernel = cv::getStructuringElement(
            cv::MORPH_RECT,
            cv::Size(3, 3)
        );

        cv::morphologyEx(
            mask,
            mask,
            cv::MORPH_CLOSE,
            kernel
        );

        // --------------------------------------------------
        // 4. ПОИСК КОНТУРОВ
        // --------------------------------------------------

        std::vector<std::vector<cv::Point>> contours;

        cv::findContours(
            mask,
            contours,
            cv::RETR_EXTERNAL,
            cv::CHAIN_APPROX_SIMPLE
        );

        std::vector<DetectionBox> boxes;

        // --------------------------------------------------
        // 5. ФИЛЬТРАЦИЯ И СОХРАНЕНИЕ ПРЯМОУГОЛЬНИКОВ
        // --------------------------------------------------

        for (const auto& contour : contours) {

            const double area = cv::contourArea(contour);

            if (area <= 300.0) {
                continue;
            }

            const cv::Rect rect = cv::boundingRect(contour);

            if (rect.width < 15 || rect.height < 15) {
                continue;
            }

            if (rect.width > image.cols * 0.95 ||
                rect.height > image.rows * 0.95) {
                continue;
            }

            DetectionBox box;

            box.x = rect.x;
            box.y = rect.y;
            box.width = rect.width;
            box.height = rect.height;
            box.confidence = 1.0;

            boxes.push_back(box);
        }

        // --------------------------------------------------
        // 6. СОРТИРОВКА ОБЪЕКТОВ
        // --------------------------------------------------

        std::sort(
            boxes.begin(),
            boxes.end(),
            [](const DetectionBox& a, const DetectionBox& b) {

                if (a.y != b.y) {
                    return a.y < b.y;
                }

                return a.x < b.x;
            }
        );

        // --------------------------------------------------
        // 7. ПОДГОТОВКА ПРЕВЬЮ
        // --------------------------------------------------

        cv::Mat preview = image.clone();

        // Вертикальные границы секторов.
        const int sectorX1 = static_cast<int>(
            std::round(image.cols / 3.0)
        );

        const int sectorX2 = static_cast<int>(
            std::round(2.0 * image.cols / 3.0)
        );

        cv::line(
            preview,
            cv::Point(sectorX1, 0),
            cv::Point(sectorX1, image.rows - 1),
            cv::Scalar(255, 0, 0),
            2
        );

        cv::line(
            preview,
            cv::Point(sectorX2, 0),
            cv::Point(sectorX2, image.rows - 1),
            cv::Scalar(255, 0, 0),
            2
        );

        // Горизонтальные границы зоны опрыскивания.
        const int sprayZoneTop = static_cast<int>(
            std::round(
                image.rows * SPRAY_ZONE_TOP_PERCENT / 100.0
            )
        );

        const int sprayZoneBottom = static_cast<int>(
            std::round(
                image.rows * SPRAY_ZONE_BOTTOM_PERCENT / 100.0
            )
        );

        cv::line(
            preview,
            cv::Point(0, sprayZoneTop),
            cv::Point(image.cols - 1, sprayZoneTop),
            cv::Scalar(0, 255, 255),
            2
        );

        cv::line(
            preview,
            cv::Point(0, sprayZoneBottom),
            cv::Point(image.cols - 1, sprayZoneBottom),
            cv::Scalar(0, 255, 255),
            2
        );

        // --------------------------------------------------
        // 8. ОБРАБОТКА КАЖДОГО ОБЪЕКТА
        // --------------------------------------------------

        for (const auto& b : boxes) {

            const int x2 = b.x + b.width;
            const int y2 = b.y + b.height;

            // Определяем, какие вертикальные секторы
            // пересекаются с прямоугольником объекта.
            const std::string sectors = getSectors(
                image.cols,
                b.x,
                b.y,
                b.width,
                b.height,
                SECTOR_OVERLAP_PERCENT
            );

            // Нижняя точка прямоугольника объекта.
            const int y0 = b.y + b.height;

            // Проверяем попадание нижней точки
            // в горизонтальную зону опрыскивания.
            const int spray = isInSprayZone(
                image.rows,
                b.y,
                b.height,
                SPRAY_ZONE_TOP_PERCENT,
                SPRAY_ZONE_BOTTOM_PERCENT
            );

            // Рисуем прямоугольник объекта.
            cv::rectangle(
                preview,
                cv::Point(b.x, b.y),
                cv::Point(x2, y2),
                cv::Scalar(0, 255, 0),
                2
            );

            // Отмечаем нижнюю точку объекта.
            cv::circle(
                preview,
                cv::Point(b.x + b.width / 2, y0),
                4,
                cv::Scalar(0, 0, 255),
                -1
            );

            // Подпись объекта.
            const std::string label =
                "S:" + sectors + " Spray:" + std::to_string(spray);

            cv::putText(
                preview,
                label,
                cv::Point(b.x, std::max(15, b.y - 5)),
                cv::FONT_HERSHEY_SIMPLEX,
                0.45,
                cv::Scalar(0, 255, 0),
                1
            );

            // --------------------------------------------------
            // 9. ЗАПИСЬ В CSV
            // --------------------------------------------------

            csv << '"'
                << filename << "\","
                << b.x << ','
                << b.y << ','
                << b.width << ','
                << b.height << ','
                << x2 << ','
                << y2 << ','
                << b.confidence << ','
                << '"'
                << sectors << "\","
                << y0 << ','
                << spray << '\n';
        }

        // --------------------------------------------------
        // 10. СОХРАНЕНИЕ ПРЕВЬЮ
        // --------------------------------------------------

        const fs::path outputPath =
            outputDir / (imagePath.stem().string() + "_preview.jpg");

        cv::imwrite(outputPath.string(), preview);

        processedImages++;

        std::cout << "Detected objects: "
                  << boxes.size() << '\n';
    }

    csv.close();

    std::cout << "\n================================\n";
    std::cout << "Processed images: "
              << processedImages << '\n';
    std::cout << "CSV saved: "
              << csvPath << '\n';
    std::cout << "================================\n";

    return 0;
}
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/geometry/2d.hpp>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "sectors.h"

namespace fs = std::filesystem;

struct DetectionBox {
    int x;
    int y;
    int width;
    int height;
    double confidence;
};

int main() {
    // Папки проекта
    const fs::path inputDir = "input";
    const fs::path outputDir = "output";
    const fs::path csvPath = outputDir / "coordinates.csv";

    // Настройки поиска бирюзовых рамок в HSV
    const cv::Scalar lowerCyan(75, 60, 60);
    const cv::Scalar upperCyan(105, 255, 255);

    // Фильтры для найденных контуров
    const int minWidth = 15;
    const int minHeight = 15;
    const double minArea = 300.0;

    if (!fs::exists(inputDir) || !fs::is_directory(inputDir)) {
        std::cerr << "ERROR: input folder not found: "
                  << inputDir << '\n';
        std::cerr << "Place images in the 'image' folder.\n";
        return 1;
    }

    fs::create_directories(outputDir);

    std::ofstream csv(csvPath);
    if (!csv.is_open()) {
        std::cerr << "ERROR: cannot create CSV file.\n";
        return 1;
    }

    csv << "image,x,y,width,height,x2,y2,confidence,sectors\n";

    int imageCount = 0;
    int totalBoxes = 0;

    const std::vector<std::string> extensions = {
        ".jpg", ".jpeg", ".png", ".bmp", ".webp"
    };

    for (const auto& entry : fs::directory_iterator(inputDir)) {
        if (!entry.is_regular_file()) {
            continue;
        }

        std::string ext = entry.path().extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(),
            [](unsigned char c) {
                return static_cast<char>(std::tolower(c));
            });

        if (std::find(extensions.begin(), extensions.end(), ext)
            == extensions.end()) {
            continue;
        }

        const std::string filename = entry.path().filename().string();
        cv::Mat image = cv::imread(entry.path().string());

        if (image.empty()) {
            std::cerr << "Cannot read: " << filename << '\n';
            continue;
        }

        // Переводим изображение в HSV, чтобы выделить цвет рамок
        cv::Mat hsv;
        cv::cvtColor(image, hsv, cv::COLOR_BGR2HSV);

        // Маска бирюзовых рамок
        cv::Mat cyanMask;
        cv::inRange(hsv, lowerCyan, upperCyan, cyanMask);

        // Маска синих рамок
        const cv::Scalar lowerBlue(90, 60, 60);
        const cv::Scalar upperBlue(135, 255, 255);

        cv::Mat blueMask;
        cv::inRange(hsv, lowerBlue, upperBlue, blueMask);

        // Объединяем обе маски
        cv::Mat mask;
        cv::bitwise_or(cyanMask, blueMask, mask);

        // Соединяем небольшие разрывы в линиях рамок
        cv::Mat kernel = cv::getStructuringElement(
            cv::MORPH_RECT, cv::Size(3, 3));

        cv::morphologyEx(mask, mask, cv::MORPH_CLOSE, kernel);

        // Находим внешние контуры
        std::vector<std::vector<cv::Point>> contours;
        cv::findContours(
            mask,
            contours,
            cv::RETR_EXTERNAL,
            cv::CHAIN_APPROX_SIMPLE
        );

        std::vector<DetectionBox> boxes;

        for (const auto& contour : contours) {
            const double area = cv::contourArea(contour);

            if (area < minArea) {
                continue;
            }

            const cv::Rect rect = cv::boundingRect(contour);

            if (rect.width < minWidth || rect.height < minHeight) {
                continue;
            }

            // Пропускаем почти полноэкранные области
            if (rect.width > image.cols * 0.98 &&
                rect.height > image.rows * 0.98) {
                continue;
            }

            // Это оценка геометрической формы, не confidence модели
            const double confidence = 0.0;

            boxes.push_back({
                rect.x,
                rect.y,
                rect.width,
                rect.height,
                confidence
            });
        }

        // Сортируем рамки сверху вниз, затем слева направо
        std::sort(boxes.begin(), boxes.end(),
            [](const DetectionBox& a, const DetectionBox& b) {
                if (a.y != b.y) {
                    return a.y < b.y;
                }
                return a.x < b.x;
            });

        // Рисуем найденные прямоугольники на копии изображения
        cv::Mat preview = image.clone();
        const int sectorX1 = image.cols / 3;
        const int sectorX2 = (2 * image.cols) / 3;

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
        for (std::size_t i = 0; i < boxes.size(); ++i) {
            const auto& b = boxes[i];

            const int x2 = b.x + b.width;
            const int y2 = b.y + b.height;

            cv::rectangle(
                preview,
                cv::Rect(b.x, b.y, b.width, b.height),
                cv::Scalar(0, 255, 0),
                2
            );

            cv::putText(
                preview,
                std::to_string(i + 1),
                cv::Point(b.x, std::max(15, b.y - 5)),
                cv::FONT_HERSHEY_SIMPLEX,
                0.5,
                cv::Scalar(0, 255, 0),
                1
            );

            const std::string sectors = getSectors(
                image.cols,
                b.x,
                b.y,
                b.width,
                b.height
            );

            csv << '"' << filename << "\","
                << b.x << ','
                << b.y << ','
                << b.width << ','
                << b.height << ','
                << x2 << ','
                << y2 << ','
                << b.confidence << ','
                << '"' << sectors << '"' << '\n';
        }

        const fs::path previewPath =
            outputDir / (entry.path().stem().string() + "_detected.jpg");

        if (!cv::imwrite(previewPath.string(), preview)) {
            std::cerr << "Cannot save preview: "
                      << previewPath.string() << '\n';
        }

        std::cout << filename << ": "
                  << boxes.size() << " boxes found\n";

        ++imageCount;
        totalBoxes += static_cast<int>(boxes.size());
    }

    csv.close();

    std::cout << "\nFinished.\n";
    std::cout << "Images processed: " << imageCount << '\n';
    std::cout << "Total boxes found: " << totalBoxes << '\n';
    std::cout << "CSV: " << csvPath.string() << '\n';
    std::cout << "Previews: " << outputDir.string() << '\n';

    return 0;
}

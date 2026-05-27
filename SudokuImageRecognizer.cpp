#include "SudokuImageRecognizer.h"

#include <QCoreApplication>
#include <QFileInfo>
#include <QStandardPaths>
#include <opencv2/core/cuda.hpp>
#include <utility>

namespace {
std::pair<int, double> argmaxSoftmax(const cv::Mat& logits)
{
    cv::Mat row = logits.reshape(1, 1);
    cv::Mat scores;
    row.convertTo(scores, CV_32F);

    double maxLogit = 0.0;
    cv::minMaxLoc(scores, nullptr, &maxLogit);

    cv::Mat probs;
    cv::exp(scores - maxLogit, probs);
    probs /= cv::sum(probs)[0];

    cv::Point classId;
    double confidence = 0.0;
    cv::minMaxLoc(probs, nullptr, &confidence, nullptr, &classId);
    return {classId.x, confidence};
}
}

SudokuImageRecognizer::SudokuImageRecognizer()
{
    QString modelPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) +
                        "/OCR_Model/custom_model.onnx";
    if (!QFileInfo::exists(modelPath)) {
        modelPath = QCoreApplication::applicationDirPath() + "/OCR_Model/custom_model.onnx";
    }
    if (!QFileInfo::exists(modelPath)) {
        modelPath = "OCR_Model/custom_model.onnx";
    }
    if (!QFileInfo::exists(modelPath)) {
        modelPath = QCoreApplication::applicationDirPath() + "/../../OCR_Model/custom_model.onnx";
    }
    if (!QFileInfo::exists(modelPath)) {
        modelPath = QCoreApplication::applicationDirPath() + "/../Resources/OCR_Model/custom_model.onnx";
    }

    try {
        m_digitNet = cv::dnn::readNetFromONNX(modelPath.toLocal8Bit().constData());
        m_digitNetLoaded = !m_digitNet.empty();
        if (!m_digitNetLoaded) {
            m_digitNetError = "OpenCV DNN returned an empty network.";
            return;
        }

        bool useCuda = false;
        try {
            useCuda = cv::cuda::getCudaEnabledDeviceCount() > 0;
        } catch (const cv::Exception&) {
            useCuda = false;
        }

        if (useCuda) {
            m_digitNet.setPreferableBackend(cv::dnn::DNN_BACKEND_CUDA);
            m_digitNet.setPreferableTarget(cv::dnn::DNN_TARGET_CUDA);
        } else {
            m_digitNet.setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
            m_digitNet.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);
        }

    } catch (const cv::Exception& e) {
        m_digitNetLoaded = false;
        m_digitNetError = e.what();
    }
}

cv::Mat SudokuImageRecognizer::preprocessDigitForCustomModel(const cv::Mat& cell)
{
    cv::Mat bgr;
    if (cell.channels() == 3) {
        bgr = cell.clone();
    } else if (cell.channels() == 4) {
        cv::cvtColor(cell, bgr, cv::COLOR_BGRA2BGR);
    } else {
        cv::cvtColor(cell, bgr, cv::COLOR_GRAY2BGR);
    }

    cv::Mat resized;
    cv::resize(bgr, resized, cv::Size(224, 224), 0, 0, cv::INTER_AREA);
    return resized;
}

void SudokuImageRecognizer::splitIntoCells(const cv::Mat& warpedGray,
                                           std::array<cv::Mat, 81>& cells)
{
    int cellSize = warpedGray.rows / 9;
    int idx = 0;

    for (int r = 0; r < 9; ++r) {
        for (int c = 0; c < 9; ++c) {
            int x = c * cellSize;
            int y = r * cellSize;
            cv::Rect roi(x, y, cellSize, cellSize);
            cv::Mat cell = warpedGray(roi).clone();

            int margin = 4;
            cv::Rect innerRoi(margin, margin,
                              cell.cols - 2 * margin,
                              cell.rows - 2 * margin);

            cells[idx++] = cell(innerRoi).clone();
        }
    }
}

int SudokuImageRecognizer::recognizeDigit(const cv::Mat& cell)
{
    if (!m_digitNetLoaded) return 0;

    cv::Mat gray;
    if (cell.channels() == 3)
        cv::cvtColor(cell, gray, cv::COLOR_BGR2GRAY);
    else
        gray = cell.clone();

    cv::Mat binary;
    cv::adaptiveThreshold(gray, binary, 255,
        cv::ADAPTIVE_THRESH_GAUSSIAN_C, cv::THRESH_BINARY_INV, 11, 2);

    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, {2, 2});
    cv::morphologyEx(binary, binary, cv::MORPH_OPEN, kernel);

    if (cv::countNonZero(binary) < 15) return 0;

    cv::Rect bbox = cv::boundingRect(binary);
    if (bbox.width < 3 || bbox.height < 6) return 0;

    cv::Mat digit = preprocessDigitForCustomModel(cell);
    cv::Mat blob = cv::dnn::blobFromImage(digit, 1.0 / 255.0, cv::Size(224, 224),
                                          cv::Scalar(),
                                          true, false, CV_32F);

    try {
        m_digitNet.setInput(blob);
        cv::Mat output = m_digitNet.forward();
        auto [digitClass, confidence] = argmaxSoftmax(output);
        if (digitClass < 1 || digitClass > 9 || confidence < 0.60) {
            return 0;
        }
        return digitClass;
    } catch (const cv::Exception&) {
        return 0;
    }
}

bool SudokuImageRecognizer::processImage(const std::string& filePath,
                                         cv::Mat& warpedColor,
                                         int grid[9][9],
                                         std::string& diagMsg)
{
    diagMsg.clear();
    cv::Mat src = cv::imread(filePath);
    if (src.empty()) {
        diagMsg = "[Image Load] Cannot read image: " + filePath;
        return false;
    }

    if (!m_digitNetLoaded) {
        diagMsg = "[OCR Model] Failed to load OCR_Model/custom_model.onnx.\n" + m_digitNetError;
        return false;
    }

    warpedColor = m_detector.detect(src, diagMsg);
    if (warpedColor.empty())
        return false;

    std::array<cv::Mat, 81> cells;
    splitIntoCells(warpedColor, cells);

    for (int r = 0; r < 9; ++r) {
        for (int c = 0; c < 9; ++c) {
            grid[r][c] = recognizeDigit(cells[r * 9 + c]);
        }
    }

    return true;
}

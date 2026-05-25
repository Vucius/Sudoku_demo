#pragma once
#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>
#include <array>
#include "RobustSudokuDetector.h"

class SudokuImageRecognizer
{
public:
    SudokuImageRecognizer();

    bool processImage(const std::string& filePath,
                      cv::Mat& warpedColor,
                      int grid[9][9],
                      std::string& diagMsg);

private:
    RobustSudokuDetector m_detector;
    cv::dnn::Net m_digitNet;
    bool m_digitNetLoaded = false;
    std::string m_digitNetError;

    cv::Mat preprocessDigitForCustomModel(const cv::Mat& cell);

    void splitIntoCells(const cv::Mat& warpedGray,
                        std::array<cv::Mat, 81>& cells);

    int recognizeDigit(const cv::Mat& cell);
};

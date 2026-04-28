#pragma once
#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>
#include <array>
#include <vector>
#include "RobustSudokuDetector.h"

struct DigitTemplate {
    int digit;
    cv::Mat img;
};

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
    std::vector<DigitTemplate> m_templates;

    cv::Mat padAndResize(const cv::Mat& src);

    void splitIntoCells(const cv::Mat& warpedGray,
                        std::array<cv::Mat, 81>& cells);

    int recognizeDigit(const cv::Mat& cell);
};

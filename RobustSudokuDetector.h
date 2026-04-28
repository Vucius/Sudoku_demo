#pragma once
#include <opencv2/opencv.hpp>
#include <vector>
#include <array>
#include <cmath>
#include <algorithm>
#include <string>

class RobustSudokuDetector
{
public:
    // 主入口：返回透视矫正后的 450x450 彩色图，失败返回空 Mat
    // diagMsg 会记录具体在哪一步失败
    cv::Mat detect(const cv::Mat& src, std::string& diagMsg);

private:
    // 步骤1：预处理 - 增强浅蓝/低对比度线条
    cv::Mat preprocess(const cv::Mat& src);

    // 步骤2：霍夫线检测，分离水平/垂直线
    void detectLines(const cv::Mat& binary,
                     std::vector<float>& hLines,   // 水平线 y 坐标
                     std::vector<float>& vLines);  // 垂直线 x 坐标

    // 步骤3：聚类去重（合并距离很近的重复线）
    std::vector<float> clusterLines(const std::vector<float>& positions,
                                    float threshold = 15.0f);

    // 步骤4：验证并筛选出等间距的 10 条线（覆盖 9 格）
    bool pickEvenlySpaced(const std::vector<float>& lines,
                          std::vector<float>& out10,
                          int imageSize);

    // 步骤5：用四个角做透视矫正
    cv::Mat warpToGrid(const cv::Mat& src,
                       float x0, float x9,
                       float y0, float y9);
};

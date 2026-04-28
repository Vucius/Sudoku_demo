#include "SudokuImageRecognizer.h"
#include <QDir>
#include <QFileInfoList>
#include <QStringList>

SudokuImageRecognizer::SudokuImageRecognizer()
{
    // 加载无模型的字符模板
    for (int d = 1; d <= 9; ++d) {
        QDir dir(QString("Character_Sample/%1").arg(d));
        if (!dir.exists()) continue;
        
        QStringList filters;
        filters << "*.png" << "*.jpg" << "*.bmp";
        dir.setNameFilters(filters);
        
        QFileInfoList list = dir.entryInfoList(QDir::Files);
        for (const QFileInfo& fileInfo : list) {
            std::string path = fileInfo.absoluteFilePath().toLocal8Bit().constData();
            cv::Mat img = cv::imread(path, cv::IMREAD_UNCHANGED);
            if (img.empty()) continue;
            
            cv::Mat bin;
            if (img.channels() == 4) {
                // 提取 alpha 通道：文字区域是不透明的(255)
                std::vector<cv::Mat> channels;
                cv::split(img, channels);
                cv::threshold(channels[3], bin, 127, 255, cv::THRESH_BINARY);
            } else {
                cv::Mat gray;
                if (img.channels() == 3) cv::cvtColor(img, gray, cv::COLOR_BGR2GRAY);
                else gray = img.clone();
                
                // 判断背景色：大多数像素 > 128 说明是白底黑字，需要反转
                if (cv::countNonZero(gray > 128) > gray.total() / 2) {
                    cv::threshold(gray, bin, 0, 255, cv::THRESH_BINARY_INV | cv::THRESH_OTSU);
                } else {
                    cv::threshold(gray, bin, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);
                }
            }
            
            // 贴边裁剪
            cv::Rect bbox = cv::boundingRect(bin);
            if (bbox.width > 0 && bbox.height > 0) {
                cv::Mat processed = padAndResize(bin(bbox));
                m_templates.push_back({d, processed});
            }
        }
    }
}

// ── 等比例缩放并居中补齐到 24x24 ───────────────────────
cv::Mat SudokuImageRecognizer::padAndResize(const cv::Mat& src)
{
    double scale = 24.0 / std::max(src.cols, src.rows);
    cv::Mat resized;
    cv::resize(src, resized, cv::Size(), scale, scale, cv::INTER_AREA);
    
    cv::Mat canvas = cv::Mat::zeros(24, 24, CV_8UC1);
    int dx = (24 - resized.cols) / 2;
    int dy = (24 - resized.rows) / 2;
    
    // 边界保护
    if (dx < 0) dx = 0;
    if (dy < 0) dy = 0;
    if (dx + resized.cols > 24) dx = 24 - resized.cols;
    if (dy + resized.rows > 24) dy = 24 - resized.rows;
    
    resized.copyTo(canvas(cv::Rect(dx, dy, resized.cols, resized.rows)));
    
    // 再次二值化以防 resize 产生的边缘模糊
    cv::threshold(canvas, canvas, 127, 255, cv::THRESH_BINARY);
    return canvas;
}

// ── 切割 81 个单元格 ──────────────────────────────────
void SudokuImageRecognizer::splitIntoCells(const cv::Mat& warpedGray,
                                           std::array<cv::Mat, 81>& cells)
{
    int cellSize = warpedGray.rows / 9;
    int idx = 0;

    for (int r = 0; r < 9; ++r)
    {
        for (int c = 0; c < 9; ++c)
        {
            int x = c * cellSize;
            int y = r * cellSize;
            cv::Rect roi(x, y, cellSize, cellSize);

            cv::Mat cell = warpedGray(roi).clone();

            int margin = 4; // 调小一点，防止贴边的数字被切掉
            cv::Rect innerRoi(margin, margin,
                              cell.cols - 2 * margin,
                              cell.rows - 2 * margin);

            cells[idx++] = cell(innerRoi).clone();
        }
    }
}

// ── 检测单元格是否包含数字并识别（基于模板匹配） ────────────────
int SudokuImageRecognizer::recognizeDigit(const cv::Mat& cell)
{
    if (m_templates.empty()) return 0;

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
    if (bbox.width == 0 || bbox.height == 0) return 0;

    cv::Mat digit = padAndResize(binary(bbox));

    int bestDigit = 0;
    int minDiff = 24 * 24 + 1; // 最大可能的像素差异

    for (const auto& tmpl : m_templates) {
        cv::Mat diff;
        // 异或运算：只有不一样的像素会变成 255
        cv::bitwise_xor(digit, tmpl.img, diff);
        int score = cv::countNonZero(diff);

        if (score < minDiff) {
            minDiff = score;
            bestDigit = tmpl.digit;
        }
    }

    // 如果差异度大于一定阈值（比如超过总像素的三分之一），说明可能是噪点或乱码
    if (minDiff > (24 * 24) / 3) {
        return 0;
    }

    return bestDigit;
}

// ── 主入口 ─────────────────────────────────────────────
bool SudokuImageRecognizer::processImage(const std::string& filePath,
                                         cv::Mat& warpedColor,
                                         int grid[9][9],
                                         std::string& diagMsg)
{
    diagMsg.clear();
    cv::Mat src = cv::imread(filePath);
    if (src.empty())
    {
        diagMsg = "[Image Load] 无法读取图片，请检查路径或格式: " + filePath;
        return false;
    }

    // 检查 OCR 模型是否加载成功
    // 检查是否有加载到模板
    if (m_templates.empty())
    {
        diagMsg = "[OCR 模型替代] 未能找到任何字体模板。\n"
                  "请确保 Character_Sample 文件夹在程序同级目录下，且里面包含 1~9 的子文件夹和图片。";
        return false;
    }

    // 使用霍夫线检测 + 聚类的鲁棒检测器
    warpedColor = m_detector.detect(src, diagMsg);
    if (warpedColor.empty())
        return false; // diagMsg 已由 detector 赋值

    cv::Mat warpedGray;
    cv::cvtColor(warpedColor, warpedGray, cv::COLOR_BGR2GRAY);

    std::array<cv::Mat, 81> cells;
    splitIntoCells(warpedGray, cells);

    for (int r = 0; r < 9; ++r)
    {
        for (int c = 0; c < 9; ++c)
        {
            grid[r][c] = recognizeDigit(cells[r * 9 + c]);
        }
    }

    return true;
}

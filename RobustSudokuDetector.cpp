#include "RobustSudokuDetector.h"
#include <numeric>

// ── 步骤1：预处理 ──────────────────────────────────────
cv::Mat RobustSudokuDetector::preprocess(const cv::Mat& src)
{
    // 1a. 提取颜色掩码（针对浅蓝色线）
    cv::Mat hsv;
    cv::cvtColor(src, hsv, cv::COLOR_BGR2HSV);
    cv::Mat blueMask;
    // 浅蓝色范围：H=85-135, S=20-255, V=80-255（放宽范围兼容更多截图来源）
    cv::inRange(hsv,
                cv::Scalar(85, 20, 80),
                cv::Scalar(135, 255, 255),
                blueMask);

    // 1b. 灰度自适应阈值（兼容深色线）
    cv::Mat gray;
    cv::cvtColor(src, gray, cv::COLOR_BGR2GRAY);
    cv::GaussianBlur(gray, gray, {3, 3}, 0);
    cv::Mat adaptBin;
    cv::adaptiveThreshold(gray, adaptBin, 255,
                          cv::ADAPTIVE_THRESH_GAUSSIAN_C,
                          cv::THRESH_BINARY_INV, 11, 2);

    // 1c. 合并两种检测结果，并加入 Canny 边缘检测回退
    cv::Mat edges;
    cv::Canny(gray, edges, 50, 150);
    
    cv::Mat combined;
    cv::bitwise_or(blueMask, adaptBin, combined);
    cv::bitwise_or(combined, edges, combined);

    // 1d. 分方向膨胀，强化水平/垂直连续性
    cv::Mat kernelH = cv::getStructuringElement(cv::MORPH_RECT, {5, 1});
    cv::Mat kernelV = cv::getStructuringElement(cv::MORPH_RECT, {1, 5});
    cv::Mat dilH, dilV;
    cv::dilate(combined, dilH, kernelH);
    cv::dilate(combined, dilV, kernelV);
    cv::bitwise_or(dilH, dilV, combined);

    // [调试诊断] 将预处理后的二值图写入文件供排查（如果仍失败可以看这个图）
    cv::imwrite("debug_preprocess.png", combined);

    return combined;
}

// ── 步骤2：霍夫线检测 (改为概率霍夫，更好处理断裂/虚线) ──────────
void RobustSudokuDetector::detectLines(const cv::Mat& binary,
                                        std::vector<float>& hLines,
                                        std::vector<float>& vLines)
{
    std::vector<cv::Vec4i> lines;
    int minLen = std::min(binary.rows, binary.cols) * 15 / 100; // 最短线段
    int maxGap = 15; // 允许断裂的像素

    // 概率霍夫，门槛 20 票
    cv::HoughLinesP(binary, lines, 1, CV_PI / 180, 20, minLen, maxGap);

    for (auto& l : lines)
    {
        float x1 = l[0], y1 = l[1], x2 = l[2], y2 = l[3];
        float dx = x2 - x1;
        float dy = y2 - y1;
        float angle = std::fabs(std::atan2(dy, dx));

        // 水平线：角度接近 0 或 PI
        if (angle < CV_PI / 12 || angle > CV_PI * 11 / 12)
        {
            hLines.push_back((y1 + y2) / 2.0f);
        }
        // 垂直线：角度接近 PI/2
        else if (std::fabs(angle - CV_PI / 2) < CV_PI / 12)
        {
            vLines.push_back((x1 + x2) / 2.0f);
        }
    }
}

// ── 步骤3：聚类去重 (修复链式合并 bug) ──────────────────────
std::vector<float> RobustSudokuDetector::clusterLines(
    const std::vector<float>& positions, float threshold)
{
    if (positions.empty()) return {};

    std::vector<float> sorted = positions;
    std::sort(sorted.begin(), sorted.end());

    std::vector<float> clusters;
    float groupSum = sorted[0];
    int groupCount = 1;

    for (size_t i = 1; i < sorted.size(); ++i)
    {
        // 修复：必须与当前簇的中心比较，而不是与上一个元素比较，防止因线条密集被合并成一条
        float currentCenter = groupSum / groupCount;
        if (sorted[i] - currentCenter < threshold)
        {
            groupSum += sorted[i];
            ++groupCount;
        }
        else
        {
            clusters.push_back(groupSum / groupCount);
            groupSum  = sorted[i];
            groupCount = 1;
        }
    }
    clusters.push_back(groupSum / groupCount);
    return clusters;
}

// ── 步骤4：从聚类结果里找等间距的 10 条线 ─────────────
bool RobustSudokuDetector::pickEvenlySpaced(
    const std::vector<float>& lines,
    std::vector<float>& out10,
    int imageSize)
{
    // 关键修复：允许 >= 10 条，但也尝试从较少的线中推断
    // 如果聚类后线少于 10 条，说明部分线没被检测到，允许"补插"
    if (lines.size() < 3) return false;  // 至少要有 3 条才能估算间距（放宽）

    float bestScore = 1e9f;

    // 枚举起止线对（至少跨 3 条线）
    for (size_t i = 0; i < lines.size(); ++i)
    {
        for (size_t j = i + 2; j < lines.size(); ++j)
        {
            float start    = lines[i];
            float totalLen = lines[j] - lines[i];

            // 太小不是数独（至少占图像 20%，放宽以兼容小截图）
            if (totalLen < imageSize * 0.2f) continue;

            // 从 lines[i..j] 的实际间隔推断格子数（3~9）
            // 期望: totalLen ≈ N * step，N 必须能整除为 9
            // 这里直接假设总跨度对应 9 格，计算理想 step
            float step = totalLen / 9.0f;
            if (step < 5.0f) continue;  // 格子太小

            // 对每个期望位置找最近的实际线，允许找不到（用期望值补全）
            float totalError = 0.0f;
            std::vector<float> picked;
            int matched = 0;

            for (int k = 0; k <= 9; ++k)
            {
                float expected = start + step * k;
                float minDist  = step * 0.45f;  // 容差 45% 步长（放宽）
                float bestLine = -1.0f;

                for (auto& l : lines)
                {
                    float dist = std::fabs(l - expected);
                    if (dist < minDist) { minDist = dist; bestLine = l; }
                }

                if (bestLine >= 0.0f)
                {
                    totalError += minDist;
                    picked.push_back(bestLine);
                    ++matched;
                }
                else
                {
                    // 用期望值插补缺失线，误差计为半步
                    totalError += step * 0.25f;
                    picked.push_back(expected);
                }
            }

            // 至少要匹配到 6 条真实线（允许 4 条缺失，放宽）
            if (matched < 6) continue;

            if (totalError < bestScore)
            {
                bestScore = totalError;
                out10     = picked;
            }
        }
    }

    // 验证：平均误差 < 40% 步长（放宽，k=0..9 共 10 个点）
    if (out10.size() == 10)
    {
        float step = (out10[9] - out10[0]) / 9.0f;
        if (step > 0 && bestScore / 10.0f < step * 0.40f)
            return true;
    }
    return false;
}

// ── 步骤5：透视矫正 ────────────────────────────────────
cv::Mat RobustSudokuDetector::warpToGrid(const cv::Mat& src,
                                          float x0, float x9,
                                          float y0, float y9)
{
    std::vector<cv::Point2f> srcPts = {
        {x0, y0}, {x9, y0}, {x9, y9}, {x0, y9}
    };
    std::vector<cv::Point2f> dstPts = {
        {0, 0}, {449, 0}, {449, 449}, {0, 449}
    };
    cv::Mat M = cv::getPerspectiveTransform(srcPts, dstPts);
    cv::Mat warped;
    cv::warpPerspective(src, warped, M, {450, 450});
    return warped;
}

// ── 主入口 ────────────────────────────────────────────
cv::Mat RobustSudokuDetector::detect(const cv::Mat& src, std::string& diagMsg)
{
    diagMsg.clear();
    cv::Mat binary = preprocess(src);

    std::vector<float> rawH, rawV;
    detectLines(binary, rawH, rawV);

    // — 诊断： Hough 检测到的原始线数
    if (rawH.empty() && rawV.empty())
    {
        diagMsg = "[Step 2 detectLines] HoughLines 未检测到任何线条\n"
                  "可能原因：图片分辨率过小、线条颜色不在预处理范围内、或 minVotes 仍过高";
        return cv::Mat();
    }
    if (rawH.empty())
    {
        diagMsg = "[Step 2 detectLines] 未检测到水平线，砪直线数量 = " + std::to_string(rawV.size());
        return cv::Mat();
    }
    if (rawV.empty())
    {
        diagMsg = "[Step 2 detectLines] 未检测到垂直线，水平线数量 = " + std::to_string(rawH.size());
        return cv::Mat();
    }

    // — 聚类阈值：降到 8px，适配小分辨率截图（原 12px 会把相邻线误合并）
    std::vector<float> hClusters = clusterLines(rawH, 8.0f);
    std::vector<float> vClusters = clusterLines(rawV, 8.0f);

    // — 诊断： 聚类后线数
    if (hClusters.size() < 3)
    {
        diagMsg = "[Step 3 clusterLines] 水平线聚类后只有 " + std::to_string(hClusters.size())
                + " 条（需要 >= 3）\n"
                  "参考：rawH 原始线 " + std::to_string(rawH.size()) + " 条";
        return cv::Mat();
    }
    if (vClusters.size() < 3)
    {
        diagMsg = "[Step 3 clusterLines] 垂直线聚类后只有 " + std::to_string(vClusters.size())
                + " 条（需要 >= 3）\n"
                  "参考：rawV 原始线 " + std::to_string(rawV.size()) + " 条";
        return cv::Mat();
    }

    std::vector<float> h10, v10;
    bool hOk = pickEvenlySpaced(hClusters, h10, src.rows);
    bool vOk = pickEvenlySpaced(vClusters, v10, src.cols);

    if (!hOk && !vOk)
    {
        diagMsg = "[Step 4 pickEvenlySpaced] 水平线和垂直线均无法拥合成等间距的 10 条\n"
                  "水平线聚类: " + std::to_string(hClusters.size()) + " 条  "
                  "垂直线聚类: " + std::to_string(vClusters.size()) + " 条\n"
                  "可能题目：检测线条断裂、线间距不均匀、或网格占图比例 < 20%";
        return cv::Mat();
    }
    if (!hOk)
    {
        diagMsg = "[Step 4 pickEvenlySpaced] 水平线拥合失败\n"
                  "水平线聚类: " + std::to_string(hClusters.size()) + " 条\n"
                  "垂直线 OK（" + std::to_string(vClusters.size()) + " 条）";
        return cv::Mat();
    }
    if (!vOk)
    {
        diagMsg = "[Step 4 pickEvenlySpaced] 垂直线拥合失败\n"
                  "垂直线聚类: " + std::to_string(vClusters.size()) + " 条\n"
                  "水平线 OK（" + std::to_string(hClusters.size()) + " 条）";
        return cv::Mat();
    }

    diagMsg = "[OK] 检测成功";
    return warpToGrid(src, v10.front(), v10.back(),
                          h10.front(), h10.back());
}

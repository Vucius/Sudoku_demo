#pragma once

#include <QHeaderView>
#include <QHBoxLayout>
#include <QImage>
#include <QLabel>
#include <QPushButton>
#include <QColor>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QWidget>
#include <array>
#include <opencv2/opencv.hpp>
#include "SudokuGrid.h"
#include "SudokuImageRecognizer.h"
#include "SudokuSolver.h"

class Sudoku_demo : public QWidget
{
    Q_OBJECT

public:
    explicit Sudoku_demo(QWidget* parent = nullptr);
    ~Sudoku_demo();

private slots:
    void onLoadImage();
    void onRecognize();
    void onRetrain();
    void onSolve();

private:
    QLabel* m_originalImageLabel = nullptr;
    QTableWidget* m_leftGridTable = nullptr;
    SudokuGrid* m_rightGrid = nullptr;

    QPushButton* m_btnLoad = nullptr;
    QPushButton* m_btnRecognize = nullptr;
    QPushButton* m_btnRetrain = nullptr;
    QPushButton* m_btnSolve = nullptr;
    QLabel* m_noticeLabel = nullptr;
    QLabel* m_trainingStatusLabel = nullptr;

    QString m_currentImagePath;
    bool m_updatingGrid = false;
    std::array<std::array<int, 9>, 9> m_puzzleData{};
    std::array<std::array<bool, 9>, 9> m_correctedMask{};

    static QImage matToQImage(const cv::Mat& mat);

    QWidget* createPanel(const QString& title, QWidget* content, const QColor& titleColor);
    void configureRecognitionTable();
    void fillRecognitionTable(int grid[9][9]);
    void refreshButtonStates(bool hasImage, bool hasRecognition);
    bool readCurrentPuzzle(std::array<std::array<int, 9>, 9>& puzzle);
    void showOriginalImage(const QString& filePath);
};

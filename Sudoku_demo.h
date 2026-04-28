#pragma once

#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QImage>
#include <QTableWidget>
#include <QHeaderView>
#include <array>
#include <opencv2/opencv.hpp>
#include "SudokuGrid.h"
#include "SudokuSolver.h"
#include "SudokuImageRecognizer.h"

class Sudoku_demo : public QWidget
{
    Q_OBJECT

public:
    Sudoku_demo(QWidget *parent = nullptr);
    ~Sudoku_demo();

private slots:
    void onLoadImage();
    void onSolve();

private:
    // Left side: editable sudoku grid (to replace warped image)
    QTableWidget* m_leftGridTable;
    QPushButton* m_btnLoad;

    // Right side: solved puzzle grid
    SudokuGrid* m_rightGrid;
    QPushButton* m_btnSolve;

    // Current puzzle data (from image recognition)
    std::array<std::array<int, 9>, 9> m_puzzleData{};

    // Helper: convert cv::Mat to QImage
    static QImage matToQImage(const cv::Mat& mat);
};

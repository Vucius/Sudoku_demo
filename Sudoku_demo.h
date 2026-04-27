#pragma once

#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <array>
#include "SudokuGrid.h"
#include "SudokuSolver.h"

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
    // Left side: recognized puzzle
    SudokuGrid* m_leftGrid;
    QPushButton* m_btnLoad;

    // Right side: solved puzzle
    SudokuGrid* m_rightGrid;
    QPushButton* m_btnSolve;

    // Current puzzle data
    std::array<std::array<int, 9>, 9> m_puzzleData{};

    // Parse a sudoku image (stub: opens file dialog and fills demo data)
    bool loadSudokuFromImage(const QString& filePath);
};

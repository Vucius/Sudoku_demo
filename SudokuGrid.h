#pragma once

#include <QWidget>
#include <QPainter>
#include <QVector>
#include <array>

class SudokuGrid : public QWidget
{
    Q_OBJECT

public:
    explicit SudokuGrid(QWidget* parent = nullptr);
    ~SudokuGrid() override = default;

    // Set the entire grid (0 = empty)
    void setGrid(const std::array<std::array<int, 9>, 9>& grid);

    // Get current grid
    const std::array<std::array<int, 9>, 9>& getGrid() const { return m_grid; }

    // Mark which cells are "given" (for color differentiation)
    void setGivenMask(const std::array<std::array<bool, 9>, 9>& mask);

    // Clear the grid
    void clearGrid();

protected:
    void paintEvent(QPaintEvent* event) override;
    QSize sizeHint() const override { return QSize(360, 360); }
    QSize minimumSizeHint() const override { return QSize(180, 180); }

private:
    std::array<std::array<int, 9>, 9>  m_grid{};
    std::array<std::array<bool, 9>, 9> m_given{};
};

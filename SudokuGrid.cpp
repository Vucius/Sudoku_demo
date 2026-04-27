#include "SudokuGrid.h"
#include <QPainter>
#include <QPaintEvent>
#include <QFont>

SudokuGrid::SudokuGrid(QWidget* parent)
    : QWidget(parent)
{
    clearGrid();
    setMinimumSize(180, 180);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void SudokuGrid::setGrid(const std::array<std::array<int, 9>, 9>& grid)
{
    m_grid = grid;
    update();
}

void SudokuGrid::setGivenMask(const std::array<std::array<bool, 9>, 9>& mask)
{
    m_given = mask;
    update();
}

void SudokuGrid::clearGrid()
{
    for (auto& row : m_grid)
        row.fill(0);
    for (auto& row : m_given)
        row.fill(false);
    update();
}

void SudokuGrid::paintEvent(QPaintEvent* /*event*/)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Calculate the largest square that fits
    int side = qMin(width(), height());
    int offsetX = (width() - side) / 2;
    int offsetY = (height() - side) / 2;

    double cellSize = side / 9.0;

    // Background
    painter.fillRect(offsetX, offsetY, side, side, QColor(255, 255, 255));

    // Draw cell values
    QFont cellFont("Segoe UI", static_cast<int>(cellSize * 0.45), QFont::Bold);
    painter.setFont(cellFont);

    for (int r = 0; r < 9; ++r) {
        for (int c = 0; c < 9; ++c) {
            if (m_grid[r][c] != 0) {
                QRectF cellRect(offsetX + c * cellSize, offsetY + r * cellSize,
                                cellSize, cellSize);

                if (m_given[r][c]) {
                    // Given digits: dark blue
                    painter.setPen(QColor(25, 55, 109));
                } else {
                    // Solved digits: teal/green
                    painter.setPen(QColor(0, 150, 100));
                }

                painter.drawText(cellRect, Qt::AlignCenter,
                                 QString::number(m_grid[r][c]));
            }
        }
    }

    // Draw thin grid lines
    QPen thinPen(QColor(180, 190, 200), 1);
    painter.setPen(thinPen);
    for (int i = 0; i <= 9; ++i) {
        double x = offsetX + i * cellSize;
        double y = offsetY + i * cellSize;
        painter.drawLine(QPointF(x, offsetY), QPointF(x, offsetY + side));
        painter.drawLine(QPointF(offsetX, y), QPointF(offsetX + side, y));
    }

    // Draw thick box lines (3x3)
    QPen thickPen(QColor(25, 55, 109), 3);
    painter.setPen(thickPen);
    for (int i = 0; i <= 3; ++i) {
        double x = offsetX + i * 3 * cellSize;
        double y = offsetY + i * 3 * cellSize;
        painter.drawLine(QPointF(x, offsetY), QPointF(x, offsetY + side));
        painter.drawLine(QPointF(offsetX, y), QPointF(offsetX + side, y));
    }

    // Outer border
    QPen borderPen(QColor(25, 55, 109), 4);
    painter.setPen(borderPen);
    painter.drawRect(QRectF(offsetX, offsetY, side, side));
}

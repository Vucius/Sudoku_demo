#include "SudokuSolver.h"

bool SudokuSolver::solve(Grid& grid)
{
    int row, col;
    if (!findEmpty(grid, row, col))
        return true; // No empty cell => solved

    for (int num = 1; num <= 9; ++num) {
        if (isValid(grid, row, col, num)) {
            grid[row][col] = num;
            if (solve(grid))
                return true;
            grid[row][col] = 0; // backtrack
        }
    }
    return false;
}

bool SudokuSolver::findEmpty(const Grid& grid, int& row, int& col)
{
    for (row = 0; row < 9; ++row)
        for (col = 0; col < 9; ++col)
            if (grid[row][col] == 0)
                return true;
    return false;
}

bool SudokuSolver::isValid(const Grid& grid, int row, int col, int num)
{
    // Check row
    for (int c = 0; c < 9; ++c)
        if (grid[row][c] == num)
            return false;

    // Check column
    for (int r = 0; r < 9; ++r)
        if (grid[r][col] == num)
            return false;

    // Check 3x3 box
    int boxRow = (row / 3) * 3;
    int boxCol = (col / 3) * 3;
    for (int r = boxRow; r < boxRow + 3; ++r)
        for (int c = boxCol; c < boxCol + 3; ++c)
            if (grid[r][c] == num)
                return false;

    return true;
}

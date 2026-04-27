#pragma once

#include <array>

class SudokuSolver
{
public:
    using Grid = std::array<std::array<int, 9>, 9>;

    // Solve the grid in-place. Returns true if solved.
    static bool solve(Grid& grid);

private:
    static bool findEmpty(const Grid& grid, int& row, int& col);
    static bool isValid(const Grid& grid, int row, int col, int num);
};

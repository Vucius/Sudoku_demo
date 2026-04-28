#pragma once

#include <array>
#include <bitset>
#include <vector>
#include <utility>

// 候选数用 bitset<10>，bit[1..9] 对应数字 1..9
using Candidates = std::array<std::array<std::bitset<10>, 9>, 9>;

class SudokuSolver
{
public:
    using Grid = std::array<std::array<int, 9>, 9>;

    // Solve the grid in-place. Returns true if solved.
    static bool solve(Grid& grid);

private:
    // ── 候选数管理 ──
    static void initCandidates(int grid[9][9], Candidates& cands);
    static void propagate(int grid[9][9], Candidates& cands, int r, int c);

    // ── 解题技巧 ──
    static bool nakedSingle(int grid[9][9], Candidates& cands);
    static bool hiddenSingle(int grid[9][9], Candidates& cands);
    static bool nakedPair(int grid[9][9], Candidates& cands);
    static bool hiddenPair(int grid[9][9], Candidates& cands);

    // ── 辅助 ──
    static inline int boxOf(int r, int c) { return (r / 3) * 3 + (c / 3); }
};

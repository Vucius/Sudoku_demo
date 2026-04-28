#include "SudokuSolver.h"

namespace {
    bool isValidForDfs(int grid[9][9], int r, int c, int d) {
        // 检查行和列
        for (int i = 0; i < 9; ++i) {
            if (grid[r][i] == d) return false;
            if (grid[i][c] == d) return false;
        }
        // 检查 3x3 宫
        int br = (r / 3) * 3, bc = (c / 3) * 3;
        for (int dr = 0; dr < 3; ++dr) {
            for (int dc = 0; dc < 3; ++dc) {
                if (grid[br + dr][bc + dc] == d) return false;
            }
        }
        return true;
    }

    // DFS 回溯法（带最少候选数优化 MRV）
    bool dfsSolve(int grid[9][9]) {
        int minCands = 10;
        int minR = -1, minC = -1;
        
        for (int r = 0; r < 9; ++r) {
            for (int c = 0; c < 9; ++c) {
                if (grid[r][c] == 0) {
                    int count = 0;
                    for (int d = 1; d <= 9; ++d) {
                        if (isValidForDfs(grid, r, c, d)) count++;
                    }
                    if (count == 0) return false; // 出现死胡同
                    if (count < minCands) {
                        minCands = count;
                        minR = r;
                        minC = c;
                    }
                }
            }
        }
        
        // 没找到空格，说明解完了
        if (minR == -1) return true; 
        
        for (int d = 1; d <= 9; ++d) {
            if (isValidForDfs(grid, minR, minC, d)) {
                grid[minR][minC] = d;
                if (dfsSolve(grid)) return true;
                grid[minR][minC] = 0; // 回溯
            }
        }
        return false;
    }
}

// ── 公共入口：桥接 std::array 与 int[9][9] ──────────────
bool SudokuSolver::solve(Grid& grid)
{
    // 拷贝到 C 数组供内部使用
    int g[9][9];
    for (int r = 0; r < 9; ++r)
        for (int c = 0; c < 9; ++c)
            g[r][c] = grid[r][c];

    Candidates cands;
    initCandidates(g, cands);

    bool changed = true;
    while (changed)
    {
        changed  = nakedSingle (g, cands);
        changed |= hiddenSingle(g, cands);
        changed |= nakedPair   (g, cands);
        changed |= hiddenPair  (g, cands);
        // 还可以继续扩展：nakedTriple、boxLineReduction、xWing...
    }

    // 检查基础逻辑推导是否完全解开
    bool solved = true;
    for (int r = 0; r < 9; ++r) {
        for (int c = 0; c < 9; ++c) {
            if (g[r][c] == 0) {
                solved = false;
                break;
            }
        }
    }

    // 如果基础推导卡壳了（遇到骨灰级难度），则启动暴力回溯法作为终极兜底
    if (!solved) {
        if (!dfsSolve(g)) return false; // 回溯法也无解，说明题目本身有问题
    }

    // 拷贝回 std::array
    for (int r = 0; r < 9; ++r)
        for (int c = 0; c < 9; ++c)
            grid[r][c] = g[r][c];

    return true;
}

// ── 初始化所有空格的候选数 ──────────────────────────────
void SudokuSolver::initCandidates(int grid[9][9], Candidates& cands)
{
    for (int r = 0; r < 9; ++r)
        for (int c = 0; c < 9; ++c)
            cands[r][c].set();   // 全部置1，bit[0]不用

    for (int r = 0; r < 9; ++r)
    {
        for (int c = 0; c < 9; ++c)
        {
            if (grid[r][c] != 0)
            {
                int d = grid[r][c];
                // 已填数字，只保留自己的 bit
                cands[r][c].reset();
                cands[r][c].set(d);

                // 从同行/列/宫清除这个候选数
                for (int i = 0; i < 9; ++i)
                {
                    if (i != c) cands[r][i].reset(d);  // 同行
                    if (i != r) cands[i][c].reset(d);  // 同列
                }
                int br = (r / 3) * 3, bc = (c / 3) * 3;
                for (int dr = 0; dr < 3; ++dr)
                    for (int dc = 0; dc < 3; ++dc)
                    {
                        int nr = br + dr, nc = bc + dc;
                        if (nr != r || nc != c)
                            cands[nr][nc].reset(d);
                    }
            }
        }
    }
}

// ── 某格确定后，向外传播消除 ──────────────────────────
void SudokuSolver::propagate(int grid[9][9], Candidates& cands, int r, int c)
{
    int d = grid[r][c];
    cands[r][c].reset();
    cands[r][c].set(d);

    for (int i = 0; i < 9; ++i)
    {
        if (i != c) cands[r][i].reset(d);
        if (i != r) cands[i][c].reset(d);
    }
    int br = (r / 3) * 3, bc = (c / 3) * 3;
    for (int dr = 0; dr < 3; ++dr)
        for (int dc = 0; dc < 3; ++dc)
        {
            int nr = br + dr, nc = bc + dc;
            if (nr != r || nc != c)
                cands[nr][nc].reset(d);
        }
}

// ── 技巧1：Naked Single ────────────────────────────────
// 某空格候选数只剩1个，直接填入
bool SudokuSolver::nakedSingle(int grid[9][9], Candidates& cands)
{
    bool changed = false;
    for (int r = 0; r < 9; ++r)
    {
        for (int c = 0; c < 9; ++c)
        {
            if (grid[r][c] != 0) continue;
            if (cands[r][c].count() == 1)
            {
                for (int d = 1; d <= 9; ++d)
                    if (cands[r][c].test(d)) { grid[r][c] = d; break; }
                propagate(grid, cands, r, c);
                changed = true;
            }
        }
    }
    return changed;
}

// ── 技巧2：Hidden Single ──────────────────────────────
// 某数字在某行/列/宫只有一格能填
bool SudokuSolver::hiddenSingle(int grid[9][9], Candidates& cands)
{
    bool changed = false;

    // 扫描行
    for (int r = 0; r < 9; ++r)
    {
        for (int d = 1; d <= 9; ++d)
        {
            int count = 0, lastC = -1;
            for (int c = 0; c < 9; ++c)
                if (grid[r][c] == 0 && cands[r][c].test(d))
                    { ++count; lastC = c; }
            if (count == 1)
            {
                grid[r][lastC] = d;
                propagate(grid, cands, r, lastC);
                changed = true;
            }
        }
    }

    // 扫描列
    for (int c = 0; c < 9; ++c)
    {
        for (int d = 1; d <= 9; ++d)
        {
            int count = 0, lastR = -1;
            for (int r = 0; r < 9; ++r)
                if (grid[r][c] == 0 && cands[r][c].test(d))
                    { ++count; lastR = r; }
            if (count == 1)
            {
                grid[lastR][c] = d;
                propagate(grid, cands, lastR, c);
                changed = true;
            }
        }
    }

    // 扫描宫
    for (int box = 0; box < 9; ++box)
    {
        int br = (box / 3) * 3, bc = (box % 3) * 3;
        for (int d = 1; d <= 9; ++d)
        {
            int count = 0, lastR = -1, lastC = -1;
            for (int dr = 0; dr < 3; ++dr)
                for (int dc = 0; dc < 3; ++dc)
                {
                    int r = br + dr, c = bc + dc;
                    if (grid[r][c] == 0 && cands[r][c].test(d))
                        { ++count; lastR = r; lastC = c; }
                }
            if (count == 1)
            {
                grid[lastR][lastC] = d;
                propagate(grid, cands, lastR, lastC);
                changed = true;
            }
        }
    }

    return changed;
}

// ── 技巧3：Naked Pair ─────────────────────────────────
// 同行/列/宫中两格候选数完全相同且只有2个
// 则可从其他格清除这两个数字
bool SudokuSolver::nakedPair(int grid[9][9], Candidates& cands)
{
    bool changed = false;

    auto scanUnit = [&](std::vector<std::pair<int,int>>& cells) -> bool
    {
        bool localChanged = false;
        for (size_t i = 0; i < cells.size(); ++i)
        {
            auto [r1, c1] = cells[i];
            if (grid[r1][c1] != 0 || cands[r1][c1].count() != 2) continue;

            for (size_t j = i + 1; j < cells.size(); ++j)
            {
                auto [r2, c2] = cells[j];
                if (grid[r2][c2] != 0) continue;
                if (cands[r1][c1] != cands[r2][c2]) continue;

                // 找到一对 Naked Pair，从其他格清除这两个候选数
                for (auto& [r3, c3] : cells)
                {
                    if ((r3 == r1 && c3 == c1) || (r3 == r2 && c3 == c2)) continue;
                    if (grid[r3][c3] != 0) continue;
                    auto before = cands[r3][c3];
                    for (int d = 1; d <= 9; ++d)
                        if (cands[r1][c1].test(d)) cands[r3][c3].reset(d);
                    if (cands[r3][c3] != before) localChanged = true;
                }
            }
        }
        return localChanged;
    };

    // 行
    for (int r = 0; r < 9; ++r)
    {
        std::vector<std::pair<int,int>> cells;
        for (int c = 0; c < 9; ++c) cells.push_back({r, c});
        changed |= scanUnit(cells);
    }
    // 列
    for (int c = 0; c < 9; ++c)
    {
        std::vector<std::pair<int,int>> cells;
        for (int r = 0; r < 9; ++r) cells.push_back({r, c});
        changed |= scanUnit(cells);
    }
    // 宫
    for (int box = 0; box < 9; ++box)
    {
        int br = (box / 3) * 3, bc = (box % 3) * 3;
        std::vector<std::pair<int,int>> cells;
        for (int dr = 0; dr < 3; ++dr)
            for (int dc = 0; dc < 3; ++dc)
                cells.push_back({br + dr, bc + dc});
        changed |= scanUnit(cells);
    }

    return changed;
}

// ── 技巧4：Hidden Pair ──────────────────────────────
// 某宫/行/列中，两个数字只出现在同两个格里
// 则这两格中其他候选数可以全部清除
bool SudokuSolver::hiddenPair(int grid[9][9], Candidates& cands)
{
    bool changed = false;

    auto scanUnit = [&](std::vector<std::pair<int,int>>& cells) -> bool
    {
        bool localChanged = false;
        for (int d1 = 1; d1 <= 8; ++d1)
        {
            for (int d2 = d1 + 1; d2 <= 9; ++d2)
            {
                // 找 d1 和 d2 都只出现在哪些格
                std::vector<std::pair<int,int>> d1Cells, d2Cells;
                for (auto& [r, c] : cells)
                {
                    if (grid[r][c] != 0) continue;
                    if (cands[r][c].test(d1)) d1Cells.push_back({r, c});
                    if (cands[r][c].test(d2)) d2Cells.push_back({r, c});
                }
                // d1 和 d2 都只在同两格
                if (d1Cells.size() == 2 && d2Cells.size() == 2
                    && d1Cells == d2Cells)
                {
                    for (auto& [r, c] : d1Cells)
                    {
                        auto before = cands[r][c];
                        // 清除这两格里除 d1, d2 以外的所有候选数
                        for (int d = 1; d <= 9; ++d)
                            if (d != d1 && d != d2) cands[r][c].reset(d);
                        if (cands[r][c] != before) localChanged = true;
                    }
                }
            }
        }
        return localChanged;
    };

    for (int r = 0; r < 9; ++r)
    {
        std::vector<std::pair<int,int>> cells;
        for (int c = 0; c < 9; ++c) cells.push_back({r, c});
        changed |= scanUnit(cells);
    }
    for (int c = 0; c < 9; ++c)
    {
        std::vector<std::pair<int,int>> cells;
        for (int r = 0; r < 9; ++r) cells.push_back({r, c});
        changed |= scanUnit(cells);
    }
    for (int box = 0; box < 9; ++box)
    {
        int br = (box / 3) * 3, bc = (box % 3) * 3;
        std::vector<std::pair<int,int>> cells;
        for (int dr = 0; dr < 3; ++dr)
            for (int dc = 0; dc < 3; ++dc)
                cells.push_back({br + dr, bc + dc});
        changed |= scanUnit(cells);
    }

    return changed;
}

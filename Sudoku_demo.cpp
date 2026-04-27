#include "Sudoku_demo.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QFont>
#include <QFrame>
#include <QGroupBox>
#include <QSpacerItem>

Sudoku_demo::Sudoku_demo(QWidget *parent)
    : QWidget(parent)
{
    // ---- Window setup ----
    setWindowTitle(QStringLiteral("Sudoku Demo - 识别与求解"));
    resize(900, 520);
    setMinimumSize(700, 420);

    // ---- Main horizontal layout ----
    QHBoxLayout* mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(16);

    // ========== Left panel ==========
    QVBoxLayout* leftLayout = new QVBoxLayout();
    leftLayout->setSpacing(10);

    QLabel* leftTitle = new QLabel(QStringLiteral("识别结果"));
    QFont titleFont("Segoe UI", 13, QFont::Bold);
    leftTitle->setFont(titleFont);
    leftTitle->setAlignment(Qt::AlignCenter);
    leftTitle->setStyleSheet("color: #19376D; padding: 4px;");

    m_leftGrid = new SudokuGrid(this);

    m_btnLoad = new QPushButton(QStringLiteral("📂 加载图片识别数独"));
    m_btnLoad->setMinimumHeight(40);
    m_btnLoad->setCursor(Qt::PointingHandCursor);
    m_btnLoad->setStyleSheet(
        "QPushButton {"
        "  background-color: #19376D;"
        "  color: white;"
        "  border: none;"
        "  border-radius: 8px;"
        "  font-size: 14px;"
        "  font-weight: bold;"
        "  padding: 8px 16px;"
        "}"
        "QPushButton:hover {"
        "  background-color: #2B4F8C;"
        "}"
        "QPushButton:pressed {"
        "  background-color: #0F2340;"
        "}"
    );

    leftLayout->addWidget(leftTitle);
    leftLayout->addWidget(m_leftGrid, 1);
    leftLayout->addWidget(m_btnLoad);

    // ========== Vertical separator ==========
    QFrame* separator = new QFrame(this);
    separator->setFrameShape(QFrame::VLine);
    separator->setStyleSheet("color: #CDD5E0;");

    // ========== Right panel ==========
    QVBoxLayout* rightLayout = new QVBoxLayout();
    rightLayout->setSpacing(10);

    QLabel* rightTitle = new QLabel(QStringLiteral("求解结果"));
    rightTitle->setFont(titleFont);
    rightTitle->setAlignment(Qt::AlignCenter);
    rightTitle->setStyleSheet("color: #009664; padding: 4px;");

    m_rightGrid = new SudokuGrid(this);

    m_btnSolve = new QPushButton(QStringLiteral("🧩 求解数独"));
    m_btnSolve->setMinimumHeight(40);
    m_btnSolve->setCursor(Qt::PointingHandCursor);
    m_btnSolve->setEnabled(false); // disabled until puzzle is loaded
    m_btnSolve->setStyleSheet(
        "QPushButton {"
        "  background-color: #009664;"
        "  color: white;"
        "  border: none;"
        "  border-radius: 8px;"
        "  font-size: 14px;"
        "  font-weight: bold;"
        "  padding: 8px 16px;"
        "}"
        "QPushButton:hover {"
        "  background-color: #00B87A;"
        "}"
        "QPushButton:pressed {"
        "  background-color: #006644;"
        "}"
        "QPushButton:disabled {"
        "  background-color: #B0B8C4;"
        "}"
    );

    rightLayout->addWidget(rightTitle);
    rightLayout->addWidget(m_rightGrid, 1);
    rightLayout->addWidget(m_btnSolve);

    // ========== Assemble main layout ==========
    mainLayout->addLayout(leftLayout, 1);
    mainLayout->addWidget(separator);
    mainLayout->addLayout(rightLayout, 1);

    // ---- Connect signals ----
    connect(m_btnLoad, &QPushButton::clicked, this, &Sudoku_demo::onLoadImage);
    connect(m_btnSolve, &QPushButton::clicked, this, &Sudoku_demo::onSolve);

    // ---- Global stylesheet ----
    setStyleSheet(
        "QWidget { background-color: #F4F6FA; }"
    );
}

Sudoku_demo::~Sudoku_demo()
{
}

void Sudoku_demo::onLoadImage()
{
    QString filePath = QFileDialog::getOpenFileName(
        this,
        QStringLiteral("选择数独图片"),
        QString(),
        QStringLiteral("Images (*.png *.jpg *.jpeg *.bmp);;All Files (*)")
    );

    if (filePath.isEmpty())
        return;

    if (loadSudokuFromImage(filePath)) {
        // Display on left grid
        m_leftGrid->setGrid(m_puzzleData);

        // Mark non-zero cells as "given"
        std::array<std::array<bool, 9>, 9> mask{};
        for (int r = 0; r < 9; ++r)
            for (int c = 0; c < 9; ++c)
                mask[r][c] = (m_puzzleData[r][c] != 0);
        m_leftGrid->setGivenMask(mask);

        // Enable solve button
        m_btnSolve->setEnabled(true);

        // Clear the right grid
        m_rightGrid->clearGrid();
    }
}

void Sudoku_demo::onSolve()
{
    // Copy puzzle data and solve
    auto solution = m_puzzleData;

    if (SudokuSolver::solve(solution)) {
        // Build given mask from original puzzle
        std::array<std::array<bool, 9>, 9> mask{};
        for (int r = 0; r < 9; ++r)
            for (int c = 0; c < 9; ++c)
                mask[r][c] = (m_puzzleData[r][c] != 0);

        m_rightGrid->setGrid(solution);
        m_rightGrid->setGivenMask(mask);
    } else {
        QMessageBox::warning(this, QStringLiteral("求解失败"),
                             QStringLiteral("无法求解该数独，请检查输入是否正确。"));
    }
}

bool Sudoku_demo::loadSudokuFromImage(const QString& /*filePath*/)
{
    // ================================================================
    // TODO: 在此处接入 OCR / 图像识别逻辑
    //       当前使用演示数据代替真实图像识别
    // ================================================================

    // Demo puzzle (0 = empty cell)
    m_puzzleData = {{
        {{ 5, 3, 0,  0, 7, 0,  0, 0, 0 }},
        {{ 6, 0, 0,  1, 9, 5,  0, 0, 0 }},
        {{ 0, 9, 8,  0, 0, 0,  0, 6, 0 }},

        {{ 8, 0, 0,  0, 6, 0,  0, 0, 3 }},
        {{ 4, 0, 0,  8, 0, 3,  0, 0, 1 }},
        {{ 7, 0, 0,  0, 2, 0,  0, 0, 6 }},

        {{ 0, 6, 0,  0, 0, 0,  2, 8, 0 }},
        {{ 0, 0, 0,  4, 1, 9,  0, 0, 5 }},
        {{ 0, 0, 0,  0, 8, 0,  0, 7, 9 }},
    }};

    return true;
}

#include "Sudoku_demo.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QFont>
#include <QFrame>
#include <QPixmap>

// ---- cv::Mat -> QImage conversion ----
QImage Sudoku_demo::matToQImage(const cv::Mat& mat)
{
    if (mat.type() == CV_8UC3)
    {
        cv::Mat rgb;
        cv::cvtColor(mat, rgb, cv::COLOR_BGR2RGB);
        return QImage((const unsigned char*)rgb.data,
                      rgb.cols, rgb.rows, (int)rgb.step,
                      QImage::Format_RGB888).copy();
    }
    else if (mat.type() == CV_8UC1)
    {
        return QImage((const unsigned char*)mat.data,
                      mat.cols, mat.rows, (int)mat.step,
                      QImage::Format_Grayscale8).copy();
    }
    return QImage();
}

// ---- Constructor ----
Sudoku_demo::Sudoku_demo(QWidget *parent)
    : QWidget(parent)
{
    // ---- Window setup ----
    setWindowTitle(QStringLiteral("Sudoku Demo - \u8BC6\u522B\u4E0E\u6C42\u89E3"));
    resize(900, 520);
    setMinimumSize(700, 420);

    // ---- Main horizontal layout ----
    QHBoxLayout* mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(16);

    // ========== Left panel (warped image display) ==========
    QVBoxLayout* leftLayout = new QVBoxLayout();
    leftLayout->setSpacing(10);

    QLabel* leftTitle = new QLabel(QStringLiteral("\u8BC6\u522B\u7ED3\u679C"));
    QFont titleFont("Segoe UI", 13, QFont::Bold);
    leftTitle->setFont(titleFont);
    leftTitle->setAlignment(Qt::AlignCenter);
    leftTitle->setStyleSheet("color: #19376D; padding: 4px;");

    m_leftGridTable = new QTableWidget(9, 9, this);
    m_leftGridTable->horizontalHeader()->setVisible(false);
    m_leftGridTable->verticalHeader()->setVisible(false);
    m_leftGridTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_leftGridTable->hide(); // 初始状态下隐藏（即空白）
    
    for (int i = 0; i < 9; ++i) {
        m_leftGridTable->setRowHeight(i, 40);
        m_leftGridTable->setColumnWidth(i, 40);
    }
    m_leftGridTable->setFixedSize(40 * 9 + 4, 40 * 9 + 4);
    
    m_leftGridTable->setStyleSheet(
        "QTableWidget {"
        "  background-color: #FFFFFF;"
        "  border: 2px solid #19376D;"
        "  gridline-color: #CDD5E0;"
        "  font-size: 20px;"
        "  font-weight: bold;"
        "  color: #19376D;"
        "}"
    );

    // 增加 4 条粗线（2条水平，2条垂直），画出标准的 3x3 宫格样式
    for (int i = 1; i <= 2; ++i) {
        QFrame* vLine = new QFrame(m_leftGridTable->viewport());
        vLine->setStyleSheet("background-color: #19376D;");
        vLine->setGeometry(i * 3 * 40 - 1, 0, 2, 40 * 9);
        vLine->setAttribute(Qt::WA_TransparentForMouseEvents);

        QFrame* hLine = new QFrame(m_leftGridTable->viewport());
        hLine->setStyleSheet("background-color: #19376D;");
        hLine->setGeometry(0, i * 3 * 40 - 1, 40 * 9, 2);
        hLine->setAttribute(Qt::WA_TransparentForMouseEvents);
    }

    // 实时同步：当用户在左侧表格中手动修改数字时，立刻同步到底层的数独数组中
    connect(m_leftGridTable, &QTableWidget::cellChanged, this, [this](int row, int col) {
        QTableWidgetItem* item = m_leftGridTable->item(row, col);
        if (item && !item->text().isEmpty()) {
            m_puzzleData[row][col] = item->text().toInt();
        } else {
            m_puzzleData[row][col] = 0;
        }
    });

    m_btnLoad = new QPushButton(QStringLiteral("\U0001F4C2 \u52A0\u8F7D\u56FE\u7247\u8BC6\u522B\u6570\u72EC"));
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
    leftLayout->addWidget(m_leftGridTable, 0, Qt::AlignCenter);
    leftLayout->addWidget(m_btnLoad);

    // ========== Vertical separator ==========
    QFrame* separator = new QFrame(this);
    separator->setFrameShape(QFrame::VLine);
    separator->setStyleSheet("color: #CDD5E0;");

    // ========== Right panel (solved grid) ==========
    QVBoxLayout* rightLayout = new QVBoxLayout();
    rightLayout->setSpacing(10);

    QLabel* rightTitle = new QLabel(QStringLiteral("\u6C42\u89E3\u7ED3\u679C"));
    rightTitle->setFont(titleFont);
    rightTitle->setAlignment(Qt::AlignCenter);
    rightTitle->setStyleSheet("color: #009664; padding: 4px;");

    m_rightGrid = new SudokuGrid(this);

    m_btnSolve = new QPushButton(QStringLiteral("\U0001F9E9 \u6C42\u89E3\u6570\u72EC"));
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
    setStyleSheet("QWidget { background-color: #F4F6FA; }");
}

Sudoku_demo::~Sudoku_demo()
{
}

// ---- Load image & recognize ----
void Sudoku_demo::onLoadImage()
{
    QString filePath = QFileDialog::getOpenFileName(
        this,
        QStringLiteral("\u9009\u62E9\u6570\u72EC\u56FE\u7247"),
        QString(),
        QStringLiteral("Images (*.png *.jpg *.jpeg *.bmp);;All Files (*)")
    );

    if (filePath.isEmpty())
        return;

    SudokuImageRecognizer recognizer;
    cv::Mat warped;
    int grid[9][9] = { 0 };
    std::string diagMsg;

    // 修复：Windows 下 cv::imread 不支持 UTF-8 编码的中文路径，需要转换为 Local8Bit (例如 GBK)
    std::string localPath = filePath.toLocal8Bit().constData();
    bool ok = recognizer.processImage(localPath, warped, grid, diagMsg);

    if (!ok)
    {
        QString msg = QString::fromStdString(diagMsg);
        QMessageBox::warning(this,
                             QStringLiteral("识别失败"),
                             QStringLiteral("没有识别到数独盘面。\n\n诊断信息：\n") + msg);
        return;
    }

    // 将识别到的网格填充到左侧表格中（等待用户校对和补充）
    for (int r = 0; r < 9; ++r) {
        for (int c = 0; c < 9; ++c) {
            if (grid[r][c] != 0) {
                // 已接入 MNIST 模型，现在能直接填写真实的识别数字
                QTableWidgetItem* item = new QTableWidgetItem(QString::number(grid[r][c]));
                item->setTextAlignment(Qt::AlignCenter);
                item->setForeground(QBrush(QColor(25, 55, 109))); // 深蓝色表示识别出的原生数字
                m_leftGridTable->setItem(r, c, item);
            } else {
                QTableWidgetItem* item = new QTableWidgetItem("");
                item->setTextAlignment(Qt::AlignCenter);
                m_leftGridTable->setItem(r, c, item);
            }
        }
    }
    
    // 显示左侧识别后的网格
    m_leftGridTable->show();

    // Enable solve button
    m_btnSolve->setEnabled(true);

    // Clear the right grid
    m_rightGrid->clearGrid();
}

void Sudoku_demo::onSolve()
{
    // 从左侧表格中读取用户校对后的数字
    for (int r = 0; r < 9; ++r) {
        for (int c = 0; c < 9; ++c) {
            QTableWidgetItem* item = m_leftGridTable->item(r, c);
            if (item && !item->text().isEmpty()) {
                m_puzzleData[r][c] = item->text().toInt();
            } else {
                m_puzzleData[r][c] = 0;
            }
        }
    }

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
        QMessageBox::warning(this,
                             QStringLiteral("\u6C42\u89E3\u5931\u8D25"),
                             QStringLiteral("\u65E0\u6CD5\u6C42\u89E3\u8BE5\u6570\u72EC\uFF0C\u8BF7\u68C0\u67E5\u8F93\u5165\u662F\u5426\u6B63\u786E\u3002"));
    }
}

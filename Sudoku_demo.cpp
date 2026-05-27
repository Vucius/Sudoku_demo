#include "Sudoku_demo.h"

#include <QBrush>
#include <QColor>
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFont>
#include <QFrame>
#include <QList>
#include <QMessageBox>
#include <QPixmap>
#include <QCloseEvent>
#include <QRegularExpression>
#include <QStandardPaths>

namespace {
QString resolveProjectDir()
{
    QString current = QDir::currentPath();
    if (QFileInfo::exists(current + "/OCR_Model/retrain_custom_model.py")) {
        return current;
    }

    QDir appDir(QCoreApplication::applicationDirPath());
    if (QFileInfo::exists(appDir.filePath("../../OCR_Model/retrain_custom_model.py"))) {
        appDir.cd("../..");
        return appDir.absolutePath();
    }

    QDir macResources(QCoreApplication::applicationDirPath());
    if (QFileInfo::exists(macResources.filePath("../Resources/OCR_Model/retrain_custom_model.py"))) {
        macResources.cd("../Resources");
        return macResources.absolutePath();
    }

    return current;
}

QString resolveWritableDataDir()
{
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (dataDir.isEmpty()) {
        dataDir = QDir::homePath() + "/.sudoku_demo";
    }
    QDir().mkpath(dataDir);
    return dataDir;
}
}

QImage Sudoku_demo::matToQImage(const cv::Mat& mat)
{
    if (mat.type() == CV_8UC3) {
        cv::Mat rgb;
        cv::cvtColor(mat, rgb, cv::COLOR_BGR2RGB);
        return QImage(rgb.data, rgb.cols, rgb.rows, static_cast<int>(rgb.step),
                      QImage::Format_RGB888).copy();
    }

    if (mat.type() == CV_8UC1) {
        return QImage(mat.data, mat.cols, mat.rows, static_cast<int>(mat.step),
                      QImage::Format_Grayscale8).copy();
    }

    return QImage();
}

Sudoku_demo::Sudoku_demo(QWidget* parent)
    : QWidget(parent)
{
    setWindowTitle(QStringLiteral("Sudoku Demo - OCR and Solver"));
    resize(1180, 560);
    setMinimumSize(980, 480);

    m_originalImageLabel = new QLabel(this);
    m_originalImageLabel->setAlignment(Qt::AlignCenter);
    m_originalImageLabel->setMinimumSize(300, 360);
    m_originalImageLabel->setText(QStringLiteral("No image loaded"));
    m_originalImageLabel->setStyleSheet(
        "QLabel { background: #FFFFFF; border: 2px solid #CDD5E0; color: #6B7280; }"
    );

    m_leftGridTable = new QTableWidget(9, 9, this);
    configureRecognitionTable();
    m_leftGridTable->hide();

    m_rightGrid = new SudokuGrid(this);

    QWidget* originalPanel = createPanel(QStringLiteral("Original Image"), m_originalImageLabel, QColor(70, 70, 70));
    QWidget* recognitionPanel = createPanel(QStringLiteral("OCR Result"), m_leftGridTable, QColor(25, 55, 109));
    QWidget* solutionPanel = createPanel(QStringLiteral("Solution"), m_rightGrid, QColor(0, 150, 100));

    QHBoxLayout* contentLayout = new QHBoxLayout();
    contentLayout->setSpacing(14);
    contentLayout->addWidget(originalPanel, 1);
    contentLayout->addWidget(recognitionPanel, 1);
    contentLayout->addWidget(solutionPanel, 1);

    m_btnLoad = new QPushButton(QStringLiteral("Load Image"), this);
    m_btnRecognize = new QPushButton(QStringLiteral("Recognize"), this);
    m_btnRetrain = new QPushButton(QStringLiteral("Retrain"), this);
    m_btnSolve = new QPushButton(QStringLiteral("Solve"), this);

    const QList<QPushButton*> buttons = {m_btnLoad, m_btnRecognize, m_btnRetrain, m_btnSolve};
    for (QPushButton* button : buttons) {
        button->setMinimumHeight(38);
        button->setCursor(Qt::PointingHandCursor);
        button->setStyleSheet(
            "QPushButton {"
            "  background-color: #19376D;"
            "  color: white;"
            "  border: none;"
            "  border-radius: 6px;"
            "  font-size: 14px;"
            "  font-weight: 600;"
            "  padding: 8px 18px;"
            "}"
            "QPushButton:hover { background-color: #2B4F8C; }"
            "QPushButton:pressed { background-color: #0F2340; }"
            "QPushButton:disabled { background-color: #B0B8C4; }"
        );
    }

    QHBoxLayout* actionLayout = new QHBoxLayout();
    actionLayout->setSpacing(10);
    actionLayout->addStretch();
    actionLayout->addWidget(m_btnLoad);
    actionLayout->addWidget(m_btnRecognize);
    actionLayout->addWidget(m_btnRetrain);
    actionLayout->addWidget(m_btnSolve);
    actionLayout->addStretch();

    m_noticeLabel = new QLabel(QStringLiteral("Ready"), this);
    m_trainingStatusLabel = new QLabel(QStringLiteral("Retrain: idle"), this);
    m_noticeLabel->setStyleSheet("color: #4B5563; font-size: 12px;");
    m_trainingStatusLabel->setStyleSheet("color: #4B5563; font-size: 12px;");

    QHBoxLayout* statusLayout = new QHBoxLayout();
    statusLayout->setContentsMargins(2, 0, 2, 0);
    statusLayout->addWidget(m_noticeLabel, 1, Qt::AlignLeft);
    statusLayout->addWidget(m_trainingStatusLabel, 1, Qt::AlignRight);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(14);
    mainLayout->addLayout(contentLayout, 1);
    mainLayout->addLayout(actionLayout);
    mainLayout->addLayout(statusLayout);

    connect(m_btnLoad, &QPushButton::clicked, this, &Sudoku_demo::onLoadImage);
    connect(m_btnRecognize, &QPushButton::clicked, this, &Sudoku_demo::onRecognize);
    connect(m_btnRetrain, &QPushButton::clicked, this, &Sudoku_demo::onRetrain);
    connect(m_btnSolve, &QPushButton::clicked, this, &Sudoku_demo::onSolve);

    refreshButtonStates(false, false);
    setStyleSheet("QWidget { background-color: #F4F6FA; }");
}

Sudoku_demo::~Sudoku_demo() = default;

QWidget* Sudoku_demo::createPanel(const QString& title, QWidget* content, const QColor& titleColor)
{
    QWidget* panel = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(panel);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(10);

    QLabel* titleLabel = new QLabel(title, panel);
    QFont titleFont("Segoe UI", 13, QFont::Bold);
    titleLabel->setFont(titleFont);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet(QString("color: %1; padding: 4px;").arg(titleColor.name()));

    layout->addWidget(titleLabel);
    layout->addWidget(content, 1, Qt::AlignCenter);
    return panel;
}

void Sudoku_demo::configureRecognitionTable()
{
    m_leftGridTable->horizontalHeader()->setVisible(false);
    m_leftGridTable->verticalHeader()->setVisible(false);
    m_leftGridTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_leftGridTable->setSelectionBehavior(QAbstractItemView::SelectItems);
    m_leftGridTable->setEditTriggers(QAbstractItemView::AnyKeyPressed |
                                     QAbstractItemView::SelectedClicked |
                                     QAbstractItemView::EditKeyPressed);

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
        "QTableWidget::item:selected {"
        "  background-color: #FFF176;"
        "  color: #19376D;"
        "  border: 2px solid #2563EB;"
        "}"
    );

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

    connect(m_leftGridTable, &QTableWidget::cellChanged, this, [this](int row, int col) {
        if (m_updatingGrid) {
            return;
        }

        QTableWidgetItem* item = m_leftGridTable->item(row, col);
        int value = 0;
        if (item && !item->text().trimmed().isEmpty()) {
            value = item->text().toInt();
            if (value < 0 || value > 9) {
                value = 0;
            }
        }

        m_puzzleData[row][col] = value;
        m_correctedMask[row][col] = true;

        if (!item) {
            item = new QTableWidgetItem();
            m_leftGridTable->setItem(row, col, item);
        }

        item->setText(value > 0 ? QString::number(value) : QString());
        item->setTextAlignment(Qt::AlignCenter);
        item->setBackground(QBrush(QColor("#FFF176")));
        item->setForeground(QBrush(QColor("#19376D")));
    });

    connect(m_leftGridTable, &QTableWidget::cellClicked, this, [this](int row, int col) {
        QTableWidgetItem* item = m_leftGridTable->item(row, col);
        if (!item) {
            item = new QTableWidgetItem();
            item->setTextAlignment(Qt::AlignCenter);
            m_leftGridTable->setItem(row, col, item);
        }

        m_leftGridTable->setCurrentCell(row, col);
        m_leftGridTable->editItem(item);
    });
}

void Sudoku_demo::refreshButtonStates(bool hasImage, bool hasRecognition)
{
    m_btnRecognize->setEnabled(hasImage);
    m_btnRetrain->setEnabled(hasRecognition);
    m_btnSolve->setEnabled(hasRecognition);
}

void Sudoku_demo::showOriginalImage(const QString& filePath)
{
    QPixmap pix(filePath);
    if (pix.isNull()) {
        m_originalImageLabel->setText(QStringLiteral("Preview failed"));
        return;
    }

    m_originalImageLabel->setPixmap(
        pix.scaled(m_originalImageLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation)
    );
}

void Sudoku_demo::onLoadImage()
{
    QString filePath = QFileDialog::getOpenFileName(
        this,
        QStringLiteral("Select sudoku image"),
        QString(),
        QStringLiteral("Images (*.png *.jpg *.jpeg *.bmp);;All Files (*)")
    );

    if (filePath.isEmpty()) {
        return;
    }

    m_currentImagePath = filePath;
    showOriginalImage(filePath);
    m_leftGridTable->hide();
    m_rightGrid->clearGrid();
    m_puzzleData = {};
    m_correctedMask = {};
    m_noticeLabel->setText(QStringLiteral("Image loaded. Click Recognize to run OCR."));
    m_trainingStatusLabel->setText(QStringLiteral("Retrain: idle"));
    refreshButtonStates(true, false);
}

void Sudoku_demo::onRecognize()
{
    if (m_currentImagePath.isEmpty()) {
        return;
    }

    SudokuImageRecognizer recognizer;
    cv::Mat warped;
    int grid[9][9] = {};
    std::string diagMsg;

    std::string localPath = m_currentImagePath.toLocal8Bit().constData();
    bool ok = recognizer.processImage(localPath, warped, grid, diagMsg);
    if (!ok) {
        m_noticeLabel->setText(QStringLiteral("Recognition failed."));
        QMessageBox::warning(this,
                             QStringLiteral("Recognition failed"),
                             QStringLiteral("No sudoku board was recognized.\n\nDiagnostics:\n") +
                                 QString::fromStdString(diagMsg));
        return;
    }

    fillRecognitionTable(grid);
    m_leftGridTable->show();
    m_rightGrid->clearGrid();
    m_noticeLabel->setText(QStringLiteral("Recognition complete. Click a cell and type 0-9 to correct it."));
    m_trainingStatusLabel->setText(QStringLiteral("Retrain: idle"));
    refreshButtonStates(true, true);
}

void Sudoku_demo::fillRecognitionTable(int grid[9][9])
{
    m_updatingGrid = true;
    m_leftGridTable->clearContents();
    m_puzzleData = {};
    m_correctedMask = {};

    for (int r = 0; r < 9; ++r) {
        for (int c = 0; c < 9; ++c) {
            int value = grid[r][c];
            m_puzzleData[r][c] = value;

            QTableWidgetItem* item = new QTableWidgetItem(value > 0 ? QString::number(value) : QString());
            item->setTextAlignment(Qt::AlignCenter);
            item->setForeground(QBrush(QColor("#19376D")));
            item->setBackground(QBrush(Qt::white));
            m_leftGridTable->setItem(r, c, item);
        }
    }

    m_updatingGrid = false;
}

bool Sudoku_demo::readCurrentPuzzle(std::array<std::array<int, 9>, 9>& puzzle)
{
    puzzle = {};
    for (int r = 0; r < 9; ++r) {
        for (int c = 0; c < 9; ++c) {
            QTableWidgetItem* item = m_leftGridTable->item(r, c);
            int value = 0;
            if (item && !item->text().trimmed().isEmpty()) {
                bool ok = false;
                value = item->text().toInt(&ok);
                if (!ok || value < 0 || value > 9) {
                    QMessageBox::warning(this,
                                         QStringLiteral("Invalid input"),
                                         QStringLiteral("OCR result cells only accept values from 0 to 9."));
                    return false;
                }
            }
            puzzle[r][c] = value;
        }
    }
    return true;
}

void Sudoku_demo::onSolve()
{
    std::array<std::array<int, 9>, 9> puzzle{};
    if (!readCurrentPuzzle(puzzle)) {
        return;
    }

    m_puzzleData = puzzle;
    auto solution = puzzle;
    if (!SudokuSolver::solve(solution)) {
        m_noticeLabel->setText(QStringLiteral("Solve failed. Continue correcting OCR cells."));
        QMessageBox::warning(this,
                             QStringLiteral("Solve failed"),
                             QStringLiteral("Unable to solve this sudoku. Please continue correcting the OCR result."));
        return;
    }

    std::array<std::array<bool, 9>, 9> mask{};
    for (int r = 0; r < 9; ++r) {
        for (int c = 0; c < 9; ++c) {
            mask[r][c] = (puzzle[r][c] != 0);
        }
    }

    m_rightGrid->setGrid(solution);
    m_rightGrid->setGivenMask(mask);
    m_noticeLabel->setText(QStringLiteral("Solved."));
}

void Sudoku_demo::onRetrain()
{
    std::array<std::array<int, 9>, 9> puzzle{};
    if (!readCurrentPuzzle(puzzle)) {
        m_trainingStatusLabel->setText(QStringLiteral("Retrain: invalid input"));
        return;
    }

    m_trainingStatusLabel->setText(QStringLiteral("Retrain: validating board..."));

    auto solution = puzzle;
    if (!SudokuSolver::solve(solution)) {
        m_noticeLabel->setText(QStringLiteral("Current board is unsolvable. Continue fixing OCR errors."));
        m_trainingStatusLabel->setText(QStringLiteral("Retrain: blocked"));
        return;
    }

    std::array<std::array<bool, 9>, 9> mask{};
    for (int r = 0; r < 9; ++r) {
        for (int c = 0; c < 9; ++c) {
            mask[r][c] = (puzzle[r][c] != 0);
        }
    }
    m_rightGrid->setGrid(solution);
    m_rightGrid->setGivenMask(mask);

    SudokuImageRecognizer recognizer;
    cv::Mat warped;
    int ignoredGrid[9][9] = {};
    std::string diagMsg;
    std::string localPath = m_currentImagePath.toLocal8Bit().constData();
    if (!recognizer.processImage(localPath, warped, ignoredGrid, diagMsg)) {
        m_noticeLabel->setText(QStringLiteral("Could not prepare training samples."));
        m_trainingStatusLabel->setText(QStringLiteral("Retrain: failed"));
        return;
    }

    QString retrainDataDir;
    QString errorMessage;
    if (!saveTrainingSamples(warped, puzzle, solution, retrainDataDir, errorMessage)) {
        m_noticeLabel->setText(errorMessage);
        m_trainingStatusLabel->setText(QStringLiteral("Retrain: failed"));
        return;
    }

    m_noticeLabel->setText(QStringLiteral("Board validated. Training samples saved."));
    startTrainingProcess(retrainDataDir);
}

bool Sudoku_demo::saveTrainingSamples(const cv::Mat& warpedColor,
                                      const std::array<std::array<int, 9>, 9>& puzzle,
                                      const std::array<std::array<int, 9>, 9>& solvedBoard,
                                      QString& retrainDataDir,
                                      QString& errorMessage)
{
    Q_UNUSED(solvedBoard);

    if (warpedColor.empty()) {
        errorMessage = QStringLiteral("No warped board image available.");
        return false;
    }

    const QString root = resolveWritableDataDir() + "/OCR_Model/retrain_data/train";
    QDir rootDir(root);
    if (!rootDir.exists() && !rootDir.mkpath(".")) {
        errorMessage = QStringLiteral("Failed to create retrain data directory.");
        return false;
    }

    const QString stamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss_zzz");
    const int cellSize = warpedColor.rows / 9;

    for (int label = 0; label <= 9; ++label) {
        rootDir.mkpath(QString::number(label));
    }

    for (int r = 0; r < 9; ++r) {
        for (int c = 0; c < 9; ++c) {
            int label = puzzle[r][c];
            if (label < 1 || label > 9) {
                continue;
            }

            int margin = 4;
            cv::Rect roi(c * cellSize + margin,
                         r * cellSize + margin,
                         cellSize - 2 * margin,
                         cellSize - 2 * margin);
            cv::Mat cell = warpedColor(roi).clone();

            QString filePath = QString("%1/%2/%3_r%4_c%5.png")
                .arg(root)
                .arg(label)
                .arg(stamp)
                .arg(r)
                .arg(c);
            if (!cv::imwrite(filePath.toLocal8Bit().constData(), cell)) {
                errorMessage = QStringLiteral("Failed to write training sample.");
                return false;
            }
        }
    }

    retrainDataDir = root;
    return true;
}

void Sudoku_demo::startTrainingProcess(const QString& retrainDataDir)
{
    if (m_trainingProcess && m_trainingProcess->state() != QProcess::NotRunning) {
        m_trainingStatusLabel->setText(QStringLiteral("Retrain: already running"));
        return;
    }

    QString projectDir = resolveProjectDir();
    QString writableDir = resolveWritableDataDir();

    QString pythonExe = projectDir + "/.venv/Scripts/python.exe";
    if (!QFileInfo::exists(pythonExe)) {
        pythonExe = QStringLiteral("python");
    }

    QString basePth = writableDir + "/OCR_Model/custom_model.pth";
    if (!QFileInfo::exists(basePth)) {
        basePth = projectDir + "/OCR_Model/custom_model.pth";
    }

    QString scriptPath = projectDir + "/OCR_Model/retrain_custom_model.py";
    QStringList args;
    args << scriptPath
         << "--base-data" << (projectDir + "/Character_Sample")
         << "--retrain-data" << retrainDataDir
         << "--base-pth" << basePth
         << "--output-pth" << (writableDir + "/OCR_Model/custom_model.pth")
         << "--output-onnx" << (writableDir + "/OCR_Model/custom_model.onnx")
         << "--epochs" << "12";

    m_trainingProcess = new QProcess(this);
    m_trainingProcess->setWorkingDirectory(projectDir);
    m_trainingInterrupted = false;
    connect(m_trainingProcess, &QProcess::readyReadStandardOutput,
            this, &Sudoku_demo::onTrainingOutput);
    connect(m_trainingProcess, &QProcess::readyReadStandardError,
            this, &Sudoku_demo::onTrainingError);
    connect(m_trainingProcess, &QProcess::finished,
            this, &Sudoku_demo::onTrainingFinished);

    m_btnRetrain->setEnabled(false);
    m_btnRecognize->setEnabled(false);
    m_trainingStatusLabel->setText(QStringLiteral("Retrain: running..."));
    m_trainingProcess->start(pythonExe, args);
}

void Sudoku_demo::onTrainingOutput()
{
    if (!m_trainingProcess) {
        return;
    }

    QString text = QString::fromLocal8Bit(m_trainingProcess->readAllStandardOutput());
    static const QRegularExpression epochPattern("epoch=(\\d+)/(\\d+)\\s+loss=([0-9.]+)\\s+acc=([0-9.]+)%");
    QRegularExpressionMatchIterator it = epochPattern.globalMatch(text);
    QRegularExpressionMatch lastMatch;
    while (it.hasNext()) {
        lastMatch = it.next();
    }

    if (lastMatch.hasMatch()) {
        m_trainingStatusLabel->setText(
            QStringLiteral("Retrain: epoch %1/%2, loss %3, acc %4%")
                .arg(lastMatch.captured(1))
                .arg(lastMatch.captured(2))
                .arg(lastMatch.captured(3))
                .arg(lastMatch.captured(4))
        );
    } else if (text.contains("saved_onnx=")) {
        m_trainingStatusLabel->setText(QStringLiteral("Retrain: exporting complete"));
    }
}

void Sudoku_demo::onTrainingError()
{
    if (!m_trainingProcess) {
        return;
    }

    QString text = QString::fromLocal8Bit(m_trainingProcess->readAllStandardError()).trimmed();
    if (!text.isEmpty()) {
        m_noticeLabel->setText(QStringLiteral("Retrain log: ") + text.left(180));
    }
}

void Sudoku_demo::onTrainingFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    QString stdoutText;
    QString stderrText;
    if (m_trainingProcess) {
        stdoutText = QString::fromLocal8Bit(m_trainingProcess->readAllStandardOutput());
        stderrText = QString::fromLocal8Bit(m_trainingProcess->readAllStandardError());
        m_trainingProcess->deleteLater();
        m_trainingProcess = nullptr;
    }

    if (m_trainingInterrupted) {
        m_noticeLabel->setText(QStringLiteral("Retrain interrupted. Previous model was kept."));
        m_trainingStatusLabel->setText(QStringLiteral("Retrain: interrupted"));
        m_btnRetrain->setEnabled(true);
        m_btnRecognize->setEnabled(!m_currentImagePath.isEmpty());
        return;
    }

    bool ok = (exitStatus == QProcess::NormalExit && exitCode == 0);
    if (ok) {
        m_noticeLabel->setText(QStringLiteral("Retrain complete. Next recognition will use the updated model."));
        m_trainingStatusLabel->setText(QStringLiteral("Retrain: complete"));
    } else {
        QString message = stderrText.isEmpty() ? stdoutText : stderrText;
        m_noticeLabel->setText(QStringLiteral("Retrain failed: ") + message.left(180));
        m_trainingStatusLabel->setText(QStringLiteral("Retrain: failed"));
    }

    m_btnRetrain->setEnabled(true);
    m_btnRecognize->setEnabled(!m_currentImagePath.isEmpty());
}

void Sudoku_demo::closeEvent(QCloseEvent* event)
{
    if (m_trainingProcess && m_trainingProcess->state() != QProcess::NotRunning) {
        m_trainingInterrupted = true;
        m_trainingStatusLabel->setText(QStringLiteral("Retrain: stopping..."));
        m_trainingProcess->terminate();
        if (!m_trainingProcess->waitForFinished(3000)) {
            m_trainingProcess->kill();
            m_trainingProcess->waitForFinished(3000);
        }
    }

    QWidget::closeEvent(event);
}

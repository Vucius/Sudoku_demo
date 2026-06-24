# AGENTS.md — Sudoku Demo

This file provides context for AI coding assistants working on the Sudoku Demo project.

## Project Overview

Sudoku Demo is a Windows desktop application that:
1. Loads a Sudoku puzzle image
2. Detects the grid and recognizes digits via a custom OCR model (ONNX)
3. Lets the user correct OCR mistakes
4. Solves the puzzle using logical strategies + DFS backtracking
5. Retrains the OCR model from corrected samples

**Tech Stack**: C++17, Qt 6 Widgets, OpenCV 4 (vcpkg), PyTorch (Python retraining scripts).

## Repository Layout

```
.
├── main.cpp                          # Qt entry point
├── Sudoku_demo.cpp/.h                # Main window, UI wiring, retrain orchestration
├── SudokuImageRecognizer.cpp/.h      # Board detection + OCR inference pipeline
├── RobustSudokuDetector.cpp/.h       # OpenCV grid line detection and perspective warp
├── SudokuSolver.cpp/.h               # Rule-based solver (naked/hidden singles/pairs) + DFS
├── SudokuGrid.cpp/.h                 # Solution grid display widget
├── OCR_Model/
│   ├── custom_model.onnx             # Runtime ONNX model for C++ inference
│   ├── custom_model.pth              # PyTorch checkpoint for retraining base
│   ├── train.py                      # Full training script (from scratch)
│   ├── retrain_custom_model.py       # Incremental retraining script (called by app)
│   └── retrain_data/train/           # Corrected cell images saved by the app
├── Character_Sample/                 # Synthetic digit dataset (1-9), ~8300 images
│   └── generate_dataset.py           # Script to regenerate the synthetic dataset
├── Sudoku_Sample/                    # Example Sudoku images for manual testing
├── CMakeLists.txt                    # Cross-platform CMake build
├── Sudoku_demo.vcxproj               # Visual Studio project (primary build)
├── vcpkg.json                        # OpenCV dependency declaration
├── installer.iss                     # Inno Setup installer script
└── .github/workflows/release.yml     # CI release workflow
```

## Architecture

### C++ Application

- **Entry**: `main.cpp` → creates `QApplication` + `Sudoku_demo` widget.
- **Recognition pipeline**: `Sudoku_demo::onRecognize()` → `SudokuImageRecognizer::processImage()` → `RobustSudokuDetector::detect()` (grid extraction) → `splitIntoCells()` → `recognizeDigit()` (OpenCV DNN with ONNX model).
- **Solver**: `SudokuSolver::solve()` applies naked singles, hidden singles, naked pairs, hidden pairs in a loop, then falls back to DFS backtracking with MRV heuristic.
- **Retraining**: `Sudoku_demo::onRetrain()` validates the board, saves cropped cell images to `retrain_data/`, then spawns `retrain_custom_model.py` via `QProcess`.

### Python Training

- **Model**: `SudokuDigitCNN` — 3×(Conv2d→BN→ReLU→MaxPool) → AdaptiveAvgPool(4,4) → FC(1024→128→10). ~160K parameters.
- **Input**: 224×224 RGB images.
- **Device detection**: Intel XPU > CUDA > CPU.
- **Output**: `.pth` (PyTorch weights) + `.onnx` (exported for OpenCV DNN inference).

### Data Flow (Retraining)

```
User corrects OCR → App validates board is solvable →
App crops cells & saves to retrain_data/train/<digit>/ →
App launches retrain_custom_model.py via QProcess →
Script combines Character_Sample + retrain_data →
Script fine-tunes from base .pth → Exports new .pth + .onnx →
App detects "saved_onnx=" in stdout → Next recognition uses updated model
```

## Build Instructions

### Windows / Visual Studio (Primary)

1. Open `Sudoku_demo.slnx` or `Sudoku_demo.vcxproj` in Visual Studio 2022.
2. Ensure Qt VS Tools can resolve the `6.8.0_msvc2022_64` installation (or update `QtInstall` in `.vcxproj`).
3. Select `x64` configuration.
4. Build → Run.

### CMake (Cross-Platform)

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

### Python Environment

```bash
python -m venv .venv
.venv\Scripts\activate
pip install torch torchvision Pillow onnx
```

## Coding Conventions

### C++

- **Standard**: C++17, MSVC v143.
- **Naming**: PascalCase for classes (`SudokuSolver`), camelCase for methods/variables (`onRetrain`, `m_trainingProcess`).
- **Member prefix**: `m_` for private member variables.
- **Qt signals/slots**: Use `connect()` with function pointers (not string-based `SIGNAL`/`SLOT`).
- **OpenCV**: Use `cv::Mat` for image data; `cv::dnn` for ONNX inference.
- **Error handling**: Use `bool` return values with diagnostic strings; avoid exceptions in hot paths.
- **Comments**: Chinese comments are acceptable and common throughout the codebase.

### Python

- **Model class**: `SudokuDigitCNN` must be kept in sync between `train.py` and `retrain_custom_model.py`. Any architecture change requires updating both files.
- **Output protocol**: `retrain_custom_model.py` prints `epoch=N/M loss=X acc=Y%` (parsed by C++ via regex) and `saved_onnx=<path>` on completion.
- **Safety**: Temporary `.pth.tmp`/`.onnx.tmp` files are written first, size-checked (>1024 bytes), then atomically replaced. Previous files are backed up to `.bak`.

## Key Constraints

- **ONNX opset**: Currently `opset_version=11`. Do not change without verifying OpenCV DNN compatibility.
- **Input size**: The model expects `224×224` RGB. If changed, update `train.py`, `retrain_custom_model.py`, `generate_dataset.py`, `SudokuImageRecognizer::preprocessDigitForCustomModel()`, and `SudokuImageRecognizer::recognizeDigit()`.
- **Class labels**: Classes 1-9 correspond to digits 1-9. Class 0 is "empty cell" (used only in retrain_data). `Character_Sample/` folders are named 1-9 (no folder 0).
- **Model files**: `custom_model.pth` and `custom_model.onnx` must coexist. The `.pth` is the retraining base; the `.onnx` is the inference model. Never ship one without the other.
- **Writable data dir**: At runtime, the app writes retrained models to `QStandardPaths::AppDataLocation`. The bundled model in `OCR_Model/` is the fallback.

## Testing

### Manual Test

1. Launch the app.
2. Load an image from `Sudoku_Sample/`.
3. Click **Recognize** → verify digits in OCR table.
4. Correct any wrong cells → click **Solve** → verify solution.
5. Click **Retrain** → wait for training to complete → re-recognize and verify improvement.

### Python Training Smoke Test

```bash
cd OCR_Model
python train.py
# Expect: ~50 epochs of output ending with model export messages
```

### Retrain Script Smoke Test

```bash
cd OCR_Model
python retrain_custom_model.py \
  --base-data ../Character_Sample \
  --retrain-data retrain_data \
  --base-pth custom_model.pth \
  --output-pth custom_model_test.pth \
  --output-onnx custom_model_test.onnx \
  --epochs 2
```

## Performance Notes

- **Training bottleneck**: The 224×224 input size is the dominant factor. See the training acceleration analysis for detailed recommendations.
- **Inference**: 81 cells × single-cell ONNX inference on CPU takes ~300-500ms at 224×224. Reducing input size to 64×64 would bring this under 50ms.
- **Grid detection**: `RobustSudokuDetector` uses probabilistic Hough transform; performance depends on image resolution but is typically <100ms.
- **Solver**: Rule-based strategies + DFS backtracking solves most puzzles in <1ms.

## Common Pitfalls

1. **Model architecture mismatch**: If you change `SudokuDigitCNN` in one file, you must update the other. The `.pth` won't load if the architecture differs.
2. **Label offset**: `train.py` uses `ImageFolder` with `target_transform=lambda y: y + 1` because folder names start at "1" but `ImageFolder` assigns labels starting at 0. `retrain_custom_model.py` uses `DigitFolderDataset` which directly uses folder names as labels. Both produce the same label mapping (1-9).
3. **Path resolution**: The app searches multiple paths for the ONNX model (`AppDataLocation` → `applicationDirPath()` → `CWD` → relative paths). When debugging, check which model file is actually being loaded.
4. **Unicode paths**: On Windows, `cv::imread`/`cv::imwrite` may fail with non-ASCII paths. The app uses `toLocal8Bit()` for conversion.

# Sudoku Demo

Sudoku Demo is a Windows desktop application that recognizes Sudoku puzzles from images, lets the user correct OCR mistakes, solves the puzzle, and can retrain its digit OCR model from corrected samples.

The application is built with Qt Widgets and C++. Image processing and model inference are handled by OpenCV. The OCR model is a small PyTorch CNN exported to ONNX and loaded through OpenCV DNN at runtime.

## Main Features

- Load a Sudoku image from disk.
- Detect the Sudoku board and warp it into a normalized 9x9 grid.
- Split the board into 81 cells and recognize digits with `OCR_Model/custom_model.onnx`.
- Show the recognized puzzle in an editable 9x9 table.
- Solve the corrected puzzle with a rule-based solver plus DFS backtracking.
- Save corrected cell crops and retrain the OCR model from inside the app.

## Repository Layout

```text
.
|-- main.cpp                         # Qt application entry point
|-- Sudoku_demo.cpp/.h               # Main window, user flow, retraining orchestration
|-- SudokuImageRecognizer.cpp/.h     # Board detection + cell OCR pipeline
|-- RobustSudokuDetector.cpp/.h      # OpenCV grid detection and perspective warp
|-- SudokuSolver.cpp/.h              # Sudoku solving logic
|-- SudokuGrid.cpp/.h                # Solution grid widget
|-- OCR_Model/
|   |-- custom_model.onnx            # Runtime OCR model used by C++ inference
|   |-- custom_model.pth             # PyTorch weights used as retraining base
|   |-- retrain_custom_model.py      # Incremental retraining/export script
|   `-- retrain_data/train/          # Corrected samples saved by the app
|-- Character_Sample/                # Base synthetic digit dataset, folders 1-9
|-- Sudoku_Sample/                   # Example Sudoku images for manual testing
|-- vcpkg.json                       # OpenCV dependency declaration
`-- Sudoku_demo.vcxproj              # Visual Studio / Qt VS Tools project
```

## Runtime Flow

1. `main.cpp` creates the Qt application and opens `Sudoku_demo`.
2. The user clicks **Load Image** and selects a PNG/JPG/BMP Sudoku image.
3. The user clicks **Recognize**.
4. `SudokuImageRecognizer::processImage()` loads the image with OpenCV.
5. `RobustSudokuDetector::detect()` preprocesses the image, detects horizontal and vertical grid lines, clusters them, selects 10 evenly spaced lines in each direction, and warps the board to a fixed 450x450 image.
6. `SudokuImageRecognizer::splitIntoCells()` cuts the warped board into 81 cells.
7. `SudokuImageRecognizer::recognizeDigit()` checks whether each cell contains foreground pixels. Non-empty cells are resized to 224x224 and passed into `custom_model.onnx` through OpenCV DNN.
8. The recognized digits are displayed in the editable OCR table. Empty cells are represented as `0` internally and shown blank in the UI.
9. The user can click cells and type `0`-`9` to correct recognition errors.
10. The user clicks **Solve**.
11. `SudokuSolver::solve()` first applies logical strategies such as naked singles, hidden singles, naked pairs, and hidden pairs. If the puzzle is still incomplete, it falls back to DFS backtracking with a minimum-candidate heuristic.
12. The solved board is shown in the solution panel. Original givens are tracked with a mask so they can be visually distinguished from solved values.

## Retraining Flow

The **Retrain** button uses the corrected OCR grid as supervised data for the digit model.

1. The app reads the current OCR table and validates that all cells are in the range `0`-`9`.
2. The solver checks that the corrected puzzle is solvable. If it is not solvable, retraining is blocked because the labels are likely wrong.
3. The original image is processed again to obtain the warped 450x450 board.
4. `saveTrainingSamples()` crops each non-empty corrected cell and writes it to:

```text
OCR_Model/retrain_data/train/<digit>/<timestamp>_r<row>_c<col>.png
```

5. The app starts `OCR_Model/retrain_custom_model.py` through `QProcess`.
6. The script combines the base dataset from `Character_Sample/` with the collected samples in `OCR_Model/retrain_data/train/`.
7. It loads `OCR_Model/custom_model.pth`, trains for the requested number of epochs, writes temporary model files, backs up the previous model, then atomically replaces:

```text
OCR_Model/custom_model.pth
OCR_Model/custom_model.onnx
```

8. The next recognition run uses the updated ONNX model.

## Requirements

### C++ Application

- Windows x64
- Visual Studio 2022 with MSVC v143
- Qt 6 for MSVC 2022 64-bit
- Qt Visual Studio Tools / Qt MSBuild support
- OpenCV 4, supplied through vcpkg integration in this project

The project file currently references this Qt installation name:

```text
6.11.0_msvc2022_64
```

If your Qt installation uses a different name, update the `QtInstall` value in `Sudoku_demo.vcxproj` or configure it through Qt VS Tools.

### Python Retraining

Retraining requires Python packages used by `OCR_Model/retrain_custom_model.py`:

```text
torch
torchvision
Pillow
onnx
```

The app first tries to run:

```text
.venv/Scripts/python.exe
```

If that file does not exist, it falls back to `python` from `PATH`.

## Build and Run

1. Open `Sudoku_demo.slnx` or `Sudoku_demo.vcxproj` in Visual Studio.
2. Select `x64` and either `Debug` or `Release`.
3. Make sure Qt VS Tools can resolve the configured Qt installation.
4. Build the project.
5. Run the generated executable from Visual Studio.

For image recognition to work, `OCR_Model/custom_model.onnx` must be reachable from either the working directory or the app directory layout expected by `SudokuImageRecognizer`.

## Manual Test Procedure

1. Launch the application.
2. Click **Load Image**.
3. Select one of the images in `Sudoku_Sample/`.
4. Click **Recognize**.
5. Inspect the OCR table and correct wrong cells if needed.
6. Click **Solve**.
7. If the solution succeeds and OCR corrections were made, click **Retrain** to add the corrected cells to the training set and update the model.

## Notes and Limitations

- The board detector expects visible Sudoku grid lines. Very low-resolution, heavily cropped, rotated, or low-contrast images may fail detection.
- The detector writes `debug_preprocess.png` in the working directory to help inspect preprocessing failures.
- OCR confidence below `0.60` is treated as an empty cell.
- Retraining depends on valid `custom_model.pth` weights. If the base weights are missing or too small, the script stops instead of overwriting the model.
- Retraining replaces the model only after temporary `.pth` and `.onnx` exports pass a basic size check. Existing model files are copied to `.bak` before replacement.

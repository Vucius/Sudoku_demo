cmake_minimum_required(VERSION 3.21)

project(SudokuDemo VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)
find_package(OpenCV REQUIRED COMPONENTS core imgproc imgcodecs dnn)

set(APP_SOURCES
    main.cpp
    Sudoku_demo.cpp
    Sudoku_demo.h
    SudokuGrid.cpp
    SudokuGrid.h
    SudokuSolver.cpp
    SudokuSolver.h
    SudokuImageRecognizer.cpp
    SudokuImageRecognizer.h
    RobustSudokuDetector.cpp
    RobustSudokuDetector.h
    Sudoku_demo.ui
    Sudoku_demo.qrc
)

qt_add_executable(Sudoku_demo
    ${APP_SOURCES}
)

target_link_libraries(Sudoku_demo PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ${OpenCV_LIBS}
)

if(WIN32)
    set_target_properties(Sudoku_demo PROPERTIES WIN32_EXECUTABLE TRUE)
elseif(APPLE)
    set_target_properties(Sudoku_demo PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_BUNDLE_NAME "Sudoku Demo"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.daedalus.sudokudemo"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
    )
endif()

set(APP_RESOURCE_DIRS
    Character_Sample
    OCR_Model
    Sudoku_Sample
)

foreach(resource_dir IN LISTS APP_RESOURCE_DIRS)
    add_custom_command(TARGET Sudoku_demo POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/${resource_dir}"
            "$<TARGET_FILE_DIR:Sudoku_demo>/${resource_dir}"
    )

    if(APPLE)
        add_custom_command(TARGET Sudoku_demo POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_SOURCE_DIR}/${resource_dir}"
                "$<TARGET_BUNDLE_CONTENT_DIR:Sudoku_demo>/Resources/${resource_dir}"
        )
    endif()
endforeach()

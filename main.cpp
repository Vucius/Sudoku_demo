#include "Sudoku_demo.h"
#include <QApplication>
#include <QFont>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // Use a clean system font
    QFont appFont("Segoe UI", 10);
    app.setFont(appFont);

    Sudoku_demo window;
    window.show();
    return app.exec();
}

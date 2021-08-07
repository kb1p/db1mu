#include <QApplication>
#include "b1mainwindow.h"

#include <iostream>
#include <cstdlib>

int main(int argc, char **argv) 
{
    int rv;
    if (argc > 1 && (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0))
    {
        std::cout << "Usage: " << argv[0] << " [<iNES-ROM-file>]" << std::endl;

        rv = EXIT_SUCCESS;
    }
    else
    {
        QCoreApplication::setAttribute(Qt::AA_UseOpenGLES);

        QApplication app(argc, argv);

        b1MainWindow wnd;
        wnd.show();

        rv = app.exec();
    }

    return rv;
}

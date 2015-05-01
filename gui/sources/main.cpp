#include <QApplication>
#include "b1mainwindow.h"

int main(int argc, char **argv) 
{
    QApplication app(argc, argv);
    
    b1MainWindow wnd;
    wnd.show();
    
    return app.exec();
}

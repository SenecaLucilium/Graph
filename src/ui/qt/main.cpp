#include "MainWindow.h"

#include <QApplication>

int main(int argc, char* argv[])
{
    QApplication application(argc, argv);
    src::ui::qt::MainWindow window;
    window.show();
    return application.exec();
}

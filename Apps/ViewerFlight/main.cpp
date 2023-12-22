#include "MainWindow.h"
#include <QApplication>
#include <osgEarth/Common>
#include "Utils/Parse.h"
int main(int argc, char** argv)
{
    osgEarth::initialize();
    QApplication app(argc, argv);

    MainWindow mainwinndow;

    mainwinndow.show();
    return app.exec();
}

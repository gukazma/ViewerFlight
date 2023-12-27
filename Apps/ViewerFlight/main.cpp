#include "MainWindow.h"
#include <QApplication>
#include <UAVMVS/Context.hpp>
int main(int argc, char** argv)
{
    uavmvs::context::Init();
    QApplication app(argc, argv);

    MainWindow mainwinndow;

    mainwinndow.show();
    return app.exec();
}

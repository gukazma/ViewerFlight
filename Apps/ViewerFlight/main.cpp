#include "MainWindow.h"
#include <QApplication>
#include <UAVMVS/Context.hpp>
#include <QTranslator>
int main(int argc, char** argv)
{
    uavmvs::context::Init();
    QApplication app(argc, argv);

    QTranslator* translator = new QTranslator;
    translator->load(":/Translations/zh_CN.qm");
    app.installTranslator(translator);

    MainWindow mainwinndow;

    mainwinndow.show();
    return app.exec();
}

#include "MainWindow.h"
#include <QApplication>
#include <QSplashScreen>
#include <UAVMVS/Context.hpp>
#include <QTranslator>
int main(int argc, char** argv)
{
    uavmvs::context::Init();
    QApplication app(argc, argv);
    QPixmap       pixmap(QString::fromLocal8Bit(":/splashScreen.png"));
    QSplashScreen splash(pixmap);
    splash.show();

    QTranslator* translator = new QTranslator;
    translator->load(":/Translations/zh_CN.qm");
    app.installTranslator(translator);

    MainWindow mainwindow;
    mainwindow.showMaximized();

    splash.finish(&mainwindow);
    return app.exec();
}

#include "MainWindow.h"
#include <QApplication>
#include <QSplashScreen>
#include <UAVMVS/Context.hpp>
#include <QTranslator>
#include <QFile>
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

    /*QFile styleFile(":/style.qss");
    if (styleFile.open(QFile::ReadOnly)) {
        QString styleSheet = QLatin1String(styleFile.readAll());
        app.setStyleSheet(styleSheet);
    }*/

    MainWindow mainwindow;
    mainwindow.showMaximized();

    splash.finish(&mainwindow);
    return app.exec();
}

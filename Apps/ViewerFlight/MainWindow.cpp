#include "MainWindow.h"
#include "ui_MainWindow.h"
#include <QLineEdit>
#include <QRegExpValidator>
#include <QPushButton>
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    // 经度
    QLineEdit* lineEdit_Longitude = new QLineEdit(this);
    lineEdit_Longitude->setGeometry(100, 100, 180, 80);
    lineEdit_Longitude->setMaximumWidth(100);
    QRegExp regExp_Longitude(QString::fromLocal8Bit("(E|W)?[0-1][0-7]\\d°[0-5]\\d'[0-5]\\d\""));
    lineEdit_Longitude->setValidator(new QRegExpValidator(regExp_Longitude, lineEdit_Longitude));
    lineEdit_Longitude->setInputMask(QString::fromLocal8Bit("A000°00'00\""));
    lineEdit_Longitude->setText(QString::fromLocal8Bit("E000°00'00\""));

    // 纬度
    QLineEdit* lineEdit_Latitude = new QLineEdit(this);
    lineEdit_Latitude->setGeometry(100, 200, 180, 80);
    lineEdit_Latitude->setMaximumWidth(100);
    QRegExp regExp_Latitude(QString::fromLocal8Bit("(N|S)?[0-8]\\d°[0-5]\\d'[0-5]\\d\""));
    lineEdit_Latitude->setValidator(new QRegExpValidator(regExp_Latitude, lineEdit_Latitude));
    lineEdit_Latitude->setInputMask(QString::fromLocal8Bit("A00°00'00\""));
    lineEdit_Latitude->setText(QString::fromLocal8Bit("N00°00'00\""));

    QPushButton* viewButton = new QPushButton("View", this);
    viewButton->setMaximumWidth(50);
    QWidget*     widget   = new QWidget(this);
    QHBoxLayout* layout   = new QHBoxLayout(widget);
    layout->setAlignment(Qt::AlignLeft);
    layout->addWidget(lineEdit_Longitude);
    layout->addWidget(lineEdit_Latitude);
    layout->addWidget(viewButton);
    widget->setLayout(layout);

    // 将QWidget部件添加到工具栏
    ui->toolBar->addWidget(widget);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_actionHome_triggered()
{
    ui->osgviewer->home();
}

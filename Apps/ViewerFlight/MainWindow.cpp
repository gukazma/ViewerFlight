#include "MainWindow.h"
#include "ui_MainWindow.h"
#include <QLineEdit>
#include <QRegExpValidator>
#include <QPushButton>
#include <QLabel>
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    // 经度
    QLineEdit* lineEdit_Longitude = new QLineEdit(this);
    lineEdit_Longitude->setGeometry(100, 100, 180, 80);
    lineEdit_Longitude->setMaximumWidth(100);
    lineEdit_Longitude->setText("0.000");

    // 纬度
    QLineEdit* lineEdit_Latitude = new QLineEdit(this);
    lineEdit_Latitude->setGeometry(100, 200, 180, 80);
    lineEdit_Latitude->setMaximumWidth(100);
    lineEdit_Latitude->setText("0.000");
    

    QPushButton* viewButton = new QPushButton(tr ("View"), this);
    viewButton->setMaximumWidth(50);
    QWidget*     widget   = new QWidget(this);
    QHBoxLayout* layout   = new QHBoxLayout(widget);
    layout->setAlignment(Qt::AlignLeft);
    QLabel* label_Longitude = new QLabel(tr("Longitude"));
    label_Longitude->setMaximumWidth(50);
    QLabel* label_Latitude = new QLabel(tr("Latitude"));
    label_Latitude->setMaximumWidth(50);
    layout->addWidget(label_Longitude);
    layout->addWidget(lineEdit_Longitude);
    layout->addWidget(label_Latitude);
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


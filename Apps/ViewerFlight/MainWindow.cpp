#include "MainWindow.h"
#include "ui_MainWindow.h"
#include <QLineEdit>
#include <QRegExpValidator>
#include <QFileDialog>
#include <QPushButton>
#include <QDragEnterEvent>
#include <QLabel>
#include <QMimeData>
#include <QProgressDialog>
#include <QMessageBox>
#include <QtConcurrentMap>
#include <UAVMVS/Context.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <osgDB/ReadFile>
#include <osg/ComputeBoundsVisitor>
#include <iostream>
MainWindow::MainWindow(QWidget *parent_) : QMainWindow(parent_)
    ,
    ui(new Ui::MainWindow)
{   
    ui->setupUi(this);
    createProgressDialog();

    setAcceptDrops(true);
    // 经度
    QLineEdit* lineEdit_Longitude = new QLineEdit(this);
    lineEdit_Longitude->setGeometry(100, 100, 180, 80);
    lineEdit_Longitude->setMaximumWidth(100);
    lineEdit_Longitude->setText("121.31");

    // 纬度
    QLineEdit* lineEdit_Latitude = new QLineEdit(this);
    lineEdit_Latitude->setGeometry(100, 200, 180, 80);
    lineEdit_Latitude->setMaximumWidth(100);
    lineEdit_Latitude->setText("31.16");
    

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

    QFrame* separator = new QFrame(this);
    separator->setFrameShape(QFrame::VLine);
    separator->setFrameShadow(QFrame::Sunken);
    layout->addWidget(separator);

    QPushButton* addLayerButton = new QPushButton(tr("Add layer"));
    QPushButton* homeLayerButton = new QPushButton(tr("Home layer"));
    addLayerButton->setMaximumWidth(100);
    homeLayerButton->setMaximumWidth(130);
    layout->addWidget(addLayerButton);
    layout->addWidget(homeLayerButton);
    widget->setLayout(layout);
    
    // 将QWidget部件添加到工具栏
    ui->toolBar->addWidget(widget);

    connect(viewButton, &QPushButton::clicked, [=]() { 
        float longitue = lineEdit_Longitude->text().toFloat();
        float latitude = lineEdit_Latitude->text().toFloat();
        uavmvs::context::View(osgEarth::Viewpoint("", longitue, latitude, 5000, 0, -90, 1000), 5);
    });

    connect(homeLayerButton, &QPushButton::clicked, [=]() {
        uavmvs::context::HomeLayerView();
    });

    connect(addLayerButton, &QPushButton::clicked, [=]() {
        auto layerDir = QFileDialog::getExistingDirectory(nullptr, tr("Open layer dir"));
        if (!layerDir.isEmpty()) {
            try {
                addLayer(layerDir);
            }
            catch (const std::exception& e) {
                QMessageBox::warning(nullptr, "", e.what());
            }
        }
    });
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::dragEnterEvent(QDragEnterEvent* event_) {
    if (event_->mimeData()->hasUrls() && event_->mimeData()->urls().size() == 1) {
        const QUrl url = event_->mimeData()->urls().at(0);
        if (url.isLocalFile() && QFileInfo(url.toLocalFile()).isDir()) {
            event_->acceptProposedAction();
            return;
        }
    }
    event_->ignore();
}

void MainWindow::dropEvent(QDropEvent* event_)
{
    const QUrl    url        = event_->mimeData()->urls().at(0);
    const QString folderPath = url.toLocalFile();
    std::cout << "folder: " << folderPath.toStdString() << std::endl;
    try {
        addLayer(folderPath);
    }
    catch (const std::exception& e) {
        QMessageBox::warning(nullptr, "", e.what());
    }
    event_->acceptProposedAction();
}

void MainWindow::addLayer(const QString& dir_) {
    boost::filesystem::path dir          = dir_.toLocal8Bit().constData();
    boost::filesystem::path metadataPath = dir / "metadata.xml";
    if (!boost::filesystem::exists(metadataPath)) {
        throw std::runtime_error("metadata.xml not exits!");
        return;
    }
    uavmvs::context::SetupMetadata(metadataPath);
    
    std::vector<boost::filesystem::path> tilePaths;
    for (const auto& entry : boost::filesystem::directory_iterator(dir)) {
        if (boost::filesystem::is_directory(entry) &&
            boost::algorithm::contains(entry.path().filename().string(), "Tile_")) {
            auto tilePath = entry.path() / entry.path().filename();
            tilePath.replace_extension(".osgb");
            if (boost::filesystem::exists(tilePath)) {
                tilePaths.push_back(tilePath);
            }
        }
    }
    osg::ref_ptr<osg::Group> tiles = new osg::Group;
    osg::BoundingBox       bbox;
    std::mutex                           modelsMutex;

    auto readOsgb = [&](const boost::filesystem::path& tilePath) {
        auto node = osgDB::readRefNodeFile(tilePath.generic_string().c_str());
        osg::ComputeBoundsVisitor computeBoundsVisitor;
        node->accept(computeBoundsVisitor);
        std::lock_guard<std::mutex> lk(modelsMutex);
        bbox.expandBy(computeBoundsVisitor.getBoundingBox());
        tiles->addChild(node);
    };

    m_futureWather.setFuture(QtConcurrent::map(tilePaths, readOsgb));
    m_prgDialog->setVisible(true);
    m_prgDialog->exec();
    m_futureWather.waitForFinished();
    uavmvs::context::AddLayer(tiles, bbox);
}

void MainWindow::createProgressDialog() {
    if (!m_prgDialog) {
        m_prgDialog = new QProgressDialog(this);
        m_prgDialog->close();
        connect(&m_futureWather, &QFutureWatcher<void>::finished, [&]() { 
            if (m_prgDialog) {
                m_prgDialog->setVisible(false);
            }
        });
        connect(m_prgDialog,
                &QProgressDialog::canceled,
                &m_futureWather,
                &QFutureWatcher<void>::cancel);
        connect(&m_futureWather,
                &QFutureWatcher<void>::progressRangeChanged,
                m_prgDialog,
                &QProgressDialog::setRange);
        connect(&m_futureWather,
                &QFutureWatcher<void>::progressValueChanged,
                m_prgDialog,
                &QProgressDialog::setValue);
    }
}



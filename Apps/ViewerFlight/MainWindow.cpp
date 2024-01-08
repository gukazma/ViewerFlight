#include "MainWindow.h"
#include "ui_MainWindow.h"
#include <QLineEdit>
#include <QRegExpValidator>
#include <QFileDialog>
#include <QPushButton>
#include <QToolButton>
#include <QDragEnterEvent>
#include <QLabel>
#include <QMimeData>
#include <QMenu>
#include <QCheckBox>
#include <QComboBox>
#include <QSpinBox>
#include <QLineEdit>
#include <QTextEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QProgressDialog>
#include <QMessageBox>
#include <QtConcurrentMap>
#include <UAVMVS/Context.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <osgDB/ReadFile>
#include <osg/ComputeBoundsVisitor>
#include <iostream>
#include <TabToolbar/TabToolbar.h>
#include <TabToolbar/Page.h>
#include <TabToolbar/Group.h>
#include <TabToolbar/SubGroup.h>
#include <TabToolbar/StyleTools.h>
#include <TabToolbar/Builder.h>
#include "SettingsDialog.h"
MainWindow::MainWindow(QWidget *parent_) : QMainWindow(parent_)
    , ui(new Ui::MainWindow)
    , m_settingsDialog(new SettingsDialog)
{   
    ui->setupUi(this);
    createProgressDialog();

    setAcceptDrops(true);
    tt::Builder ttb(this);
    ttb.SetCustomWidgetCreator("lineEdit", []() { return new QLineEdit(); });

    tt::TabToolbar* tabToolbar = ttb.CreateTabToolbar(":/tt/tabtoolbar.json");
    addToolBar(Qt::TopToolBarArea, tabToolbar);
    QObject::connect(tabToolbar, &tt::TabToolbar::SpecialTabClicked, this, [this]() {
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
    QLineEdit* lineEdit_Longitude = (QLineEdit*)ttb["lineEdit_Longitude"];
    lineEdit_Longitude->setGeometry(100, 100, 180, 80);
    lineEdit_Longitude->setMaximumWidth(100);
    lineEdit_Longitude->setText("121.306427");
    QLineEdit* lineEdit_Latitude = (QLineEdit*)ttb["lineEdit_Latitude"];
    lineEdit_Latitude->setGeometry(100, 200, 180, 80);
    lineEdit_Latitude->setMaximumWidth(100);
    lineEdit_Latitude->setText("31.163324");

    connect(ui->actionView, &QAction::triggered, [=]() {
            float longitue = lineEdit_Longitude->text().toFloat();
            float latitude = lineEdit_Latitude->text().toFloat();
            uavmvs::context::View(osgEarth::Viewpoint("", longitue, latitude, 5000, 0, -90, 1000),
            5);
        });

    connect(ui->actionDrawRange, &QAction::triggered, [=]() { uavmvs::context::DrawRange(); });
    connect(ui->actionSettings, &QAction::triggered, [=]() { 
            m_settingsDialog->show();
        });
    connect(ui->actionViewReset, &QAction::triggered, [=]() { uavmvs::context::HomeLayerView(); });
    connect(ui->actionShowDiskPoints, &QAction::triggered, [=](bool checked) {
        uavmvs::context::ShowDiskPoints(checked);
    });
    connect(ui->actionShowAirspace, &QAction::triggered, [=](bool checked) {
        uavmvs::context::ShowAirspace(checked);
    });

    connect(ui->actionShowNormals, &QAction::triggered, [=](bool checked) {
        uavmvs::context::ShowDiskPointsNormals(checked);
    });
    connect(ui->actionGenerateWaypoints, &QAction::triggered, [=](bool checked) {
        uavmvs::context::GenerateWaypoints();
    });
    connect(
        ui->actionAirspace, &QAction::triggered, [=]() { 
            auto rangePoly = uavmvs::context::GetAirspaceRange();
            if (rangePoly.size() <= 2) {
                QMessageBox::critical(nullptr, "Error", QString::fromLocal8Bit("请先绘制安全罩范围"));
                return;
            }
            visitTile();

            std::vector<int> tasks = {1};
            m_futureWather.setFuture(
                QtConcurrent::map(tasks, [&](int) { uavmvs::context::GenerateAirspace(); }));
            m_prgDialog->setVisible(true);
            m_prgDialog->exec();
            m_futureWather.waitForFinished();

            QMessageBox::information(nullptr, "Info", QString::fromLocal8Bit("成功生成安全罩"));
        });
    connect(
        ui->actionDrawAirspaceRange, &QAction::triggered, [=]() { uavmvs::context::DrawAirspaceRange(); });
    connect(ui->actionPossionDisk, &QAction::triggered, [=]() { 
        auto rangePoly = uavmvs::context::GetRangePolygon();
        if (rangePoly.size() <= 2) {
            QMessageBox::critical(nullptr, "Error", QString::fromLocal8Bit("请先绘制采样范围"));
            return;
        }
        visitTile();
        std::vector<int> tasks = {1};
        m_futureWather.setFuture(QtConcurrent::map(tasks, [&](int) {
              uavmvs::context::PossionDiskSample(); 
            }));
        m_prgDialog->setVisible(true);
        m_prgDialog->exec();
        m_futureWather.waitForFinished();
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

void MainWindow::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    emit ui->osgviewer->resized();
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
    if (uavmvs::context::ViewLoadedTile(dir)) return;

    boost::filesystem::path metadataPath = dir / "metadata.xml";
    if (!boost::filesystem::exists(metadataPath)) {
        QMessageBox::warning(this, "", tr("metadata.xml not found!"));
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

void MainWindow::visitTile() {

    if (uavmvs::context::IsGeneratedTileMesh()) {
        return;
    }
    auto geoms = uavmvs::context::VisitTile();
    m_futureWather.setFuture(QtConcurrent::map(geoms, &uavmvs::context::AppendTile));
    m_prgDialog->setVisible(true);
    m_prgDialog->exec();
    m_futureWather.waitForFinished();
}



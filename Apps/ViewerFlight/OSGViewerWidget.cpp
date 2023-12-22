#include "OSGViewerWidget.h"
#include <osg/Camera>
#include <osg/Image>
#include <osg/ComputeBoundsVisitor>
#include <osg/PositionAttitudeTransform>
#include <osgDB/ReadFile>
#include <osgEarth/AutoClipPlaneHandler>
#include <osgEarth/AutoScaleCallback>
#include <osgEarth/EarthManipulator>
#include <osgEarth/ExampleResources>
#include <osgEarth/GeoData>
#include <osgEarth/GeoTransform>
#include <osgEarth/ModelLayer>
#include <osgEarth/Registry>
#include <osgEarth/Sky>
#include <osgEarth/SpatialReference>
#include <osgViewer/Viewer>
#include <osg/BlendFunc>
#include <boost/dll.hpp>
#include <boost/filesystem.hpp>
#include <osgGA/MultiTouchTrackballManipulator>
#include "Utils/CLabelControlEventHandler.h"

OSGViewerWidget::OSGViewerWidget(QWidget* parent) {
    setMouseTracking(true);
    connect(this, &osgQOpenGLWidget::initialized, this, &OSGViewerWidget::init);
}

OSGViewerWidget::~OSGViewerWidget() {}

void OSGViewerWidget::init()
{
    auto m_mainpulator = new osgEarth::Util::EarthManipulator;
    getOsgViewer()->setCameraManipulator(m_mainpulator);


    // load an earth file, and support all or our example command-line options
    boost::filesystem::path binPath = boost::dll::program_location().parent_path();
    auto                    resourcesPath = binPath / "osgEarth/china-simple.earth";
    auto                    node          = osgDB::readRefNodeFile(resourcesPath.generic_string());
    m_root     = new osg::Group();
    auto mapNode    = osgEarth::MapNode::findMapNode(node);
    auto map        = mapNode->getMap();

    osgEarth::SkyOptions skyOptions;   // 天空环境选项
    skyOptions.ambient() = 1;          // 环境光照水平(0~1)
    auto skyNode =
        osgEarth::SkyNode::create(skyOptions);   // 创建用于提供天空、照明和其他环境效果的类
    skyNode->setDateTime(
        osgEarth::DateTime(2021, 4, 15, 12 - 8));   // 配置环境的日期/时间。(格林尼治，时差8小时)
    skyNode->setEphemeris(new osgEarth::Util::Ephemeris);   // 用于根据日期/时间定位太阳和月亮的星历
    skyNode->setLighting(true);                             // 天空是否照亮了它的子图
    skyNode->addChild(mapNode);                             // 添加地图节点
    skyNode->attach(getOsgViewer());   // 将此天空节点附着到视图（放置天光）
    m_root->addChild(skyNode);
    m_root->addChild(node);
    getOsgViewer()->setSceneData(m_root);
    osgEarth::Util::Controls::LabelControl* positionLabel =
        new osgEarth::Util::Controls::LabelControl("", osg::Vec4(1.0, 1.0, 1.0, 1.0));
    getOsgViewer()->addEventHandler(new CLabelControlEventHandler(mapNode, positionLabel));
    m_root->addChild(osgEarth::Util::Controls::ControlCanvas::get(getOsgViewer()));
    osgEarth::Util::Controls::ControlCanvas* canvas =
        osgEarth::Util::Controls::ControlCanvas::get(getOsgViewer());
    canvas->addControl(positionLabel);

    // initialize a viewer:
    auto viewer = getOsgViewer();
    viewer->getCamera()->addCullCallback(new osgEarth::Util::AutoClipPlaneCullCallback(mapNode));

}

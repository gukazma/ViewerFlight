#include "Context.hpp"
#include <osgEarth/Common>
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
#include "Utils/Parse.h"

osgViewer::Viewer* _viewer;
osg::ref_ptr<osg::Group>                       _root;
osg::ref_ptr<osgEarth::Util::EarthManipulator> _cameraManipulator;

namespace uavmvs {
namespace context {
void Init()
{
    osgEarth::initialize();
}
void Attach(osgViewer::Viewer* viewer_)
{
    _viewer = viewer_;

    _cameraManipulator = new osgEarth::Util::EarthManipulator;
    _viewer->setCameraManipulator(_cameraManipulator);


    // load an earth file, and support all or our example command-line options
    boost::filesystem::path binPath       = boost::dll::program_location().parent_path();
    auto                    resourcesPath = binPath / "osgEarth/china-simple.earth";
    auto                    node          = osgDB::readRefNodeFile(resourcesPath.generic_string());
    _root                                = new osg::Group();
    auto mapNode                          = osgEarth::MapNode::findMapNode(node);
    auto map                              = mapNode->getMap();


    osgEarth::SkyOptions skyOptions;   // 天空环境选项
    skyOptions.ambient() = 0.1;        // 环境光照水平(0~1)
    auto skyNode =
        osgEarth::SkyNode::create(skyOptions);   // 创建用于提供天空、照明和其他环境效果的类
    skyNode->setDateTime(
        osgEarth::DateTime(2021, 4, 15, 0));   // 配置环境的日期/时间。(格林尼治，时差8小时)
    skyNode->setEphemeris(new osgEarth::Util::Ephemeris);   // 用于根据日期/时间定位太阳和月亮的星历
    skyNode->setLighting(true);                             // 天空是否照亮了它的子图
    skyNode->addChild(mapNode);                             // 添加地图节点
    skyNode->attach(_viewer);   // 将此天空节点附着到视图（放置天光）
    _root->addChild(skyNode);
    _viewer->realize();
    // m_root->addChild(node);
    _viewer->setSceneData(skyNode);
    osgEarth::Util::Controls::LabelControl* positionLabel =
        new osgEarth::Util::Controls::LabelControl("", osg::Vec4(1.0, 1.0, 1.0, 1.0));
    _viewer->addEventHandler(new CLabelControlEventHandler(mapNode, positionLabel));
    _root->addChild(osgEarth::Util::Controls::ControlCanvas::get(_viewer));
    osgEarth::Util::Controls::ControlCanvas* canvas =
        osgEarth::Util::Controls::ControlCanvas::get(_viewer);
    canvas->addControl(positionLabel);

    // initialize a viewer:
    _viewer->getCamera()->addCullCallback(new osgEarth::Util::AutoClipPlaneCullCallback(mapNode));
}
void View(const osgEarth::Viewpoint& viewpoint, int delta) {
    _cameraManipulator->setViewpoint(viewpoint, 4);
}
void Destory() {
    _root.release();
    _cameraManipulator.release();
}
}   // namespace context
}   // namespace uavmvs
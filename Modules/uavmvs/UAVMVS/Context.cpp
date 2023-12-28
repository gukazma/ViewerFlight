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
#include <osgEarth/ElevationQuery>
#include <osgEarth/SpatialReference>
#include <osgViewer/Viewer>
#include <osg/BlendFunc>
#include <boost/dll.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <osgGA/MultiTouchTrackballManipulator>
#include "Utils/CLabelControlEventHandler.h"
#include "Utils/Parse.h"

osgViewer::Viewer* _viewer;
osg::ref_ptr<osg::Group>                       _root;
osg::ref_ptr<osgEarth::Util::EarthManipulator> _cameraManipulator;
osg::ref_ptr<osgEarth::MapNode>                _mapNode;

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
    _mapNode                              = osgEarth::MapNode::findMapNode(node);
    auto map                              = _mapNode->getMap();


    osgEarth::SkyOptions skyOptions;   // 天空环境选项
    skyOptions.ambient() = 0.1;        // 环境光照水平(0~1)
    auto skyNode =
        osgEarth::SkyNode::create(skyOptions);   // 创建用于提供天空、照明和其他环境效果的类
    skyNode->setDateTime(
        osgEarth::DateTime(2021, 4, 15, 0));   // 配置环境的日期/时间。(格林尼治，时差8小时)
    skyNode->setEphemeris(new osgEarth::Util::Ephemeris);   // 用于根据日期/时间定位太阳和月亮的星历
    skyNode->setLighting(true);                             // 天空是否照亮了它的子图
    skyNode->addChild(_mapNode);                            // 添加地图节点
    skyNode->attach(_viewer);   // 将此天空节点附着到视图（放置天光）
    _root->addChild(skyNode);
    _viewer->realize();
    // m_root->addChild(node);
    _viewer->setSceneData(_root);
    // 创建 DatabasePager
    osgDB::DatabasePager* pager = new osgDB::DatabasePager;
    _viewer->setDatabasePager(pager);

    osgEarth::Util::Controls::LabelControl* positionLabel =
        new osgEarth::Util::Controls::LabelControl("", osg::Vec4(1.0, 1.0, 1.0, 1.0));
    _viewer->addEventHandler(new CLabelControlEventHandler(_mapNode, positionLabel));
    _root->addChild(osgEarth::Util::Controls::ControlCanvas::get(_viewer));
    osgEarth::Util::Controls::ControlCanvas* canvas =
        osgEarth::Util::Controls::ControlCanvas::get(_viewer);
    canvas->addControl(positionLabel);

    // initialize a viewer:
    _viewer->getCamera()->addCullCallback(new osgEarth::Util::AutoClipPlaneCullCallback(_mapNode));
}
void View(const osgEarth::Viewpoint& viewpoint, int delta) {
    _cameraManipulator->setViewpoint(
        viewpoint, 4);
}
void AddLayer(const QString& _dir) {
    boost::filesystem::path dir = _dir.toLocal8Bit().constData();
    boost::filesystem::path metadataPath = dir / "metadata.xml";
    if (!boost::filesystem::exists(metadataPath)) {
        throw std::runtime_error("metadata.xml not exits!");
        return;
    }
    auto geopoint = ParseMetaDataFile(metadataPath);
    osg::ref_ptr<osgEarth::GeoTransform> xform = new osgEarth::GeoTransform();
    // xform->setPosition(point);
    xform->setTerrain(_mapNode->getTerrain());
    osg::BoundingBox bbox;
    for (const auto& entry : boost::filesystem::directory_iterator(dir)) {
        if (boost::filesystem::is_directory(entry) &&
            boost::algorithm::contains(entry.path().filename().string(), "Tile_")) {
            auto tilePath = entry.path() / entry.path().filename();
            tilePath.replace_extension(".osgb");
            if (boost::filesystem::exists(tilePath)) {
                auto node = osgDB::readNodeFile(tilePath.generic_string()); 
                osg::ComputeBoundsVisitor computeBoundsVisitor;
                node->accept(computeBoundsVisitor);
                bbox.expandBy(computeBoundsVisitor.getBoundingBox());
                xform->addChild(node);
            }
        }
    }

    // 查询海拔高度
    osgEarth::ElevationQuery elevationQuery(_mapNode->getMap());
    double elevation = 0.0;
    elevationQuery.getElevation(geopoint, elevation);
    geopoint.z() = elevation - bbox.zMin();
    std::cout << geopoint.z() << std::endl;
    osgEarth::Viewpoint vp;
    vp.focalPoint() = geopoint;
    xform->setPosition(geopoint);

    vp.setHeading(std::string("0"));
    vp.setPitch(std::string("-45"));
    vp.setRange(std::string("5000"));
    View(vp, 5);

    _root->addChild(xform);
}
void Destory() {
    /*_root.release();
    _cameraManipulator.release();*/
}
}   // namespace context
}   // namespace uavmvs
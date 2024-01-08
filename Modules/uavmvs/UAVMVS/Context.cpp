#include "Context.hpp"
#include <osgEarth/Common>
#include <osg/Camera>
#include <osg/Image>
#include <osg/ComputeBoundsVisitor>
#include <osg/PositionAttitudeTransform>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
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
#include <memory>
#include <unordered_map>
#include <tbb/tbb.h>
#include "Utils/CLabelControlEventHandler.h"
#include "Utils/Parse.h"
#include "Utils/HUDAxis.h"
#include "Utils/EventHandler.h"
#include "Utils/DrawableVisitor.h"
#include "UAVMVS/Mesh/Mesh.h"
#include <osgEarth/PointDrawable>



osgViewer::Viewer* _viewer;
osg::ref_ptr<osg::Group>                       _root;
osg::ref_ptr<osg::Group>                                             _currentTile;
osg::ref_ptr<osgEarth::Util::EarthManipulator> _cameraManipulator;
osg::ref_ptr<osgEarth::MapNode>                _mapNode;
osg::ref_ptr<EventHandler>                     _eventHandler;
std::shared_ptr<osgEarth::GeoPoint>                                  _layerGeoPoint;
std::unordered_map<std::string, std::shared_ptr<osgEarth::GeoPoint>>
    _loadedTileGeoPoint;
std::shared_ptr<osg::BoundingBox>                                    _layerBoudingBox;
DrawableVistor                                                      _visitor;
osg::ref_ptr<osgEarth::GeoTransform>                                 _diskPointNode;
osg::ref_ptr<osgEarth::GeoTransform>                                 _airspaceNode;

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

    // 坐标轴显示
    auto axesPath = binPath / "osgEarth/axes.osgt";
    osg::ref_ptr<osg::Node> spAxes   = osgDB::readNodeFile(axesPath.generic_string());
    osg::ref_ptr<HUDAxis> hudAxes = new HUDAxis;
    _root->addChild(hudAxes);
    hudAxes->addChild(spAxes);
    osg::ref_ptr<osg::Camera> spCamera = _viewer->getCamera();
    hudAxes->setMainCamera(spCamera);
    hudAxes->setRenderOrder(osg::Camera::POST_RENDER);   // 坐标轴最后渲染，即在所有其它三维场景前面
    hudAxes->setClearMask(GL_DEPTH_BUFFER_BIT);                // 关闭深度缓冲
    hudAxes->setAllowEventFocus(false);                        // 禁止鼠标、键盘事件
    hudAxes->setReferenceFrame(osg::Transform::ABSOLUTE_RF);   // 绝对参考帧

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
    _eventHandler = new EventHandler(_root, _mapNode);
    _viewer->addEventHandler(_eventHandler);
    _layerBoudingBox = std::make_shared<osg::BoundingBox>();
}
void View(const osgEarth::Viewpoint& viewpoint, int delta) {
    _cameraManipulator->setViewpoint(
        viewpoint, 4);
}
bool ViewLoadedTile(const boost::filesystem::path& path_)
{
    if (_loadedTileGeoPoint.find(path_.generic_string()) == _loadedTileGeoPoint.end()) return false;
    osgEarth::Viewpoint vp;
    vp.focalPoint() = *_loadedTileGeoPoint[path_.generic_string()];
    vp.setHeading(std::string("0"));
    vp.setPitch(std::string("-45"));
    vp.setRange(std::string("5000"));
    View(vp, 5);
    return true;
}
void AddLayer(osg::ref_ptr<osg::Group> tiles_, osg::BoundingBox bbox_) {
    *_layerBoudingBox = bbox_;
    osg::ref_ptr<osgEarth::GeoTransform> xform = new osgEarth::GeoTransform();
    xform->setTerrain(_mapNode->getTerrain());

    for (size_t i = 0; i < tiles_->getNumChildren(); i++) {
        auto node = tiles_->getChild(i);
        node->setName("TileNode");
    }

    xform->addChild(tiles_);

    // 查询海拔高度
    osgEarth::ElevationQuery elevationQuery(_mapNode->getMap());
    double                   elevation = 0.0;
    elevationQuery.getElevation(*_layerGeoPoint, elevation);
    auto geopoint = *_layerGeoPoint;
    geopoint.z() = elevation - bbox_.zMin();
    std::cout << geopoint.z() << std::endl;
    osgEarth::Viewpoint vp;
    vp.focalPoint() = geopoint;
    xform->setPosition(geopoint);

    vp.setHeading(std::string("0"));
    vp.setPitch(std::string("-45"));
    float range = _layerBoudingBox->radius() * 6;
    vp.setRange(std::to_string(range));
    View(vp, 5);
    _currentTile = tiles_;
    _root->addChild(xform);

}
void SetupMetadata(const boost::filesystem::path& metadataPath_) {
    auto geopoint   = ParseMetaDataFile(metadataPath_);
    _layerGeoPoint  = std::make_shared<osgEarth::GeoPoint>();
    *_layerGeoPoint = geopoint;
    _loadedTileGeoPoint[metadataPath_.parent_path().generic_string()] = _layerGeoPoint;
}
void HomeLayerView() {
    if (_layerGeoPoint) {
        osgEarth::Viewpoint vp;
        osgEarth::ElevationQuery elevationQuery(_mapNode->getMap());
        double                   elevation = 0.0;
        elevationQuery.getElevation(*_layerGeoPoint, elevation);
        auto geopoint = *_layerGeoPoint;
        geopoint.z()  = elevation - _layerBoudingBox->zMin();
        vp.focalPoint() = geopoint;
        vp.setHeading(std::string("0"));
        vp.setPitch(std::string("-45"));
        float range = _layerBoudingBox->radius() * 6;
        vp.setRange(std::to_string(range));
        View(vp, 5);
    }
}
std::vector<osg::Geometry*> VisitTile()
{
    if (!_currentTile) std::vector<osg::Geometry*>();
    DrawableVistor visitor;
    _currentTile->accept(visitor);
    return visitor.m_geoms;
}
void AppendTile(osg::Geometry* geom) {
    if (!geom) return;
    uavmvs::mesh::AppendTile(geom);
}
bool IsGeneratedTileMesh() {
    return uavmvs::mesh::IsGeneratedTileMesh();
}
void GenerateAirspace() {
    auto geode = uavmvs::mesh::GenerateAirspace();
    if (!_airspaceNode) {
        _airspaceNode = new osgEarth::GeoTransform();
        _root->addChild(_airspaceNode);
        _airspaceNode->setNodeMask(0);
    }
    if (_airspaceNode->getNumChildren()) {
        _airspaceNode->removeChild(0, _airspaceNode->getNumChildren());
    }
    if (geode) {
        if (_diskPointNode) {}
        _airspaceNode->setTerrain(_mapNode->getTerrain());
        // 查询海拔高度
        osgEarth::ElevationQuery elevationQuery(_mapNode->getMap());
        double                   elevation = 0.0;
        elevationQuery.getElevation(*_layerGeoPoint, elevation);
        auto geopoint = *_layerGeoPoint;
        geopoint.z()  = elevation - _layerBoudingBox->zMin();
        osgEarth::Viewpoint vp;
        vp.focalPoint() = geopoint;
        _airspaceNode->setPosition(geopoint);
        osgEarth::Registry::shaderGenerator().run(geode);
        _airspaceNode->addChild(geode);
    }
}
void Destory() {
    /*_root.release();
    _cameraManipulator.release();*/
}
void Resize(double width_, double height_) {
    if (!_viewer) return;
    osg::ref_ptr<osg::Camera> spCamera = _viewer->getCamera();
    spCamera->setProjectionMatrixAsPerspective(30.0, width_ / height_, 1.0, 10000);
}
void DrawRange()
{
    _eventHandler->isOpen = true;
    _eventHandler->isdrawAirspace = false;
    _eventHandler->clear();
}
void DrawAirspaceRange() {
    _eventHandler->isOpen         = true;
    _eventHandler->isdrawAirspace = true;
    _eventHandler->clear();
}

void ShowDiskPoints(bool flag_) {
    if (!_diskPointNode) return;
    if (flag_) {
        _diskPointNode->setNodeMask(0xffffffff);
    }
    else {
        _diskPointNode->setNodeMask(0);
    }
}

void ShowAirspace(bool flag_) {
    if (!_airspaceNode) return;
    if (flag_) {
        _airspaceNode->setNodeMask(0xffffffff);
    }
    else {
        _airspaceNode->setNodeMask(0);
    }
}

void PossionDiskSample() {
    if (_currentTile) {
        auto points = uavmvs::mesh::PossionDisk();
        if (!_diskPointNode) {
            _diskPointNode = new osgEarth::GeoTransform();
            _root->addChild(_diskPointNode);
        }
        _diskPointNode->removeChild(0, _diskPointNode->getNumChildren());
        if (points) {
            _diskPointNode->setTerrain(_mapNode->getTerrain());
            // 查询海拔高度
            osgEarth::ElevationQuery elevationQuery(_mapNode->getMap());
            double                   elevation = 0.0;
            elevationQuery.getElevation(*_layerGeoPoint, elevation);
            auto geopoint = *_layerGeoPoint;
            geopoint.z()  = elevation - _layerBoudingBox->zMin();
            osgEarth::Viewpoint vp;
            vp.focalPoint() = geopoint;
            _diskPointNode->setPosition(geopoint);
            osgEarth::Registry::shaderGenerator().run(points);
            _diskPointNode->addChild(points);
        }
    }
}
std::vector<osg::Vec3> GetRangePolygon()
{
    if (_eventHandler) {
        return _eventHandler->m_rangeStack;
    }
    return std::vector<osg::Vec3>();
}
std::vector<osg::Vec3> GetAirspaceRange()
{
    if (_eventHandler) {
        return _eventHandler->m_airspaceRangeStack;
    }
    return std::vector<osg::Vec3>();
}
}   // namespace context
}   // namespace uavmvs
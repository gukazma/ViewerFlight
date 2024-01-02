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
#include <memory>
#include <unordered_map>
#include <tbb/tbb.h>
#include "Utils/CLabelControlEventHandler.h"
#include "Utils/Parse.h"
#include "Utils/HUDAxis.h"
#include "Utils/EventHandler.h"

osgViewer::Viewer* _viewer;
osg::ref_ptr<osg::Group>                       _root;
osg::ref_ptr<osgEarth::Util::EarthManipulator> _cameraManipulator;
osg::ref_ptr<osgEarth::MapNode>                _mapNode;
osg::ref_ptr<EventHandler>                     _eventHandler;
std::shared_ptr<osgEarth::GeoPoint>                                  _layerGeoPoint;
std::unordered_map<std::string, std::shared_ptr<osgEarth::GeoPoint>>
    _loadedTileGeoPoint;
std::shared_ptr<osg::BoundingBox>                                    _layerBoudingBox;
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

    // ��������ʾ
    auto axesPath = binPath / "osgEarth/axes.osgt";
    osg::ref_ptr<osg::Node> spAxes   = osgDB::readNodeFile(axesPath.generic_string());
    osg::ref_ptr<HUDAxis> hudAxes = new HUDAxis;
    _root->addChild(hudAxes);
    hudAxes->addChild(spAxes);
    osg::ref_ptr<osg::Camera> spCamera = _viewer->getCamera();
    hudAxes->setMainCamera(spCamera);
    hudAxes->setRenderOrder(osg::Camera::POST_RENDER);   // �����������Ⱦ����������������ά����ǰ��
    hudAxes->setClearMask(GL_DEPTH_BUFFER_BIT);                // �ر���Ȼ���
    hudAxes->setAllowEventFocus(false);                        // ��ֹ��ꡢ�����¼�
    hudAxes->setReferenceFrame(osg::Transform::ABSOLUTE_RF);   // ���Բο�֡

    osgEarth::SkyOptions skyOptions;   // ��ջ���ѡ��
    skyOptions.ambient() = 0.1;        // ��������ˮƽ(0~1)
    auto skyNode =
        osgEarth::SkyNode::create(skyOptions);   // ���������ṩ��ա���������������Ч������
    skyNode->setDateTime(
        osgEarth::DateTime(2021, 4, 15, 0));   // ���û���������/ʱ�䡣(�������Σ�ʱ��8Сʱ)
    skyNode->setEphemeris(new osgEarth::Util::Ephemeris);   // ���ڸ�������/ʱ�䶨λ̫��������������
    skyNode->setLighting(true);                             // ����Ƿ�������������ͼ
    skyNode->addChild(_mapNode);                            // ��ӵ�ͼ�ڵ�
    skyNode->attach(_viewer);   // ������սڵ㸽�ŵ���ͼ��������⣩
    _root->addChild(skyNode);
    _viewer->realize();
    // m_root->addChild(node);
    _viewer->setSceneData(_root);
    // ���� DatabasePager
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

    // ��ѯ���θ߶�
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
        vp.focalPoint() = *_layerGeoPoint;
        vp.setHeading(std::string("0"));
        vp.setPitch(std::string("-45"));
        float range = _layerBoudingBox->radius() * 6;
        vp.setRange(std::to_string(range));
        View(vp, 5);
    }
}
void Destory() {
    /*_root.release();
    _cameraManipulator.release();*/
}
void DrawWaypoint() {
    _eventHandler->isOpen = true;
}
}   // namespace context
}   // namespace uavmvs
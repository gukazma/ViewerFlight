#pragma once
#include <osgViewer/Viewer>
#include <osgEarth/Viewpoint>
#include <QString>
#include <functional>
#include <boost/filesystem/path.hpp>
namespace uavmvs
{
namespace context
{
void Init();
void Attach(osgViewer::Viewer* viewer_);
void View(const osgEarth::Viewpoint& viewpoint, int delta);
bool ViewLoadedTile(const boost::filesystem::path& path_);
void AddLayer(osg::ref_ptr<osg::Group> tiles_, osg::BoundingBox bbox_);
void SetupMetadata(
    const boost::filesystem::path& matadataPath_);
void HomeLayerView();
void GenerateAirspace();
std::vector<osg::Geometry*> VisitTile();
void AppendTile(osg::Geometry* geom);
bool IsGeneratedTileMesh();
void Destory();
void Resize(double width_, double height_);
void DrawRange();
void DrawAirspaceRange();
void PossionDiskSample();
std::vector<osg::Vec3>      GetRangePolygon();
std::vector<osg::Vec3>      GetAirspaceRange();
}
}
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
void Destory();
}
}
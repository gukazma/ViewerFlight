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
void AddLayer(const QString& dir_, std::function<void(int)> callback_ = std::function<void(int)>());
void SetupMetadata(const boost::filesystem::path& matadataPath_);
void HomeLayerView();
void Destory();
}
}
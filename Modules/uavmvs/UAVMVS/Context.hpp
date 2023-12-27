#pragma once
#include <osgViewer/Viewer>
#include <osgEarth/Viewpoint>
namespace uavmvs
{
namespace context
{
void Init();
void Attach(osg::ref_ptr<osgViewer::Viewer> viewer_);
void View(const osgEarth::Viewpoint& viewpoint, int delta);
void Destory();
}
}
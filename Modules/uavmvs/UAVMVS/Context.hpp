#pragma once
#include <osgViewer/Viewer>
namespace uavmvs
{
namespace context
{
void Init();
void Attach(osg::ref_ptr<osgViewer::Viewer> viewer_);
void Destory();
}
}
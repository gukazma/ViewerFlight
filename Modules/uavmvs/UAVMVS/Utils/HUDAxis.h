#pragma once
#include <osg/Camera>
class HUDAxis : public osg::Camera
{
public:
    HUDAxis();
    HUDAxis(const HUDAxis& copy, osg::CopyOp copyOp = osg::CopyOp::SHALLOW_COPY);
    META_Node(osg, HUDAxis);
    inline void  setMainCamera(Camera* camera) { _mainCamera = camera; }
    virtual void traverse(osg::NodeVisitor& nv);

protected:
    virtual ~HUDAxis();
    osg::observer_ptr<Camera> _mainCamera;
};

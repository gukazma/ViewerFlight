#include "OSGViewerWidget.h"
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
#include <UAVMVS/Context.hpp>

OSGViewerWidget::OSGViewerWidget(QWidget* parent) {
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    connect(this, &osgQOpenGLWidget::initialized, this, &OSGViewerWidget::init);
}

OSGViewerWidget::~OSGViewerWidget() {
    uavmvs::context::Destory();
}

void OSGViewerWidget::home() {
    
}

void OSGViewerWidget::init()
{
    uavmvs::context::Attach(getOsgViewer());
}

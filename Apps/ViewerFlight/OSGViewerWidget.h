#ifndef OSGVIEWERWIDGET_H
#define OSGVIEWERWIDGET_H
#include "osgQOpenGL/osgQOpenGLWidget"
#include <QPoint>
#include <osg/ref_ptr>
#include <string>
namespace osg
{
class Group;
}

namespace osgEarth
{
namespace Util
{
class EarthManipulator;
}
}

class OSGViewerWidget : public osgQOpenGLWidget
{
    Q_OBJECT

public:
    explicit OSGViewerWidget(QWidget* parent = nullptr);
    ~OSGViewerWidget();

public slots:
    void init();

private:
    osg::ref_ptr<osg::Group> m_root;
    osg::ref_ptr<osgEarth::Util::EarthManipulator> m_cameraManipulator;
};


#endif   // OSGVIEWERWIDGET_H

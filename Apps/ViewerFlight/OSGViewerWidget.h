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
    
    void home();
public slots:
    void init();

private:

};


#endif   // OSGVIEWERWIDGET_H

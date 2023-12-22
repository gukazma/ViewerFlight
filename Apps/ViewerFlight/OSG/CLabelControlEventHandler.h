#pragma once

#include <osgEarth/Controls>
#include <osgEarth/Formatter>
#include <osgEarth/GeoData>
#include <osgEarth/MapNode>
#include <osgGA/GUIEventHandler>
#include <osgViewer/Viewer>
class CLabelControlEventHandler : public osgGA::GUIEventHandler
{
public:
    struct Callback : public osg::Referenced
    {
        virtual void set(const osgEarth::GeoPoint& coords, osg::View* view,
                         osgEarth::MapNode* mapNode) = 0;

        virtual void reset(osg::View* view, osgEarth::MapNode* mapNode) = 0;

        virtual ~Callback() {}
    };

public:
    CLabelControlEventHandler(osgEarth::MapNode*                      mapNode,
                              osgEarth::Util::Controls::LabelControl* label     = 0L,
                              osgEarth::Formatter*                    formatter = 0L);

    virtual ~CLabelControlEventHandler() {}

    void addCallback(Callback* callback);

    /////
    void               setViewer(osgViewer::Viewer* v) { viewer = v; }
    osgViewer::Viewer* viewer = nullptr;


public:
    bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa);

protected:
    osgEarth::MapNode*                          _mapNode;
    osg::NodePath                               _mapNodePath;
    typedef std::vector<osg::ref_ptr<Callback>> Callbacks;
    Callbacks                                   _callbacks;
};


class MouseCoordsLabelCallback : public CLabelControlEventHandler::Callback
{
public:
    MouseCoordsLabelCallback(osgEarth::Util::Controls::LabelControl* label,
                             osgEarth::Formatter*                    formatter = 0L);

    virtual ~MouseCoordsLabelCallback() {}

    virtual void set(const osgEarth::GeoPoint& coords, osg::View* view, osgEarth::MapNode* mapNode);
    virtual void reset(osg::View* view, osgEarth::MapNode* mapNode);

protected:
    osg::observer_ptr<osgEarth::Util::Controls::LabelControl> _label;
    osg::ref_ptr<osgEarth::Formatter>                         _formatter;
};
#pragma once
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/Group>
#include <osgGA/GUIEventHandler>
#include <osgEarth/MapNode>
#include <osgEarth/LineDrawable>
class EventHandler : public osgGA::GUIEventHandler
{
public:
    EventHandler(osg::ref_ptr<osg::Group> root_, osg::ref_ptr<osgEarth::MapNode> mapNode_);
    virtual bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa);
    bool         isOpen = false;

private:
    osg::ref_ptr<osg::Group>      m_root;
    osg::ref_ptr<osgEarth::MapNode> m_mapNode;
    osgEarth::LineDrawable*         linedrawable;
};
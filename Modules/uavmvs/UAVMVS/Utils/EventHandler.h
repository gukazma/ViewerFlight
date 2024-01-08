#pragma once
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/Group>
#include <osgGA/GUIEventHandler>
#include <osgEarth/MapNode>
#include <osgEarth/LineDrawable>
#include <vector>
class EventHandler : public osgGA::GUIEventHandler
{
public:
    EventHandler(osg::ref_ptr<osg::Group> root_, osg::ref_ptr<osgEarth::MapNode> mapNode_);
    virtual bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa);
    void clear();
    bool isOpen = false;
    bool isdrawAirspace = false;
    std::vector<osg::Vec3> m_rangeStack;
    std::vector<osg::Vec3> m_airspaceRangeStack;
    osg::ref_ptr<osgEarth::LineDrawable> m_linedrawable;
    osg::ref_ptr<osgEarth::LineDrawable> m_airspaceLinedrawable;

private:
    osg::ref_ptr<osg::Group>      m_root;
    osg::ref_ptr<osgEarth::MapNode> m_mapNode;
};
#include "EventHandler.h"
#include <QString>
#include <fstream>
#include <iostream>
#include <osg/BlendFunc>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/Material>
#include <osg/PolygonOffset>
#include <osg/ShapeDrawable>
#include <osg/StateSet>
#include <osgDB/WriteFile>
#include <osgUtil/IntersectionVisitor>
#include <osgUtil/LineSegmentIntersector>
#include <osgViewer/Viewer>
#include <osg/LineWidth>
#include <osgEarth/LineDrawable>
EventHandler::EventHandler(osg::ref_ptr<osg::Group>        root_,
                           osg::ref_ptr<osgEarth::MapNode> mapNode_)
    : m_root(root_)
    , m_mapNode(mapNode_)
{
    linedrawable = new osgEarth::LineDrawable(GL_LINE_STRIP);
    m_mapNode->addChild(linedrawable);
    linedrawable->setColor(osgEarth::Color::Yellow);
    linedrawable->setLineWidth(5.0);
    linedrawable->setStipplePattern(0xF0F0);
    linedrawable->setStippleFactor(1);
    linedrawable->getOrCreateStateSet()->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);
}

bool EventHandler::handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa)
{
    if (ea.getEventType() == osgGA::GUIEventAdapter::RELEASE &&
        ea.getButton() == osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON && isOpen) {
        osgViewer::Viewer* viewer = dynamic_cast<osgViewer::Viewer*>(&aa);
        if (viewer) {
            osg::ref_ptr<osgUtil::LineSegmentIntersector> intersector =
                new osgUtil::LineSegmentIntersector(
                    osgUtil::Intersector::WINDOW, ea.getX(), ea.getY());

            osg::ref_ptr<osgUtil::IntersectionVisitor> visitor =
                new osgUtil::IntersectionVisitor(intersector);

            viewer->getCamera()->accept(*visitor);

            if (intersector->containsIntersections()) {
                const osgUtil::LineSegmentIntersector::Intersection& intersection =
                    intersector->getFirstIntersection();
                osg::Vec3 worldIntersectPoint    = intersection.getWorldIntersectPoint();
                osg::Vec3 worldIntersectNormal   = intersection.getWorldIntersectNormal();
                osg::Vec3 loclIntersectionPoint = intersection.getLocalIntersectPoint();
                osg::Vec3 localIntersectNormal   = intersection.getLocalIntersectNormal();
                
                linedrawable->pushVertex(worldIntersectPoint);
                linedrawable->dirty();
                linedrawable->dirtyGLObjects();
            }
        }

        return true;
    }
    return false;
}
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
#include <osgEarthDrivers/engine_rex/SurfaceNode>
EventHandler::EventHandler(osg::ref_ptr<osg::Group>        root_,
                           osg::ref_ptr<osgEarth::MapNode> mapNode_)
    : m_root(root_)
    , m_mapNode(mapNode_)
{
    m_linedrawable = new osgEarth::LineDrawable(GL_LINE_STRIP);
    m_mapNode->addChild(m_linedrawable);
    m_linedrawable->setColor(osgEarth::Color::Yellow);
    m_linedrawable->setLineWidth(5.0);
    m_linedrawable->setStipplePattern(0xF0F0);
    m_linedrawable->setStippleFactor(1);
    m_linedrawable->getOrCreateStateSet()->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);
    m_linedrawable->getOrCreateStateSet()->setRenderBinDetails(0, "RenderBin");

    m_airspaceLinedrawable = new osgEarth::LineDrawable(GL_LINE_STRIP);
    m_mapNode->addChild(m_airspaceLinedrawable);
    m_airspaceLinedrawable->setColor(osgEarth::Color::Cyan);
    m_airspaceLinedrawable->setLineWidth(5.0);
    m_airspaceLinedrawable->setStipplePattern(0xF0F0);
    m_airspaceLinedrawable->setStippleFactor(1);
    m_airspaceLinedrawable->getOrCreateStateSet()->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);
    m_airspaceLinedrawable->getOrCreateStateSet()->setRenderBinDetails(0, "RenderBin");
}

bool EventHandler::handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa)
{
    auto lineDrawable = isdrawAirspace ? m_airspaceLinedrawable : m_linedrawable;
    auto& rangeStack   = isdrawAirspace ? m_airspaceRangeStack : m_rangeStack;

    if (ea.getEventType() == osgGA::GUIEventAdapter::DOUBLECLICK &&
        ea.getButton() == osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON && isOpen) {
        osgViewer::Viewer* viewer = dynamic_cast<osgViewer::Viewer*>(&aa);
        if (viewer) {
            osg::ref_ptr<osgUtil::LineSegmentIntersector> intersector =
                new osgUtil::LineSegmentIntersector(
                    osgUtil::Intersector::WINDOW, ea.getX(), ea.getY());
            if (lineDrawable->size() == 0) {
                return false;
            }
            if (lineDrawable->size() != 1 &&
                lineDrawable->getCount() < lineDrawable->size()) {
                lineDrawable->setVertex(lineDrawable->getCount(), lineDrawable->getVertex(0));
                lineDrawable->setCount(lineDrawable->getCount() + 1);
            }
            else {
                lineDrawable->pushVertex(lineDrawable->getVertex(0));
                lineDrawable->setCount(lineDrawable->size());
            }
            rangeStack.push_back(rangeStack[0]);
            lineDrawable->dirty();
            lineDrawable->dirtyGLObjects();
            isOpen = false;
        }

        return true;
    }

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
            //if (dynamic_cast<osgEarth::REX::SurfaceNode>(intersector->getFirstIntersection())) {}
            
            osg::NodePath nodePath = intersector->getFirstIntersection().nodePath;
            if (!nodePath.empty()) {
                osg::Node* firstNode = nodePath.back();
                std::string className = firstNode->className();
                std::cout << "class name: " << className << std::endl;
                if (className == "TileDrawable") return false;
            }
            if (intersector->containsIntersections()) {
                const osgUtil::LineSegmentIntersector::Intersection& intersection =
                    intersector->getFirstIntersection();
                osg::Vec3 worldIntersectPoint    = intersection.getWorldIntersectPoint();
                osg::Vec3 worldIntersectNormal   = intersection.getWorldIntersectNormal();
                osg::Vec3 loclIntersectionPoint = intersection.getLocalIntersectPoint();
                osg::Vec3 localIntersectNormal   = intersection.getLocalIntersectNormal();
                if (lineDrawable->size() != 1 &&
                    lineDrawable->getCount() < lineDrawable->size()) {
                    lineDrawable->setVertex(lineDrawable->getCount(), worldIntersectPoint);
                    lineDrawable->setCount(lineDrawable->getCount() + 1);
                }
                else {
                    lineDrawable->pushVertex(worldIntersectPoint);
                    lineDrawable->setCount(lineDrawable->size());
                }
                rangeStack.push_back(loclIntersectionPoint);
                lineDrawable->dirty();
                lineDrawable->dirtyGLObjects();
            }
        }

        return true;
    }

    if (ea.getEventType() == osgGA::GUIEventAdapter::RELEASE &&
        ea.getButton() == osgGA::GUIEventAdapter::RIGHT_MOUSE_BUTTON) {
        osgViewer::Viewer* viewer = dynamic_cast<osgViewer::Viewer*>(&aa);
        if (viewer) {
            osg::ref_ptr<osgUtil::LineSegmentIntersector> intersector =
                new osgUtil::LineSegmentIntersector(
                    osgUtil::Intersector::WINDOW, ea.getX(), ea.getY());
            int count = lineDrawable->getCount() - 1 < 0 ? 0 : lineDrawable->getCount() - 1;
            if (!isOpen) {
                isOpen = true;
            }
            if (count <= 2) {
                lineDrawable->setCount(0);
                lineDrawable->clear();
                rangeStack.clear();
                isOpen = false;
            }
            else {
                lineDrawable->setCount(count);
                rangeStack.pop_back();
            }
            lineDrawable->dirty();
            lineDrawable->dirtyGLObjects();
        }

        return true;
    }


    if (ea.getEventType() == osgGA::GUIEventAdapter::MOVE && isOpen) {
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
                osg::Vec3 worldIntersectPoint   = intersection.getWorldIntersectPoint();
                osg::Vec3 worldIntersectNormal  = intersection.getWorldIntersectNormal();
                osg::Vec3 loclIntersectionPoint = intersection.getLocalIntersectPoint();
                osg::Vec3 localIntersectNormal  = intersection.getLocalIntersectNormal();
                if (lineDrawable->size() == 0 || lineDrawable->getCount()==0) return false;
                if (lineDrawable->getCount() == 1)
                {
                    lineDrawable->pushVertex(worldIntersectPoint);
                    lineDrawable->setCount(2);
                    lineDrawable->dirty();
                    lineDrawable->dirtyGLObjects();
                    return true;
                }
                lineDrawable->setVertex(lineDrawable->getCount() - 1, worldIntersectPoint);
                lineDrawable->dirty();
                lineDrawable->dirtyGLObjects();
            }
        }

        return true;
    }

    return false;
}

void EventHandler::clear() {
    auto lineDrawable = isdrawAirspace ? m_airspaceLinedrawable : m_linedrawable;
    auto& rangeStack   = isdrawAirspace ? m_airspaceRangeStack : m_rangeStack;
    rangeStack.clear();
    lineDrawable->clear();
    lineDrawable->dirty();
    lineDrawable->dirtyGLObjects();
    //m_mapNode->removeChild(m_linedrawable);
}

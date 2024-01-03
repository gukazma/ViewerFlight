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
}

bool EventHandler::handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa)
{
    if (ea.getEventType() == osgGA::GUIEventAdapter::DOUBLECLICK &&
        ea.getButton() == osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON && isOpen) {
        osgViewer::Viewer* viewer = dynamic_cast<osgViewer::Viewer*>(&aa);
        if (viewer) {
            osg::ref_ptr<osgUtil::LineSegmentIntersector> intersector =
                new osgUtil::LineSegmentIntersector(
                    osgUtil::Intersector::WINDOW, ea.getX(), ea.getY());
            if (m_linedrawable->size() == 0) {
                return false;
            }
            if (m_linedrawable->size() != 1 &&
                m_linedrawable->getCount() < m_linedrawable->size()) {
                m_linedrawable->setVertex(m_linedrawable->getCount(), m_linedrawable->getVertex(0));
                m_linedrawable->setCount(m_linedrawable->getCount() + 1);
            }
            else {
                m_linedrawable->pushVertex(m_linedrawable->getVertex(0));
                m_linedrawable->setCount(m_linedrawable->size());
            }
            m_linedrawable->dirty();
            m_linedrawable->dirtyGLObjects();
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
                if (m_linedrawable->size() != 1 &&
                    m_linedrawable->getCount() < m_linedrawable->size()) {
                    m_linedrawable->setVertex(m_linedrawable->getCount(), worldIntersectPoint);
                    m_linedrawable->setCount(m_linedrawable->getCount() + 1);
                }
                else {
                    m_linedrawable->pushVertex(worldIntersectPoint);
                    m_linedrawable->setCount(m_linedrawable->size());
                }
                m_linedrawable->dirty();
                m_linedrawable->dirtyGLObjects();
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
            int count = m_linedrawable->getCount() - 1 < 0 ? 0 : m_linedrawable->getCount() - 1;
            if (count <= 2) {
                m_linedrawable->setCount(1);
                m_linedrawable->clear();
                isOpen = false;
            }
            else {
                m_linedrawable->setCount(count);
            }
            m_linedrawable->dirty();
            m_linedrawable->dirtyGLObjects();
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
                if (m_linedrawable->size() == 0 || m_linedrawable->getCount()==0) return false;
                if (m_linedrawable->getCount() == 1)
                {
                    m_linedrawable->pushVertex(worldIntersectPoint);
                    m_linedrawable->setCount(2);
                    m_linedrawable->dirty();
                    m_linedrawable->dirtyGLObjects();
                    return true;
                }
                m_linedrawable->setVertex(m_linedrawable->getCount() - 1, worldIntersectPoint);
                m_linedrawable->dirty();
                m_linedrawable->dirtyGLObjects();
            }
        }

        return true;
    }


    

    
    return false;
}

void EventHandler::clear() {
    m_linedrawable->clear();
    m_mapNode->removeChild(m_linedrawable);
}

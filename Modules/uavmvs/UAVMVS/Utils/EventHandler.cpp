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
#include "Line.h"
EventHandler::EventHandler(osg::ref_ptr<osg::Group>        root_,
                           osg::ref_ptr<osgEarth::MapNode> mapNode_)
    : m_root(root_)
    , m_mapNode(mapNode_)
{}

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
                /*osg::ref_ptr<Line> line                   = new Line();
                line->push(worldIntersectPoint, {0.0f, 1.0f, 0.0f, 1.f});
                line->push(worldIntersectPoint + worldIntersectNormal * 10,
                           {0.0f, 1.0f, 0.0f, 1.f});
                line->update();
                osgEarth::LineGroup* lineGroup = new osgEarth::LineGroup;
                lineGroup->import(line);
                m_mapNode->addChild(lineGroup);*/
                osgEarth::LineDrawable* linedrawable =
                    new osgEarth::LineDrawable(GL_LINE_STRIP);
                linedrawable->setLineWidth(8);
                linedrawable->setColor(osgEarth::Color::Yellow);
                osg::ref_ptr<osg::Group>             group        = new osg::Group;
                linedrawable->pushVertex(worldIntersectPoint);
                linedrawable->pushVertex(worldIntersectPoint + worldIntersectNormal * 10);
                linedrawable->setLineWidth(5.0);
                linedrawable->setStipplePattern(0xF0F0);
                linedrawable->setStippleFactor(1);
                linedrawable->dirty();
                linedrawable->dirtyGLObjects();
                group->addChild(linedrawable);
                m_mapNode->addChild(group);
                /*auto geode = new osg::Geode;
                auto wire  = new osg::Geometry;
                geode->addDrawable(wire);

                wire->setUseDisplayList(false);
                wire->setName("Wire");

                osg::Vec3Array* verts = new osg::Vec3Array;
                verts->push_back(worldIntersectPoint);
                verts->push_back(worldIntersectPoint + worldIntersectNormal*10);
                verts->dirty();

                osg::Vec4Array* clrWire = new osg::Vec4Array(2);
                (*clrWire)[0]           = osg::Vec4f(0.0f, 1.0f, 0.0f, 1.f);
                (*clrWire)[1]           = osg::Vec4f(0.0f, 1.0f, 0.0f, 1.f);
                wire->addPrimitiveSet(new osg::DrawArrays(GL_LINES, 0, verts->size()));
                wire->setVertexArray(verts);
                wire->setColorArray(clrWire, osg::Array::BIND_PER_VERTEX);

                wire->getOrCreateStateSet()->setAttribute(new osg::LineWidth(3.5f),
                                                            osg::StateAttribute::ON);
                m_mapNode->addChild(geode);*/

            }
        }

        return true;
    }
    return false;
}
#include "Line.h"
#include <osg/LineWidth>
#include <osg/LineStipple>
Line::Line()
    : osg::Geode()
{
    m_vertices = new osg::Vec3Array;
    m_color    = new osg::Vec4Array;
    m_geometry = new osg::Geometry;
    this->addDrawable(m_geometry);
    m_geometry->setUseDisplayList(false);
    m_geometry->setName("Wire");
    m_drawArrays = new osg::DrawArrays(GL_LINES, 0, 0);
    m_geometry->addPrimitiveSet(m_drawArrays);
    m_geometry->setVertexArray(m_vertices);
    m_geometry->setColorArray(m_color, osg::Array::BIND_PER_VERTEX);
    m_geometry->getOrCreateStateSet()->setAttribute(new osg::LineWidth(3.5f),
                                                    osg::StateAttribute::ON);
    m_geometry->getOrCreateStateSet()->setAttributeAndModes(
        new osg::LineStipple(1, 0xFF00), osg::StateAttribute::OVERRIDE | osg::StateAttribute::ON);
}

void Line::push(const osg::Vec3& v_, const osg::Vec4 color_) {
    m_vertices->push_back(v_);
    m_color->push_back(color_);
}

void Line::pop() {}

void Line::update()
{
    m_drawArrays->setCount(m_vertices->size());
    m_vertices->dirty();
    m_color->dirty();
    m_drawArrays->dirty();
    m_geometry->dirtyGLObjects();
}


Line::~Line() {
	
}

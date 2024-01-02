#pragma once

#include <osg/Geode>
#include <osg/Geometry>
class Line : public osg::Geode
{
public:
    Line();
    void push(const osg::Vec3& v_, const osg::Vec4 color_);
    void pop();
    void update();
    ~Line();

    osg::ref_ptr<osg::Vec3Array> m_vertices;
    osg::ref_ptr<osg::Geometry> m_geometry;
    osg::ref_ptr<osg::Vec4Array> m_color;
    osg::ref_ptr<osg::DrawArrays> m_drawArrays;
};
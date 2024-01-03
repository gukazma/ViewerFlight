#include "DrawableVisitor.h"
#include <osgDB/WriteFile>
#include <boost/filesystem.hpp>
DrawableVistor::DrawableVistor()
    : osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ACTIVE_CHILDREN)
{}

DrawableVistor::~DrawableVistor() {}

void DrawableVistor::apply(osg::Geometry& geometry_) {
    m_geoms.push_back(&geometry_);
    /*static int num = 0;
    boost::filesystem::path outputFile = "D:/output" + std::to_string(num);
    num++;
    outputFile.replace_extension(".obj");
    osgDB::writeNodeFile(geometry_, outputFile.generic_string());*/
}

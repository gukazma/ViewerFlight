#pragma once
#include <osg/ref_ptr>
#include <boost/filesystem/path.hpp>
#include <osgEarth/GeoData>
namespace osg
{
class Node;
}
osg::ref_ptr<osg::Node> ParseOSGBFile(const boost::filesystem::path& path_);
osg::ref_ptr<osg::Node> ParseOSGBDir(const boost::filesystem::path& path_);

osgEarth::GeoPoint ParseMetaDataFile(const boost::filesystem::path& path_);




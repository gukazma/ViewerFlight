#include "Parse.h"
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <osgDB/ReadFile>
#include <osgEarth/GeoTransform>
#include <QRegExp>
#include <QString>
#include <QStringList>
namespace fs = boost::filesystem;
osg::ref_ptr<osg::Node> ParseOSGBFile(const boost::filesystem::path& path_)
{
    return osg::ref_ptr<osg::Node>();
}

osg::ref_ptr<osg::Node> ParseOSGBDir(const boost::filesystem::path& path_)
{
    osg::ref_ptr<osgEarth::GeoTransform> xform = new osgEarth::GeoTransform();

    if (fs::exists(path_) && fs::is_directory(path_)) {
        fs::directory_iterator endIter;
        for (fs::directory_iterator iter(path_); iter != endIter; ++iter) {
            fs::path itPath = iter->path();
            if (fs::is_directory(iter->status())) {
                itPath = itPath / itPath.filename();
                itPath.replace_extension("osgb");
                if (fs::exists(itPath)) {
                    xform->addChild(osgDB::readNodeFile(itPath.generic_path().generic_string()));
                }
            }
        }
    }
    else {
        return nullptr;
    }
    auto metaDataPath = path_ / "metadata.xml";
    auto point        = ParseMetaDataFile(metaDataPath);
    xform->setPosition(point);
    return xform;
}

osgEarth::GeoPoint ParseMetaDataFile(const boost::filesystem::path& path_)
{
    double      x, y, z;
    std::string srs;
    boost::property_tree::ptree pt;
    boost::property_tree::read_xml(path_.generic_string(), pt);

    srs                                = pt.get<std::string>("ModelMetadata.SRS");
    if (boost::algorithm::contains(srs, "EPSG"))
    {
        std::string              srsOrigin = pt.get<std::string>("ModelMetadata.SRSOrigin");
        std::vector<std::string> tokens;
        boost::split(tokens, srsOrigin, boost::is_any_of(","));

        if (tokens.size() == 3) {
            x = std::stod(tokens[0]);
            y = std::stod(tokens[1]);
            z = std::stod(tokens[2]);
        }
        else {
            std::cout << "Invalid SRSOrigin format." << std::endl;
        }
        auto point = osgEarth::GeoPoint(
            osgEarth::SpatialReference::create(srs), x, y, z, osgEarth::ALTMODE_ABSOLUTE);
        std::cout << point.x() << ", " << point.y() << ", " << point.z() << std::endl;
        return point;
    }
    else {
        QRegExp rx("ENU:([0-9\\.]+), ([0-9\\.]+)");   // 正则表达式用于匹配数字
        int     pos = rx.indexIn(srs.c_str());
        if (pos > -1) {
            QString lat = rx.cap(1);   // 第一个数字
            QString lon = rx.cap(2);   // 第二个数字
            std::cout << lat.toFloat() << " " << lon.toFloat() << std::endl;
            auto point = osgEarth::GeoPoint(osgEarth::SpatialReference::get("wgs84"),
                                            lon.toFloat(),
                                            lat.toFloat(),
                                            0,
                                            osgEarth::ALTMODE_ABSOLUTE);
            return point;
            // do something with lat and lon
        }
    }
    
}

#include <osg/Array>
#include <osg/Geometry>
#include <vector>
#include <boost/filesystem/path.hpp>
namespace uavmvs
{
namespace mesh
{
void AppendTile(osg::Geometry* geom);
void SaveTile(const boost::filesystem::path& path_);
bool IsGeneratedTileMesh();
osg::ref_ptr<osg::Geode> PossionDisk();
osg::ref_ptr<osg::Geode> GenerateAirspace();
void                     FilterAirspaceRange();
}

}
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
bool isIntersectionAirspace(osg::Vec3 begin_, osg::Vec3 end_);
osg::ref_ptr<osg::Geode> PossionDisk();
osg::ref_ptr<osg::Geode> GetWayPoints();
osg::ref_ptr<osg::Geode> GenerateAirspace();
std::vector<osg::Vec3>   GetDiskPoints();
std::vector<osg::Vec3>   GetDiskPointsNormals();
void                     FilterAirspaceRange();
}

}
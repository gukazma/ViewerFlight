#define NOMINMAX
#include "Mesh.h"
#include <vcg/complex/algorithms/local_optimization.h>
#include <vcg/complex/algorithms/local_optimization/tri_edge_collapse_quadric.h>
#include <vcg/complex/algorithms/update/color.h>
#include <vcg/complex/complex.h>
#include<vcg/complex/algorithms/point_sampling.h>
#include<vcg/complex/algorithms/voronoi_processing.h>
#include <vcg/space/index/kdtree/kdtree.h>
#include<vcg/complex/algorithms/update/normal.h>
#include <wrap/io_trimesh/export_off.h>
#include <wrap/io_trimesh/export_ply.h>
#include <wrap/io_trimesh/import.h>
#include <osg/Texture2D>
#include <osg/Geode>
#include <osg/Point>
#include <osgEarth/PointDrawable>
#include <osg/Shape>
#include <osg/ShapeDrawable>
#include <osg/BlendColor>
#include <osg/BlendFunc>
#include <osgUtil/SmoothingVisitor>
#include <boost/geometry.hpp>
#include <mutex>
#include "UAVMVS/Context.hpp"
#include "UAVMVS/Settings.h"
#include <unordered_map>
class MyVertex;
class MyEdge;
class MyFace;

typedef boost::geometry::model::d2::point_xy<double> point_type;
typedef boost::geometry::model::polygon<point_type>  polygon_type;

struct MyUsedTypes
    : public vcg::UsedTypes<vcg::Use<MyVertex>::AsVertexType, vcg::Use<MyEdge>::AsEdgeType,
                            vcg::Use<MyFace>::AsFaceType>
{};

class MyVertex
    : public vcg::Vertex<MyUsedTypes, vcg::vertex::VFAdj, vcg::vertex::Coord3f,
                         vcg::vertex::Normal3f, vcg::vertex::Color4b, vcg::vertex::Mark,
                         vcg::vertex::TexCoord2f, vcg::vertex::Qualityf, vcg::vertex::BitFlags>
{
public:
    vcg::math::Quadric<double>& Qd() { return q; }

private:
    vcg::math::Quadric<double> q;
};

class MyEdge : public vcg::Edge<MyUsedTypes>
{};

typedef vcg::tri::BasicVertexPair<MyVertex> VertexPair;

class MyFace : public vcg::Face<MyUsedTypes, vcg::face::VFAdj, vcg::face::FFAdj,
                                vcg::face::Normal3f, vcg::face::VertexRef,
                                vcg::face::WedgeTexCoord2f, vcg::face::BitFlags, vcg::face::Mark>
{};

// the main mesh class
class MyMesh : public vcg::tri::TriMesh<std::vector<MyVertex>, std::vector<MyFace>>
{};


class MyTriEdgeCollapse
    : public vcg::tri::TriEdgeCollapseQuadric<MyMesh, VertexPair, MyTriEdgeCollapse,
                                              vcg::tri::QInfoStandard<MyVertex>>
{
public:
    typedef vcg::tri::TriEdgeCollapseQuadric<MyMesh, VertexPair, MyTriEdgeCollapse,
                                             vcg::tri::QInfoStandard<MyVertex>>
                                         TECQ;
    typedef MyMesh::VertexType::EdgeType EdgeType;
    inline MyTriEdgeCollapse(const VertexPair& p, int i, vcg::BaseParameterClass* pp)
        : TECQ(p, i, pp)
    {}
};

namespace std {
template<> struct hash<vcg::Point3f>
{
    size_t operator()(vcg::Point3f const& vertex) const
    {
        return ((hash<float>()(vertex[0]) ^ (hash<float>()(vertex[2]) << 1)) >> 1) ^
               (hash<float>()(vertex[1]) << 1);
    }
};
}   // namespace std
MyMesh     _tileMesh;
MyMesh     _airspaceMesh;
MyMesh                   _diskSamplePoints;
std::vector<vcg::Point3f>           _waypointsBegin;
std::vector<vcg::Point3f>           _waypointsEnd;


osg::ref_ptr<osg::Geode> _airspaceNode;
std::mutex _tileMutex;
std::pair<vcg::Point3f, vcg::Point3f> _wayPointsPair;

namespace uavmvs {
namespace mesh {

static void AddSphere(osg::ref_ptr<osg::Geode> geode, osg::Vec3 position, osg::Vec4 color,
                      float radius)
{
    osg::ref_ptr<osg::Sphere>        sphere        = new osg::Sphere(position, radius);
    osg::ref_ptr<osg::ShapeDrawable> shapeDrawable = new osg::ShapeDrawable(sphere);

    // 设置球体的颜色
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array;
    colors->push_back(color);   // 红色
    shapeDrawable->setColorArray(colors);
    shapeDrawable->setColorBinding(osg::Geometry::BIND_OVERALL);
    geode->addDrawable(shapeDrawable);
}


bool IsIntersectionAirspace(osg::Vec3 begin_, osg::Vec3 end_)
{
    if (!_airspaceNode) {
        return false;
    }
    {
        osg::Vec3 up  = {0.0, 0.0, 1.0};
        osg::Vec3 dir = end_ - begin_;
        if (dir * up < 0) return false;
        osg::ref_ptr<osgUtil::LineSegmentIntersector> _lineSegmentIntersector =
            new osgUtil::LineSegmentIntersector(begin_, end_);
        osgUtil::IntersectionVisitor _iv(_lineSegmentIntersector.get());
        _airspaceNode->accept(_iv);
        osgUtil::LineSegmentIntersector::Intersections _intersections =
            _lineSegmentIntersector->getIntersections();
        int _intersectionNumber = _intersections.size();
        if (_intersectionNumber == 1) {

            return true;
        }
    }


    {
        osg::Vec3 up  = {0.0, 0.0, 1.0};
        osg::Vec3                                     end = begin_ - up * ::uavmvs::context::GetSettings()->getDistance();
        osg::ref_ptr<osgUtil::LineSegmentIntersector> _lineSegmentIntersector =
            new osgUtil::LineSegmentIntersector(begin_, end);
        osgUtil::IntersectionVisitor _iv(_lineSegmentIntersector.get());
        _airspaceNode->accept(_iv);
        osgUtil::LineSegmentIntersector::Intersections _intersections =
            _lineSegmentIntersector->getIntersections();
        int _intersectionNumber = _intersections.size();
        if (_intersectionNumber == 1) {

            return true;
        }
    }

    return false;

}

static void OSG2Mesh(osg::Geometry* geometry_, MyMesh& mesh_)
{
    std::vector<vcg::Point3f>            coordVec;
    std::vector<vcg::Point3i>            indexVec;
    std::map<vcg::Point3f, vcg::Color4b> pcmap;
    std::vector<vcg::Color4b>            colors;

    osg::Texture2D* texture = dynamic_cast<osg::Texture2D*>(
        geometry_->getStateSet()->getTextureAttribute(0, osg::StateAttribute::TEXTURE));
    if (!texture) return;
    osg::Image*     image             = texture->getImage();
    osg::Vec2Array* textureCoordArray = dynamic_cast<osg::Vec2Array*>(geometry_->getTexCoordArray(0));
    // get vertex
    osg::Vec3Array* vertices = dynamic_cast<osg::Vec3Array*>(geometry_->getVertexArray());
    for (size_t i = 0; i < vertices->size(); i++) {
        auto         v = vertices->at(i);
        vcg::Point3f p(v.x(), v.y(), v.z());
        auto         coord = textureCoordArray->at(i);
        coordVec.push_back(p);
    }

    auto faces_uint   = dynamic_cast<osg::DrawElementsUInt*>(geometry_->getPrimitiveSet(0));
    auto faces_ushort = dynamic_cast<osg::DrawElementsUShort*>(geometry_->getPrimitiveSet(0));

    if (!faces_uint && !faces_ushort) return;
    if (!vertices) return;

    std::vector<uint32_t> faces;
    if (faces_uint) {
        faces.clear();
        for (size_t i = 0; i < faces_uint->size(); i++) {
            faces.push_back(faces_uint->getElement(i));
        }
    }
    if (faces_ushort) {
        faces.clear();
        for (size_t i = 0; i < faces_ushort->size(); i++) {
            faces.push_back(faces_ushort->getElement(i));
        }
    }

    for (size_t i = 0; i < faces.size(); i += 3) {
        vcg::Point3i index;
        index.X() = faces[i];
        index.Y() = faces[i + 1];
        index.Z() = faces[i + 2];
        indexVec.push_back(index);
    }
    auto v_iter = vcg::tri::Allocator<MyMesh>::AddVertices(mesh_, coordVec.size());
    // vertices
    for (size_t i = 0; i < coordVec.size(); i++, v_iter++) {
        (*v_iter).P()[0] = coordVec[i].X();
        (*v_iter).P()[1] = coordVec[i].Y();
        (*v_iter).P()[2] = coordVec[i].Z();
    }

    for (size_t i = 0; i < indexVec.size(); i++) {
        vcg::tri::Allocator<MyMesh>::AddFace(mesh_,
                                             &mesh_.vert[indexVec[i].X()],
                                             &mesh_.vert[indexVec[i].Y()],
                                             &mesh_.vert[indexVec[i].Z()]);
    }
}

void AppendTile(osg::Geometry* geom)
{
    MyMesh submesh;
    OSG2Mesh(geom, submesh);
    std::lock_guard lk(_tileMutex);
    vcg::tri::Append<MyMesh, MyMesh>::Mesh(_tileMesh, submesh);
}

void SaveTile(const boost::filesystem::path& path_)
{
    vcg::tri::io::ExporterPLY<MyMesh>::Save(_tileMesh, path_.generic_string().c_str());
}

bool IsGeneratedTileMesh()
{
    return _tileMesh.VertexNumber();
}

osg::ref_ptr<osg::Geode> PossionDisk()
{
    _diskSamplePoints.Clear();
    std::vector<osg::Vec3> rangePoints = uavmvs::context::GetRangePolygon();
    polygon_type           rangePolygon;
    for (size_t i = 0; i < rangePoints.size(); i++) {
        boost::geometry::append(rangePolygon, point_type(rangePoints[i].x(), rangePoints[i].y()));
    }

    vcg::tri::UpdateBounding<MyMesh>::Box(_tileMesh);
    vcg::tri::Clean<MyMesh>::RemoveUnreferencedVertex(_tileMesh);
    vcg::tri::Allocator<MyMesh>::CompactEveryVector(_tileMesh);
    vcg::tri::UpdateTopology<MyMesh>::VertexFace(_tileMesh);

    vcg::tri::SurfaceSampling<MyMesh, vcg::tri::TrivialPointerSampler<MyMesh>>::PoissonDiskParam pp;
    vcg::tri::UpdateFlags<MyMesh>::FaceBorderFromVF(_tileMesh);
    vcg::tri::UpdateFlags<MyMesh>::VertexBorderFromFaceBorder(_tileMesh);
    vcg::tri::UpdateNormal<MyMesh>::PerFace(_tileMesh);
    vcg::tri::UpdateNormal<MyMesh>::PerVertex(_tileMesh);

    vcg::tri::TrivialPointerSampler<MyMesh> mps;
    vcg::tri::SurfaceSampling<MyMesh, vcg::tri::TrivialPointerSampler<MyMesh>>::PoissonDiskPruning(
        mps, _tileMesh, uavmvs::context::GetSettings()->diskRadius, pp);
    vcg::tri::RequirePerVertexNormal<MyMesh>(_diskSamplePoints);

    vcg::tri::Allocator<MyMesh>::AddVertices(_diskSamplePoints, mps.sampleVec.size());

    for (size_t i = 0; i < mps.sampleVec.size(); ++i) {
        _diskSamplePoints.vert[i].P() = mps.sampleVec[i]->cP();
        _diskSamplePoints.vert[i].N() = mps.sampleVec[i]->cN();
    }

    vcg::tri::UpdateNormal<MyMesh>::NormalizePerVertex(_diskSamplePoints);
    vcg::tri::UpdateNormal<MyMesh>::NormalizePerVertex(_tileMesh);

    // 设置关联方式
    osg::ref_ptr<osg::Geode> geode = new osg::Geode();
    vcg::Point3f             up    = {0, 0, 1};
    for (size_t i = 0; i < _diskSamplePoints.vert.size(); i++) {

        bool isWithin = boost::geometry::within(
            point_type(_diskSamplePoints.vert[i].P()[0], _diskSamplePoints.vert[i].P()[1]),
            rangePolygon);
        if (!isWithin)
        {
            _diskSamplePoints.vert[i].SetD();
            continue;
        }

        auto normal = _diskSamplePoints.vert[i].N();

        if ((normal * up) < 0) {
            _diskSamplePoints.vert[i].SetD();
            continue;
        }
        AddSphere(geode,
                  {_diskSamplePoints.vert[i].P()[0],
                   _diskSamplePoints.vert[i].P()[1],
                   _diskSamplePoints.vert[i].P()[2]},
                  osg::Vec4(1.0f, 0.0f, 0.0f, 1.0f),
                  0.5);
    }

    return geode.release();
}

osg::ref_ptr<osg::Geode> GetWayPoints()
{
    auto                     points  = uavmvs::mesh::GetDiskPoints();
    auto                     normals = uavmvs::mesh::GetDiskPointsNormals();
    osg::Vec3                up      = {0, 0, 1.0};
    osg::ref_ptr<osg::Geode> geode   = new osg::Geode();
    double                   d       = uavmvs::context::GetSettings()->getDistance();
    //std::vector<vcg::Point3f> waypoints;
    _waypointsBegin.clear();
    _waypointsEnd.clear();
    for (size_t i = 0; i < points.size(); i++) {
        auto      normal = -normals[i];
        auto      point  = points[i] + normals[i] * uavmvs::context::GetSettings()->getDistance();
        osg::Vec3 u1     = osg::Vec3(-normal.y(), normal.x(), 0);
        u1.normalize();
        osg::Vec3 u2 = u1 ^ normal;
        u2.normalize();
        float     s  = uavmvs::context::GetSettings()->getDistance();
        osg::Vec3 p1 = point + u1 * (s / 2.0) + u2 * (s * std::sqrt(3) / 2);
        osg::Vec3 p2 = point - u1 * (s / 2.0) + u2 * (s * std::sqrt(3) / 2);
        osg::Vec3 p3 = point - u2 * (s * std::sqrt(3) / 2);

        if (::uavmvs::mesh::IsIntersectionAirspace(points[i], p1)) {
            AddSphere(geode, p1, osg::Vec4(0.0f, 1.0f, 1.0f, 1.0f), 0.5);
            _waypointsBegin.push_back({p1.x(), p1.y(), p1.z()});
            _waypointsEnd.push_back({points[i].x(), points[i].y(), points[i].z()});
        }
        if (::uavmvs::mesh::IsIntersectionAirspace(points[i], p2)) {
            AddSphere(geode, p2, osg::Vec4(0.0f, 1.0f, 1.0f, 1.0f), 0.5);
            _waypointsBegin.push_back({p2.x(), p2.y(), p2.z()});
            _waypointsEnd.push_back({points[i].x(), points[i].y(), points[i].z()});
        }
        if (::uavmvs::mesh::IsIntersectionAirspace(points[i], p3)) {
            AddSphere(geode, p3, osg::Vec4(0.0f, 1.0f, 1.0f, 1.0f), 0.5);
            _waypointsBegin.push_back({p2.x(), p2.y(), p2.z()});
            _waypointsEnd.push_back({points[i].x(), points[i].y(), points[i].z()});
        }
    }
    return geode;
}

void ReorderWayPoints() {
    if (_waypointsBegin.size() == 0) return;
    
    vcg::ConstDataWrapper<vcg::Point3f> ww(
        &(_waypointsBegin[0]), _waypointsBegin.size(), sizeof(vcg::Point3f));
    vcg::KdTree<float>                                     tree(ww);
    vcg::KdTree<float>::PriorityQueue queue;

    std::vector<vcg::Point3f> newWaypointsBegin;
    std::vector<vcg::Point3f> newWaypointsEnd;
    std::vector<int>          isVisited;
    for (size_t i = 0; i < _waypointsBegin.size(); i++) {
        isVisited.push_back(0);
    }
    vcg::Point3f currentWaypoint = _waypointsBegin[0];
    newWaypointsBegin.push_back(currentWaypoint);
    newWaypointsEnd.push_back(_waypointsEnd[0]);

    for (size_t i = 0; i < _waypointsBegin.size(); i++) {
        tree.doQueryK(currentWaypoint, 5, queue);
        int   neighbours = queue.getNofElements();
        for (int i = 0; i < neighbours; i++) {
            int neightId = queue.getIndex(i);
            if (!isVisited[neightId])
            {
                currentWaypoint = _waypointsBegin[neightId];
                newWaypointsBegin.push_back(currentWaypoint);
                newWaypointsEnd.push_back(_waypointsEnd[neightId]);
                isVisited[neightId] = 1;
                break;
            }
        }
    }

    _waypointsBegin = newWaypointsBegin;
    _waypointsEnd = newWaypointsEnd;
}

void MakeTransparent(osg::Node* node, float transparencyValue)
{
    auto geode = node->asGeode();
    if (!geode) return;

    for (size_t i = 0; i < geode->getNumDrawables(); i++) {
        auto drawable = geode->getDrawable(i);
        auto geometry = drawable->asGeometry();
        if (!geometry) continue;
        osg::StateSet* state = geometry->getOrCreateStateSet();
        // 关闭灯光
        state->setMode(GL_LIGHTING, osg::StateAttribute::OFF | osg::StateAttribute::PROTECTED);
        // 打开混合融合模式
        state->setMode(GL_BLEND, osg::StateAttribute::ON);
        state->setMode(GL_DEPTH_TEST, osg::StateAttribute::ON);
        state->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
        // 使用BlendFunc实现透明效果
        osg::BlendColor* bc = new osg::BlendColor(osg::Vec4(1.0, 1.0, 1.0, 0.0));
        osg::BlendFunc*  bf = new osg::BlendFunc();
        state->setAttributeAndModes(bf, osg::StateAttribute::ON);
        state->setAttributeAndModes(bc, osg::StateAttribute::ON);
        bf->setSource(osg::BlendFunc::CONSTANT_ALPHA);
        bf->setDestination(osg::BlendFunc::ONE_MINUS_CONSTANT_ALPHA);
        bc->setConstantColor(osg::Vec4(1, 1, 1, 0.5));
    }
}

osg::ref_ptr<osg::Geode> GenerateAirspace()
{
    vcg::tri::Append<MyMesh, MyMesh>::MeshCopyConst(_airspaceMesh, _tileMesh);
    FilterAirspaceRange();

    vcg::tri::UpdateBounding<MyMesh>::Box(_airspaceMesh);
    vcg::tri::Clean<MyMesh>::RemoveUnreferencedVertex(_airspaceMesh);
    vcg::tri::Allocator<MyMesh>::CompactEveryVector(_airspaceMesh);
    vcg::tri::UpdateTopology<MyMesh>::VertexFace(_airspaceMesh);
    vcg::tri::UpdateFlags<MyMesh>::FaceBorderFromVF(_airspaceMesh);
    vcg::tri::UpdateFlags<MyMesh>::VertexBorderFromFaceBorder(_airspaceMesh);
    vcg::tri::UpdateNormal<MyMesh>::PerFace(_airspaceMesh);
    vcg::tri::UpdateNormal<MyMesh>::PerVertex(_airspaceMesh);
    vcg::tri::UpdateNormal<MyMesh>::NormalizePerVertex(_airspaceMesh);

    osg::ref_ptr<osg::Vec3Array>        vertices = new osg::Vec3Array;
    osg::ref_ptr<osg::DrawElementsUInt> drawElements =
        new osg::DrawElementsUInt(osg::PrimitiveSet::TRIANGLES);
    osg::ref_ptr<osg::Geometry>         geometry = new osg::Geometry;

    for (size_t i = 0; i < _airspaceMesh.vert.size(); i++) {
        auto v = _airspaceMesh.vert[i];
        auto n = _airspaceMesh.vert[i].N();
        v.P() += (n * 5.0f);
        osg::Vec3 vertex = osg::Vec3(v.P().X(), v.P().Y(), v.P().Z());
        vertices->push_back(vertex);
    }

    for (int fn = 0; fn < _airspaceMesh.face.size(); fn++) {
        auto& f = _airspaceMesh.face[fn];
        if (!f.IsD()) {
            for (size_t i = 0; i < 3; i++) {
                auto                    v        = f.V(i);
                int  index = vcg::tri::Index(_airspaceMesh, v);
                drawElements->push_back(index);
            }
        }
    }

    geometry->setVertexArray(vertices);
    geometry->addPrimitiveSet(drawElements);
    osg::ref_ptr<osg::StateSet> stateset = geometry->getOrCreateStateSet();
    geometry->setUseVertexBufferObjects(true);
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array;
    colors->push_back(osg::Vec4(0.0f, 1.0f, 0.0f, 1.0f));   // 红色
    geometry->setColorArray(colors);
    geometry->setColorBinding(osg::Geometry::BIND_OVERALL);
    osgUtil::SmoothingVisitor::smooth(*geometry);
    osg::ref_ptr<osg::Geode> geode = new osg::Geode;
    geode->addDrawable(geometry);
    MakeTransparent(geode, 0.5);
    _airspaceNode = geode;
    return geode;
}
std::vector<osg::Vec3> GetDiskPoints()
{
    std::vector<osg::Vec3> rnt;
    for (size_t i = 0; i < _diskSamplePoints.VertexNumber(); i++) {
        if (!_diskSamplePoints.vert[i].IsD()) {
            rnt.push_back({_diskSamplePoints.vert[i].P().X(),
                           _diskSamplePoints.vert[i].P().Y(),
                           _diskSamplePoints.vert[i].P().Z()});
        }
    }
    return rnt;
}
std::vector<osg::Vec3> GetDiskPointsNormals()
{
    std::vector<osg::Vec3> rnt;
    for (size_t i = 0; i < _diskSamplePoints.VertexNumber(); i++) {
        if (!_diskSamplePoints.vert[i].IsD()) {
            rnt.push_back({_diskSamplePoints.vert[i].N().X(),
                           _diskSamplePoints.vert[i].N().Y(),
                           _diskSamplePoints.vert[i].N().Z()});
        }
    }
    return rnt;
}
void FilterAirspaceRange() {
    for (int fn = 0; fn < _airspaceMesh.face.size(); fn++) {
        auto& f = _airspaceMesh.face[fn];
        f.ClearD();
    }

    std::vector<osg::Vec3> rangePoints = uavmvs::context::GetAirspaceRange();
    polygon_type           rangePolygon;
    for (size_t i = 0; i < rangePoints.size(); i++) {
        boost::geometry::append(rangePolygon, point_type(rangePoints[i].x(), rangePoints[i].y()));
    }

    for (int fn = 0; fn < _airspaceMesh.face.size(); fn++) {
        auto& f = _airspaceMesh.face[fn];
        if (!f.IsD()) {
            for (size_t i = 0; i < 3; i++) {
                auto v = f.V(i);
                bool isWithin =
                    boost::geometry::within(point_type(v->P()[0], v->P()[1]), rangePolygon);
                if (!isWithin) {
                    f.SetD();
                }
            }
        }
    }
}
}

}   // namespace uavmvs
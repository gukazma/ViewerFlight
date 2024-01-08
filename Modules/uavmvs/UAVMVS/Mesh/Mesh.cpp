#define NOMINMAX
#include "Mesh.h"
#include <vcg/complex/algorithms/local_optimization.h>
#include <vcg/complex/algorithms/local_optimization/tri_edge_collapse_quadric.h>
#include <vcg/complex/algorithms/update/color.h>
#include <vcg/complex/complex.h>
#include<vcg/complex/algorithms/point_sampling.h>
#include<vcg/complex/algorithms/voronoi_processing.h>
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
MyMesh _tileMesh;
std::mutex _tileMutex;

namespace uavmvs {
namespace mesh {

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
        mps, _tileMesh, 10, pp);
    MyMesh PoissonMesh;
    vcg::tri::RequirePerVertexNormal<MyMesh>(PoissonMesh);

    vcg::tri::Allocator<MyMesh>::AddVertices(PoissonMesh, mps.sampleVec.size());

    for (size_t i = 0; i < mps.sampleVec.size(); ++i) {
        PoissonMesh.vert[i].P() = mps.sampleVec[i]->cP();
        PoissonMesh.vert[i].N() = mps.sampleVec[i]->cN();
    }

    vcg::tri::UpdateNormal<MyMesh>::NormalizePerVertex(PoissonMesh);
    vcg::tri::UpdateNormal<MyMesh>::NormalizePerVertex(_tileMesh);

    // 设置关联方式
    osg::ref_ptr<osg::Geode> geode = new osg::Geode();
    for (size_t i = 0; i < PoissonMesh.vert.size(); i++) {

        bool isWithin = boost::geometry::within(
            point_type(PoissonMesh.vert[i].P()[0], PoissonMesh.vert[i].P()[1]), rangePolygon);
        if (!isWithin) continue;
        osg::ref_ptr<osg::Sphere> sphere = new osg::Sphere(
            {PoissonMesh.vert[i].P()[0], PoissonMesh.vert[i].P()[1], PoissonMesh.vert[i].P()[2]},
            1.0f);
        osg::ref_ptr<osg::ShapeDrawable> shapeDrawable = new osg::ShapeDrawable(sphere);

        // 设置球体的颜色
        osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array;
        colors->push_back(osg::Vec4(1.0f, 0.0f, 0.0f, 1.0f));   // 红色
        shapeDrawable->setColorArray(colors);
        shapeDrawable->setColorBinding(osg::Geometry::BIND_OVERALL);
        geode->addDrawable(shapeDrawable);
    }

    return geode;
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
    osg::ref_ptr<osg::Vec3Array>        vertices = new osg::Vec3Array;
    osg::ref_ptr<osg::DrawElementsUInt> drawElements =
        new osg::DrawElementsUInt(osg::PrimitiveSet::TRIANGLES);
    osg::ref_ptr<osg::Geometry>         geometry = new osg::Geometry;

    for (size_t i = 0; i < _tileMesh.vert.size(); i++) {
        auto           v      = _tileMesh.vert[i];
        auto           n      = _tileMesh.vert[i].N();
        v.P() += (n * 5.0f);
        osg::Vec3 vertex = osg::Vec3(v.P().X(), v.P().Y(), v.P().Z());
        vertices->push_back(vertex);
    }

    for (int fn = 0; fn < _tileMesh.face.size(); fn++) {
        auto& f = _tileMesh.face[fn];
        if (!f.IsD()) {
            for (size_t i = 0; i < 3; i++) {
                auto                    v        = f.V(i);
                int index = vcg::tri::Index(_tileMesh, v);
                drawElements->push_back(index);
            }
        }
    }

    geometry->setVertexArray(vertices);
    geometry->addPrimitiveSet(drawElements);
    osg::ref_ptr<osg::StateSet> stateset = geometry->getOrCreateStateSet();
    geometry->setUseVertexBufferObjects(true);

    osgUtil::SmoothingVisitor::smooth(*geometry);
    osg::ref_ptr<osg::Geode> geode = new osg::Geode;
    geode->addDrawable(geometry);
    MakeTransparent(geode, 0.5);
    return geode;
}
}

}   // namespace uavmvs
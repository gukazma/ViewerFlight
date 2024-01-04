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
class MyVertex;
class MyEdge;
class MyFace;

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

osg::ref_ptr<osg::Geode> PossionDisk(std::vector<osg::Geometry*>& geometries) {
    MyMesh mesh;
    for (auto& geom : geometries) {
        MyMesh submesh;
        OSG2Mesh(geom, submesh);
        vcg::tri::Append<MyMesh, MyMesh>::Mesh(mesh, submesh);
    }
    vcg::tri::UpdateBounding<MyMesh>::Box(mesh);
    vcg::tri::Clean<MyMesh>::RemoveUnreferencedVertex(mesh);
    vcg::tri::Allocator<MyMesh>::CompactEveryVector(mesh);
    vcg::tri::UpdateTopology<MyMesh>::VertexFace(mesh);

    vcg::tri::SurfaceSampling<MyMesh, vcg::tri::TrivialPointerSampler<MyMesh>>::PoissonDiskParam pp;
    vcg::tri::UpdateFlags<MyMesh>::FaceBorderFromVF(mesh);
    vcg::tri::UpdateFlags<MyMesh>::VertexBorderFromFaceBorder(mesh);
    vcg::tri::UpdateNormal<MyMesh>::PerFace(mesh);
    vcg::tri::UpdateNormal<MyMesh>::PerVertex(mesh);

    vcg::tri::TrivialPointerSampler<MyMesh> mps;
    vcg::tri::SurfaceSampling<MyMesh, vcg::tri::TrivialPointerSampler<MyMesh>>::PoissonDiskPruning(
        mps, mesh, 10, pp);
    MyMesh PoissonMesh;
    vcg::tri::RequirePerVertexNormal<MyMesh>(PoissonMesh);

    vcg::tri::Allocator<MyMesh>::AddVertices(PoissonMesh, mps.sampleVec.size());

    for (size_t i = 0; i < mps.sampleVec.size(); ++i) {
        PoissonMesh.vert[i].P() = mps.sampleVec[i]->cP();
        PoissonMesh.vert[i].N() = mps.sampleVec[i]->cN();
    }

    vcg::tri::UpdateNormal<MyMesh>::NormalizePerVertex(PoissonMesh);

     // 创建顶点数组
    osg::ref_ptr<osg::Vec3Array> coords = new osg::Vec3Array();
    // 创建颜色
    osg::ref_ptr<osg::Vec4Array> color = new osg::Vec4Array();
    // 创建几何体
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry();
    // 设置顶点数组
    geometry->setVertexArray(coords.get());
    geometry->setColorArray(color.get());
    geometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);

    osg::Vec3Array* normals = new osg::Vec3Array;
    normals->push_back(osg::Vec3(0.0f, 1.0f, 0.0f));
    // geometry->setNormalArray(normals);
    // geometry->setNormalBinding(osg::Geometry::BIND_OVERALL);
    // 设置关联方式
    for (size_t i = 0; i < PoissonMesh.vert.size(); i++) {
        coords->push_back(
            {PoissonMesh.vert[i].P()[0], PoissonMesh.vert[i].P()[1], PoissonMesh.vert[i].P()[2]});
        color->push_back({0.0, 1.0, 0.0, 1.0f});
    }
    geometry->addPrimitiveSet(
        new osg::DrawArrays(osg::PrimitiveSet::POINTS, 0, PoissonMesh.vert.size()));
    // 创建一个点属性对象
    osg::ref_ptr<osg::Point> point = new osg::Point();

    // 设置点的大小
    point->setSize(100000.0f);
    point->setMinSize(1000.0f);
    // 将点属性对象添加到Geometry节点中
    geometry->getOrCreateStateSet()->setAttributeAndModes(point.get(), osg::StateAttribute::ON);
    // 添加到叶节点
    osg::ref_ptr<osg::Geode> geode = new osg::Geode();
    geode->addDrawable(geometry.get());
    vcg::tri::io::ExporterPLY<MyMesh>::Save(
        PoissonMesh, "D:/PoissonMesh.ply", vcg::tri::io::Mask::IOM_VERTNORMAL);
    
    return geode;
}
}

}   // namespace uavmvs
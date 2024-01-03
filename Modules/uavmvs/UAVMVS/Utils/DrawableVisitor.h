#include <osg/NodeVisitor>
#include <vector>

class DrawableVistor : public osg::NodeVisitor
{
public:
    DrawableVistor();
    ~DrawableVistor();

    void apply(osg::Geometry& geometry_) override;

    std::vector<osg::Geometry*> m_geoms;
};
#include <osg/NodeVisitor>

class DrawableVistor : public osg::NodeVisitor
{
public:
    DrawableVistor();
    ~DrawableVistor();

    void apply(osg::Geometry& geometry_) override;

    osg::ref_ptr<osg::Group> m_geodes;
    osg::ref_ptr<osg::Group> m_geodesRange;
};
#include "HUDAxis.h"
#include <osg/DrawPixels>
#include <osg/Geode>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/Group>
#include <osg/MatrixTransform>
#include <osg/Node>
#include <osg/NodeVisitor>
#include <osg/PolygonMode>
#include <osg/PositionAttitudeTransform>
#include <osg/Shape>
#include <osg/ShapeDrawable>
#include <osg/computeBoundsVisitor>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgViewer/Viewer>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
HUDAxis::HUDAxis()
{
    // ��������ֱ�Ӷ�ȡaxes.osgt;
    //  this->addChild(osgDB::readNodeFile("axes.osgt"));
}

HUDAxis::HUDAxis(const HUDAxis& copy, osg::CopyOp copyOp /* = CopyOp::SHALLOW_COPY */)
    : Camera(copy, copyOp)
    , _mainCamera(copy._mainCamera)
{}

// ÿ�λص�ʱ���ú����ͻᱻִ��
void HUDAxis::traverse(osg::NodeVisitor& nv)
{
    double fovy, aspectRatio, vNear, vFar;
    _mainCamera->getProjectionMatrixAsPerspective(fovy, aspectRatio, vNear, vFar);

    // ��Ϊ��ͶӰ����ͶӰ�����������������Զ���Ŵ���С���������޳�������Ч����
    this->setProjectionMatrixAsOrtho(-10.0 * aspectRatio,
                                     10.0 * aspectRatio,
                                     -10.0,
                                     10.0,
                                     vNear,
                                     vFar);   // ����ͶӰ����ʹ���Ų���Ч��

    osg::Vec3 trans(8.5 * aspectRatio, -8.5, -8.0);   // ��������ģ��λ�ڴ������½ǡ�
    if (_mainCamera.valid()) {
        osg::Matrix matrix = _mainCamera->getViewMatrix();   // �ı���ͼ�������ƶ�λ�ù̶�

        // ���ƶ��̶�,��ʼ��λ�ڴ������½ǣ�������������סģ�Ϳ����϶��򰴿ո��ʱģ�ͻᶯ
        matrix.setTrans(trans);
        this->setViewMatrix(matrix);
    }
    osg::Camera::traverse(nv);
}

HUDAxis::~HUDAxis() {}

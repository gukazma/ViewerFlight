#include "CLabelControlEventHandler.h"
#include <osgEarth/StringUtils>
#include <osgEarth/Terrain>

#include <QString>
CLabelControlEventHandler::CLabelControlEventHandler(osgEarth::MapNode* mapNode,
                                                     osgEarth::Util::Controls::LabelControl* label,
                                                     osgEarth::Formatter* formatter)
    : _mapNode(mapNode)
{
    _mapNodePath.push_back((osg::Node*)mapNode->getTerrainEngine());

    if (label) {
        label->setFont(osgText::readFontFile("simhei.ttf"));
        label->setEncoding(osgText::String::ENCODING_UTF8);
        label->setHaloColor(osg::Vec4(1.0, 0.5, 0.0, 1));
        label->setHorizAlign(osgEarth::Util::Controls::Control::ALIGN_RIGHT);
        label->setVertAlign(osgEarth::Util::Controls::Control::ALIGN_BOTTOM);

        addCallback(new MouseCoordsLabelCallback(label, formatter));
    }
}

void CLabelControlEventHandler::addCallback(CLabelControlEventHandler::Callback* cb)
{
    _callbacks.push_back(cb);
}

bool CLabelControlEventHandler::handle(const osgGA::GUIEventAdapter& ea,
                                       osgGA::GUIActionAdapter&      aa)
{
    if (ea.getEventType() == ea.MOVE || ea.getEventType() == ea.DRAG ||
        ea.getEventType() == ea.SCROLL) {
        osg::Vec3d world;
        if (_mapNode->getTerrain()->getWorldCoordsUnderMouse(
                aa.asView(), ea.getX(), ea.getY(), world)) {
            osgEarth::GeoPoint map;
            map.fromWorld(_mapNode->getMapSRS(), world);

            for (Callbacks::iterator i = _callbacks.begin(); i != _callbacks.end(); ++i)
                i->get()->set(map, aa.asView(), _mapNode);
        }
        else {
            for (Callbacks::iterator i = _callbacks.begin(); i != _callbacks.end(); ++i)
                i->get()->reset(aa.asView(), _mapNode);
        }
    }

    // if (ea.getEventType() == ea.KEYDOWN)
    //{
    //	osgEarth::Util::EarthManipulator *em = dynamic_cast<osgEarth::Util::EarthManipulator
    //*>(viewer->getCameraManipulator()); 	osgEarth::Util::Viewpoint vm = em->getViewpoint();

    //	double heading  = vm.getHeading();
    //	double pitch	= vm.getPitch();
    //	double range	= vm.getRange();

    //
    //	if (ea.getKey() == 'q')		 { vm.focalPoint()->x()--; }
    //	else if (ea.getKey() == 'w') { vm.focalPoint()->x()++; }

    //	else if (ea.getKey() == 'a') { vm.focalPoint()->y()--; }
    //	else if (ea.getKey() == 's') { vm.focalPoint()->y()++; }

    //	else if (ea.getKey() == 'z') { vm.focalPoint()->z()-=100000; }
    //	else if (ea.getKey() == 'x') { vm.focalPoint()->z()+=100000; }

    //	else if (ea.getKey() == '7') { --heading; }
    //	else if (ea.getKey() == '8') { ++heading; }

    //	else if (ea.getKey() == '4') { --pitch; }
    //	else if (ea.getKey() == '5') { ++pitch; }

    //	else if (ea.getKey() == '1') { --range; }
    //	else if (ea.getKey() == '2') { ++range; }

    //	vm.setHeading(heading);
    //	vm.setPitch(pitch);
    //	vm.setRange(range);
    //	em->setViewpoint(vm);

    //}

    return false;
}

//-----------------------------------------------------------------------

MouseCoordsLabelCallback::MouseCoordsLabelCallback(osgEarth::Util::Controls::LabelControl* label,
                                                   osgEarth::Formatter* formatter)
    : _label(label)
    , _formatter(formatter)
{}

void MouseCoordsLabelCallback::set(const osgEarth::GeoPoint& mapCoords, osg::View* view,
                                   osgEarth::MapNode* mapNode)
{
    if (_label.valid()) {
        if (_formatter) {
            _label->setText(osgEarth::Util::Stringify()
                            << _formatter->format(mapCoords) << ", " << mapCoords.z());
        }
        else {
            char wsrc[512];
            sprintf(wsrc,
                    "%s:%.4f %s:%.4f %s:%.4f",
                    QString::fromLocal8Bit("经度").toUtf8().constData(),
                    mapCoords.x(),
                    QString::fromLocal8Bit("纬度").toUtf8().constData(),
                    mapCoords.y(),
                    QString::fromLocal8Bit("海拔").toUtf8().constData(),
                    mapCoords.z());
            _label->setText(wsrc);
        }
    }
}

void MouseCoordsLabelCallback::reset(osg::View* view, osgEarth::MapNode* mapNode)
{
    if (_label.valid()) {
        _label->setText("");
    }
}

//-----------------------------------------------------------------------

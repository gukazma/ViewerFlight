#include "Settings.h"


namespace uavmvs {
namespace context {
double Settings::getDistance()
{
    return (focal * diskRadius) / cmosSize * 2.0;
}
}   // namespace context
}   // namespace uavmvs


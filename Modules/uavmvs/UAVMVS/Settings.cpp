#include "Settings.h"


namespace uavmvs {
namespace context {
double Settings::getDistance()
{
    return (focal * diskRadius) / cmosSize;
}
}   // namespace context
}   // namespace uavmvs


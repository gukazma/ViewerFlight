#pragma once

namespace uavmvs {
namespace context {
struct Settings
{
    Settings()        = default;
    ~Settings()        = default;
    double focal      = 0.86;
    double diskRadius = 10;
};
}   // namespace context
}
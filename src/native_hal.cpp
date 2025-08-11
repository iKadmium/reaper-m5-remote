#ifdef NATIVE_BUILD

#include "native_hal.h"

namespace hal
{
    // Static member definitions
    NativeDisplayManager *NativeDisplayManager::instance = nullptr;
    lv_color_t NativeSystemHAL::buf_1[NativeSystemHAL::buf_size];
    lv_color_t NativeSystemHAL::buf_2[NativeSystemHAL::buf_size];
}

#endif // NATIVE_BUILD

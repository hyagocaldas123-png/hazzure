// Definitions for NativeInventoryRuntime globals (ODR-safe single definition)
#include "NativeInventoryRuntime.h"

namespace HazzureNativeInventory {
Runtime runtime;
Engine nativeEngine;
SRWLOCK frameLock = SRWLOCK_INIT;
std::atomic<bool> backendAvailable{false};
}

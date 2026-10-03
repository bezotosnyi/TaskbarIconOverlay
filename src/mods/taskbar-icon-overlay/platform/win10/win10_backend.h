#pragma once

#include "core/taskbar_backend.h"

// Returns the process-lifetime Windows 10 implementation.
TaskbarBackend& GetWin10Backend();

#pragma once

#include "core/taskbar_backend.h"

// Selects the backend once per Explorer process from the real Windows build.
TaskbarBackend& GetTaskbarBackend();

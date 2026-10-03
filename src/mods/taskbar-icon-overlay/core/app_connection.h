#pragma once

#include "core/shared_config.h"

namespace AppConnection
{
    using ConfigChangedCallback = void (*)();

    bool Connect(ConfigChangedCallback configChangedCallback);
    void Disconnect();

    bool IsConnected();
    bool IsEnabled();
    const SharedConfig::Layout* GetLayout();
} // namespace AppConnection

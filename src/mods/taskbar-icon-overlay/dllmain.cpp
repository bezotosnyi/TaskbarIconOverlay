#include "dllmain.h"

void InitializeModDefaultSettings()
{
    Wh_SetStringValue(L"numberPosition", L"topLeft");
    Wh_SetIntValue(L"numberSize", 12);
    Wh_SetStringValue(L"iconsPath", L"C:\\Program Files\\Windhawk\\icons");
    Wh_SetStringValue(L"numberColor", L"#FFFFFF");
    Wh_SetStringValue(L"backgroundColor", L"#80000000");
    Wh_SetIntValue(L"showOnAllTaskbars", 0); // false
}

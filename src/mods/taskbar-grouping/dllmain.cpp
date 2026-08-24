#include "dllmain.h"

void InitializeModDefaultSettings()
{
    Wh_SetStringValue(L"pinnedItemsMode", L"replace");
    Wh_SetStringValue(L"placeUngroupedItemsTogether", L"0");
    Wh_SetIntValue(L"useWindowIcons", 0); // false
    Wh_SetStringValue(L"groupingMode", L"regular");
    Wh_SetIntValue(L"oldTaskbarOnWin11", 0); // false
}

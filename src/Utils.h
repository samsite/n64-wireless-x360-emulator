#pragma once

#include <Guiddef.h>
#include <Winerror.h>

#include <string>

namespace Utils
{

std::string ErrToString(HRESULT hr);
GUID        StringToGuid(const std::string& str);
std::string GuidToString(GUID guid);

};

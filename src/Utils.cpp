#include "Utils.h"

#include <comdef.h>
#include <atlstr.h>

std::string Utils::ErrToString(HRESULT hr)
{
    _com_error err(hr);
    return std::string(CT2A(err.ErrorMessage()));
}

GUID Utils::StringToGuid(const std::string& str)
{
    GUID guid;
    sscanf(str.c_str(),
           "%8x-%4hx-%4hx-%2hhx%2hhx-%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx",
           &guid.Data1, &guid.Data2, &guid.Data3,
           &guid.Data4[0], &guid.Data4[1], &guid.Data4[2], &guid.Data4[3],
           &guid.Data4[4], &guid.Data4[5], &guid.Data4[6], &guid.Data4[7] );
    return guid;
}

std::string Utils::GuidToString(GUID guid)
{
    char guid_cstr[39];
    snprintf(guid_cstr, sizeof(guid_cstr),
             "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
             guid.Data1, guid.Data2, guid.Data3,
             guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
             guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
    return std::string(guid_cstr);
}

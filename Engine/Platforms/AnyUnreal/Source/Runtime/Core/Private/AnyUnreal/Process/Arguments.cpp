#include "AnyUnreal/Process/Arguments.h"
#include "AnyUnreal/Text/Encoding.h"
#include "prx/libkernel/include/Api.hpp"
#include <stdexcept>

namespace AnyUnreal::Process
{
    std::vector<std::basic_string<TCHAR>> Arguments::Read()
    {
        return Decode(getargc_nid_postfix(), getargv_nid_postfix());
    }

    std::vector<std::basic_string<TCHAR>> Arguments::Decode(int count, const char* const* arguments)
    {
        if (count < 0 || (count != 0 && arguments == nullptr)) throw std::invalid_argument("Invalid process argument list");
        std::vector<std::basic_string<TCHAR>> result;
        result.reserve(static_cast<std::size_t>(count));
        for (int index = 0; index != count; ++index)
        {
            if (arguments[index] == nullptr) throw std::invalid_argument("Null process argument");
            result.push_back(Text::DecodeUtf8(arguments[index]));
        }
        return result;
    }
}

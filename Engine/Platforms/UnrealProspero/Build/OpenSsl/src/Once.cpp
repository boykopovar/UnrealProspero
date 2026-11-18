#include <__external_threading>

extern "C" int UnrealProsperoOpenSslRunOnce(unsigned int* flag, void (*initialize)())
{
    return std::__libcpp_execute_once(flag, initialize);
}

#include "UnrealProspero/Kernel/UserService.h"
#include <stdexcept>

bool CheckUsers()
{
    using UnrealProspero::Kernel::UserService;
    {
        UserService first;
        UserService second;
        const int user = first.GetInitialUser();
        if (second.GetInitialUser() != user) return false;
        first.Close();
        if (second.GetInitialUser() != user) return false;
        try
        {
            first.GetInitialUser();
            return false;
        }
        catch (const std::logic_error&)
        {
        }
        first.Close();
    }
    UserService reopened;
    return reopened.GetInitialUser() >= 0;
}

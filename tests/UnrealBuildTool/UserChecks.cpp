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
        if (first.GetUserIndex(user) != 0 || second.GetUserId(0) != user) return false;
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
        try
        {
            first.GetUserId(0);
            return false;
        }
        catch (const std::logic_error&)
        {
        }
        try
        {
            second.GetUserId(-1);
            return false;
        }
        catch (const std::invalid_argument&)
        {
        }
        try
        {
            second.GetUserId(2147483647);
            return false;
        }
        catch (const std::out_of_range&)
        {
        }
    }
    UserService reopened;
    return reopened.GetUserId(0) == reopened.GetInitialUser() && reopened.GetUserIndex(reopened.GetInitialUser()) == 0;
}

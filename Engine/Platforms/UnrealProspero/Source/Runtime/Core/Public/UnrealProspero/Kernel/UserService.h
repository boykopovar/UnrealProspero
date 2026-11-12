#ifndef UNREALPROSPERO_KERNEL_USER_SERVICE_H
#define UNREALPROSPERO_KERNEL_USER_SERVICE_H

namespace UnrealProspero::Kernel
{
    class UserService final
    {
    public:
        UserService();
        ~UserService() noexcept(false);
        UserService(const UserService&) = delete;
        UserService& operator=(const UserService&) = delete;
        int GetInitialUser() const;
        void Close();

    private:
        bool active = false;
    };
}

#endif

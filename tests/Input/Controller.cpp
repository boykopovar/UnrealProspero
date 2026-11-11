#include "AnyUnreal/Input/ControllerHub.h"
#include <iostream>
#include <set>
#include <stdexcept>

int main()
{
    AnyUnreal::Input::ControllerHub controllers;
    if (controllers.GetInitialUser() < 0) throw std::runtime_error("Invalid initial controller user");
    std::size_t connected = 0;
    for (unsigned poll = 0; poll < 16; ++poll)
    {
        const auto samples = controllers.Poll();
        if (samples.size() > 4) throw std::runtime_error("Too many logged-in controller users");
        std::set<int> users;
        connected = 0;
        for (const auto& sample : samples)
        {
            if (sample.UserId < 0 || !users.insert(sample.UserId).second) throw std::runtime_error("Invalid controller user association");
            if (sample.Data.connected) ++connected;
        }
    }
    controllers.Close();
    controllers.Close();
    bool rejected = false;
    try { static_cast<void>(controllers.Poll()); }
    catch (const std::runtime_error&) { rejected = true; }
    if (!rejected) throw std::runtime_error("Closed controller service accepted polling");
    std::cout << "Controller service lifecycle verified; connected controllers: " << connected << '\n';
    return 0;
}

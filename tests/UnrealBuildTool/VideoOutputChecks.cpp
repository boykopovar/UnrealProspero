#include "AnyUnreal/Video/Output.h"
#include "AnyUnreal/Kernel/Error.h"
#include <stdexcept>

int main()
{
    using AnyUnreal::Video::Output;
    for (int iteration = 0; iteration != 3; ++iteration)
    {
        Output output;
        const auto resolution = output.GetResolution();
        if (resolution.fullWidth == 0 || resolution.fullHeight == 0) return 61;
        try
        {
            Output duplicate;
            return 62;
        }
        catch (const AnyUnreal::Kernel::Error&)
        {
        }
        const auto after = output.GetResolution();
        if (after.fullWidth != resolution.fullWidth || after.fullHeight != resolution.fullHeight) return 63;
        for (int rate = 0; rate != 3; ++rate) output.SetFlipRate(rate);
        try
        {
            output.SetFlipRate(3);
            return 64;
        }
        catch (const std::invalid_argument&)
        {
        }
        output.Close();
        output.Close();
        try
        {
            output.GetHandle();
            return 65;
        }
        catch (const std::logic_error&)
        {
        }
    }
    return 0;
}

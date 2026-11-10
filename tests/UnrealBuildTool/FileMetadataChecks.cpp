#include "AnyUnreal/File/Metadata.h"
#include "AnyUnreal/File/Timestamp.h"
#include <limits>
#include <stdexcept>

namespace
{
    template<typename TException, typename TOperation>
    bool rejects(TOperation operation)
    {
        try
        {
            operation();
        }
        catch (const TException&)
        {
            return true;
        }
        return false;
    }
}

bool CheckFileMetadata()
{
    using AnyUnreal::File::Timestamp;
    using AnyUnreal::File::Metadata;
    constexpr int64 epochTicks = 621355968000000000;
    if (Timestamp::ToDateTime({0, 0}).GetTicks() != epochTicks) return false;
    if (Timestamp::ToDateTime({-1, 999999999}).GetTicks() != epochTicks - 1) return false;
    if (Timestamp::ToDateTime({0, 199}).GetTicks() != epochTicks + 1) return false;
    if (Timestamp::ToDateTime({-62135596800, 0}).GetTicks() != FDateTime::MinValue().GetTicks()) return false;
    if (Timestamp::ToDateTime({253402300799, 999999999}).GetTicks() != FDateTime::MaxValue().GetTicks()) return false;
    if (!rejects<std::out_of_range>([] { Timestamp::ToDateTime({-62135596801, 0}); })) return false;
    if (!rejects<std::out_of_range>([] { Timestamp::ToDateTime({253402300800, 0}); })) return false;
    if (!rejects<std::out_of_range>([] { Timestamp::ToDateTime({std::numeric_limits<int64>::max(), 0}); })) return false;
    if (!rejects<std::invalid_argument>([] { Timestamp::ToDateTime({0, -1}); })) return false;
    if (!rejects<std::invalid_argument>([] { Timestamp::ToDateTime({0, 1000000000}); })) return false;
    const auto beforeEpoch = Timestamp::ToTimeval(FDateTime(epochTicks - 1));
    if (beforeEpoch.tv_sec != -1 || beforeEpoch.tv_usec != 999999) return false;
    const auto minimum = Timestamp::ToTimeval(FDateTime::MinValue());
    if (minimum.tv_sec != -62135596800 || minimum.tv_usec != 0) return false;
    const auto maximum = Timestamp::ToTimeval(FDateTime::MaxValue());
    if (maximum.tv_sec != 253402300799 || maximum.tv_usec != 999999) return false;
    if (!rejects<std::out_of_range>([] { Timestamp::ToTimeval(FDateTime(-1)); })) return false;
    if (!rejects<std::out_of_range>([] { Timestamp::ToTimeval(FDateTime(std::numeric_limits<int64>::max())); })) return false;
    FileStat status{};
    status.st_mode = 0100444;
    status.st_size = 123;
    status.st_birthtim = {-1, 0};
    status.st_atim = {1, 500000000};
    status.st_mtim = {2, 100};
    const auto file = Metadata::FromStat(status);
    if (!file.bIsValid || file.bIsDirectory || !file.bIsReadOnly || file.FileSize != 123) return false;
    if (file.CreationTime.GetTicks() != epochTicks - 10000000 || file.AccessTime.GetTicks() != epochTicks + 15000000 || file.ModificationTime.GetTicks() != epochTicks + 20000001) return false;
    status.st_mode = 0040755;
    const auto directory = Metadata::FromStat(status);
    if (!directory.bIsValid || !directory.bIsDirectory || directory.bIsReadOnly || directory.FileSize != -1) return false;
    status.st_mode = 0100644;
    status.st_size = -1;
    return rejects<std::invalid_argument>([&] { Metadata::FromStat(status); });
}

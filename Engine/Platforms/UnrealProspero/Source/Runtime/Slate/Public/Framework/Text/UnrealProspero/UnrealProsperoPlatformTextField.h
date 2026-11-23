#ifndef UNREALPROSPERO_PLATFORM_TEXT_FIELD_H
#define UNREALPROSPERO_PLATFORM_TEXT_FIELD_H

#include "Framework/Application/IPlatformTextField.h"
#include "Containers/Ticker.h"
#include <array>
#include <cstddef>

class FUnrealProsperoPlatformTextField final : public IPlatformTextField
{
public:
    ~FUnrealProsperoPlatformTextField() override;
    void ShowVirtualKeyboard(bool show, int32 userIndex, TSharedPtr<IVirtualKeyboardEntry> entry) override;
    bool AllowMoveCursor() override;

private:
    bool poll();
    void close();

    static constexpr std::size_t maxTextLength = 2048;
    std::array<char16_t, maxTextLength + 1> textBuffer{};
    TSharedPtr<IVirtualKeyboardEntry> activeEntry;
    FTSTicker::FDelegateHandle ticker;
};

using FPlatformTextField = FUnrealProsperoPlatformTextField;

#endif

#include "Framework/Text/UnrealProspero/UnrealProsperoPlatformTextField.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Application/SlateUser.h"
#include "Containers/StringConv.h"
#include "UnrealProspero/Kernel/Result.h"
#include "UnrealProspero/Kernel/UserService.h"
#include "prx/libSceImeDialog/include/Api.hpp"
#include <algorithm>
#include <stdexcept>

namespace
{
    constexpr int running = 1;
    constexpr int finished = 2;
    constexpr unsigned multiline = 1;
    constexpr unsigned accepted = 0;
    constexpr unsigned canceled = 1;
    constexpr unsigned aborted = 2;
}

FUnrealProsperoPlatformTextField::~FUnrealProsperoPlatformTextField()
{
    if (activeEntry) close();
}

void FUnrealProsperoPlatformTextField::ShowVirtualKeyboard(bool show, int32 userIndex, TSharedPtr<IVirtualKeyboardEntry> entry)
{
    if (!show)
    {
        if (activeEntry) close();
        return;
    }
    if (!entry || activeEntry) throw std::logic_error("Cannot open the virtual keyboard for this entry");
    if (entry->GetVirtualKeyboardType() != Keyboard_Default) throw std::invalid_argument("Unsupported virtual keyboard type");

    const auto user = FSlateApplication::Get().GetUser(userIndex);
    if (!user || !user->GetPlatformUserId().IsValid()) throw std::invalid_argument("Virtual keyboard requires a valid Slate user");

    const FString text = entry->GetText().ToString();
    const auto utf16 = StringCast<UTF16CHAR>(*text, text.Len());
    if (static_cast<std::size_t>(utf16.Length()) > maxTextLength) throw std::length_error("Virtual keyboard text exceeds its limit");

    textBuffer.fill(u'\0');
    std::copy_n(utf16.Get(), utf16.Length(), textBuffer.begin());

    const UnrealProspero::Kernel::UserService users;
    Param parameters{};
    parameters.user_id = users.GetUserId(user->GetPlatformUserId().GetInternalId());
    parameters.option = entry->IsMultilineEntry() ? multiline : 0;
    parameters.max_text_length = maxTextLength;
    parameters.input_text_buffer = textBuffer.data();
    UnrealProspero::Kernel::CheckResult(sceImeDialogInit(&parameters, nullptr), "sceImeDialogInit");
    activeEntry = MoveTemp(entry);
    ticker = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([this](float) { return poll(); }));
}

bool FUnrealProsperoPlatformTextField::AllowMoveCursor()
{
    return !activeEntry.IsValid();
}

bool FUnrealProsperoPlatformTextField::poll()
{
    const int status = sceImeDialogGetStatus();
    if (status == running) return true;
    if (status != finished) throw UnrealProspero::Kernel::Error(status, "sceImeDialogGetStatus");

    ImeDialogResult result{};
    UnrealProspero::Kernel::CheckResult(sceImeDialogGetResult(&result), "sceImeDialogGetResult");
    const auto entry = MoveTemp(activeEntry);
    ticker.Reset();
    UnrealProspero::Kernel::CheckResult(sceImeDialogTerm(), "sceImeDialogTerm");

    if (result.endstatus == accepted)
    {
        const auto end = std::find(textBuffer.begin(), textBuffer.end(), u'\0');
        if (end == textBuffer.end()) throw std::runtime_error("Unterminated virtual keyboard text");
        TArray<UTF16CHAR> utf16;
        utf16.Reserve(static_cast<int32>(end - textBuffer.begin()) + 1);
        for (auto it = textBuffer.begin(); it != end; ++it) utf16.Add(static_cast<UTF16CHAR>(*it));
        utf16.Add(0);
        const FUTF16ToTCHAR text(utf16.GetData());
        entry->SetTextFromVirtualKeyboard(FText::FromString(FString(text.Length(), text.Get())), ETextEntryType::TextEntryAccepted);
    }
    else if (result.endstatus == canceled || result.endstatus == aborted)
    {
        entry->SetTextFromVirtualKeyboard(entry->GetText(), ETextEntryType::TextEntryCanceled);
    }
    else throw std::runtime_error("Unknown virtual keyboard result");
    return false;
}

void FUnrealProsperoPlatformTextField::close()
{
    FTSTicker::RemoveTicker(ticker);
    ticker.Reset();
    const int status = sceImeDialogGetStatus();
    if (status == running) UnrealProspero::Kernel::CheckResult(sceImeDialogAbort(), "sceImeDialogAbort");
    else if (status != finished) throw UnrealProspero::Kernel::Error(status, "sceImeDialogGetStatus");
    UnrealProspero::Kernel::CheckResult(sceImeDialogTerm(), "sceImeDialogTerm");
    activeEntry.Reset();
}

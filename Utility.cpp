/*
Copyright © 2019 - 2026 by Benilda Key

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
OF SUCH DAMAGE.
*/

#include "stdafx.h"
#include <array>
#include <map>
#include <memory>
#include <string>
#include <sstream>
#include <lmerr.h>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/numeric/conversion/cast.hpp>

#include "mingw_wil_compat.h"

#include "Utility.h"

#include "App.h"

using namespace std::string_literals;

namespace Utility {

namespace detail {

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
HINSTANCE resouceInstance{nullptr};
const size_t maxStringTableResourceLen{SIZE_T_C(4096)};

/* The following code is based on the article *The universal resource class pattern* found at
http://cpp.indi.frih.net/blog/2015/07/the-universal-resource-class-pattern/. */
struct clipboard_
{
};
// NOLINTNEXTLINE(readability-named-parameter)
static BOOL CloseClipboardHelper(clipboard_*)
{
    return CloseClipboard();
}
using clipboard_ptr = std::unique_ptr<clipboard_, decltype(&CloseClipboardHelper)>;

} // namespace detail

/**
@brief Obtains a textual description of of a System Error Code.

This function is used to obtain a textual description of a System Error Code.

@param error
  The system error code value.

@see [GetLastError](https://docs.microsoft.com/en-us/windows/win32/api/errhandlingapi/nf-errhandlingapi-getlasterror)
@see [FormatMessageW](https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-formatmessagew)
@see [System Error Codes](https://docs.microsoft.com/en-us/windows/win32/debug/system-error-codes).
@return The textual description of the error.
*/
std::wstring DescribeError(IN DWORD error /* = ::GetLastError() */)
{
    wil_compat::unique_hmodule source{};
    LPWSTR buffer{nullptr};
    DWORD flags{FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_IGNORE_INSERTS|FORMAT_MESSAGE_FROM_SYSTEM};
    // If error is in the network range, load the message source.
    if (error >= NERR_BASE && error <= MAX_NERR)
    {
        source.reset(LoadLibraryExW(L"netmsg.dll", nullptr, LOAD_LIBRARY_AS_DATAFILE));
        if (source.get())
        {
            flags |= FORMAT_MESSAGE_FROM_HMODULE;
        }
    }
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-c-style-cast)
    const DWORD languageId{MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT)};
    /* Call FormatMessage() to allow for message text to be acquired from the system or from the supplied module handle. */
    const auto result{FormatMessageW(flags, source.get(), error, languageId, reinterpret_cast<LPWSTR>(&buffer), 0, nullptr)};
    if (result == 0 || buffer == nullptr)
    {
        return {};
    }
    std::wstring ret{buffer};
    LocalFree(buffer);
    return ret;
}

void SetResouceInstance(HINSTANCE hInstance)
{
    detail::resouceInstance = hInstance;
}

HINSTANCE GetResouceInstance()
{
    return detail::resouceInstance;
}

std::wstring loadString(HINSTANCE hInstance, UINT uID)
{
    std::array<wchar_t, detail::maxStringTableResourceLen> buffer{};
    try
    {
        const auto result{LoadStringW(hInstance, uID, buffer.data(), boost::numeric_cast<int>(buffer.size()))};
        if (result == FALSE || buffer.at(0) == 0)
        {
            return {};
        }
    }
    catch (boost::bad_numeric_cast&)
    {
        return {};
    }
    return buffer.data();
}

std::wstring loadString(UINT uID)
{
    return loadString(GetResouceInstance(), uID);
}

HANDLE loadImage(LPCWSTR name, UINT type, int cx, int cy, UINT fuLoad)
{
    auto* const hImage{LoadImageW(GetResouceInstance(), name, type, cx, cy, fuLoad)};
    return hImage;
}

RECT GetMonitorRectForWindow(HWND hwndWindow)
{
    const auto nScreenWidth{GetSystemMetrics(SM_CXSCREEN)};
    const auto nScreenHeight{GetSystemMetrics(SM_CYSCREEN)};
    auto* const monitor{MonitorFromWindow(hwndWindow, MONITOR_DEFAULTTONEAREST)};
    if (monitor == nullptr)
    {
        return { 0, 0, nScreenWidth, nScreenHeight };
    }
    MONITORINFO info{};
    info.cbSize = sizeof(info);
    const auto getInfo{GetMonitorInfoW(monitor, &info)};
    if (getInfo == FALSE)
    {
        return { 0, 0, nScreenWidth, nScreenHeight };
    }
    return info.rcMonitor;
}

bool CenterWindow(HWND hwndWindow)
{
    auto* hwndParent{GetParent(hwndWindow)};
    if (hwndParent == nullptr)
    {
        hwndParent = GetDesktopWindow();
    }
    if (hwndParent == nullptr)
    {
        return false;
    }
    RECT rectWindow{};
    if (GetWindowRect(hwndWindow, &rectWindow) == FALSE)
    {
        return false;
    }
    RECT rectParent{};
    if (GetWindowRect(hwndParent, &rectParent) == FALSE)
    {
        return false;
    }
    // make the window relative to its parent
    const auto nWidth{rectWindow.right - rectWindow.left};
    const auto nHeight{rectWindow.bottom - rectWindow.top};
    auto nX{(((rectParent.right - rectParent.left) - nWidth) / 2) + rectParent.left};
    auto nY{(((rectParent.bottom - rectParent.top) - nHeight) / 2) + rectParent.top};
    const auto monitorRect{GetMonitorRectForWindow(hwndWindow)};
    // make sure that the dialog box never moves outside of the screen
    nX = std::max(nX, monitorRect.left);
    nY = std::max(nY, monitorRect.top);
    nX = std::min(nX, monitorRect.right - nWidth);
    nY = std::min(nY, monitorRect.bottom - nHeight);
    MoveWindow(hwndWindow, nX, nY, nWidth, nHeight, FALSE);
    return true;
}

std::wstring getWindowText(HWND window)
{
    if (window == nullptr || IsWindow(window) == FALSE)
    {
        return {};
    }
    auto textLength{GetWindowTextLengthW(window)};
    if (textLength <= 0)
    {
        return {};
    }
    ++textLength;
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
    auto windowText{std::make_unique<wchar_t[]>(static_cast<size_t>(textLength))};
    if (!windowText)
    {
        return {};
    }
    if (GetWindowTextW(window, windowText.get(), textLength) <= 0)
    {
        return {};
    }
    return windowText.get();
}

std::wstring getClassName(HWND window)
{
    std::array<wchar_t, MAX_CLASS_NAME + 1> className{};
    const auto result{GetClassNameW(window, className.data(), static_cast<int>(className.size()))};
    if (result == 0)
    {
        return {};
    }
    return className.data();
}

std::wstring GetDialogBoxText(HWND dialog)
{
    std::wstringstream xmlStream{};
    auto windowText{Utility::getWindowText(dialog)};
    xmlStream << L"<?xml version=\"1.0\" ?>\r\n";
    xmlStream << L"<Dialog Name=\"" << ReplaceXMLCharacterEntities(windowText) << L"\">\r\n";
    for (HWND window = GetWindow(dialog, GW_CHILD); window != nullptr;  window = GetNextWindow(window, GW_HWNDNEXT))
    {
        windowText = Utility::getWindowText(window);
        if (windowText.empty())
        {
            continue;
        }
        const auto className{getClassName(window)};
        if (boost::iequals(className, L"Static"))
        {
            xmlStream << L"  <Static Name=\"" << ReplaceXMLCharacterEntities(windowText) << L"\"/>\r\n";
        }
        else if (boost::iequals(className, L"SysLink"))
        {
            xmlStream << L"  <SysLink Name=\"" << ReplaceXMLCharacterEntities(windowText) << L"\"/>\r\n";
        }
        else if (boost::iequals(className, L"Button"))
        {
            xmlStream << L"  <Button Name=\"" << ReplaceXMLCharacterEntities(windowText) << L"\"/>\r\n";
        }
    }
    xmlStream << L"</Dialog>\r\n";
    return xmlStream.str();
}

bool CopyToClipboard(HWND hwndOwner, const std::wstring& text)
{
    if (text.empty())
    {
        return false;
    }
    const auto size{(static_cast<SIZE_T>(text.length()) + 1) * sizeof(wchar_t)};
    auto clipboardText{wil_compat::unique_hglobal{GlobalAlloc(GMEM_MOVEABLE|GMEM_DDESHARE, size)}};
    if (!clipboardText)
    {
        return false;
    }
    auto clipboardTextPtr{wil_compat::unique_hglobal_locked{GlobalLock(clipboardText.get())}};
    if (!clipboardTextPtr)
    {
        return false;
    }
    std::copy_n(text.c_str(), text.length() + 1, static_cast<wchar_t*>(clipboardTextPtr.get()));
    if (OpenClipboard(hwndOwner) == FALSE)
    {
        return false;
    }
    if (EmptyClipboard() == FALSE)
    {
        return false;
    }
    auto dummy{detail::clipboard_{}};
    auto closeClipboard{detail::clipboard_ptr{&dummy, &detail::CloseClipboardHelper}};
    return (SetClipboardData(CF_UNICODETEXT, clipboardText.get()) != FALSE);
}

bool CopyToDialogBoxTextClipboard(HWND dialog)
{
    const auto dialogText{GetDialogBoxText(dialog)};
    if (dialogText.empty())
    {
        return false;
    }
    return CopyToClipboard(dialog, dialogText);
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
int messageBox(HWND hWnd, UINT textID, UINT captionID, UINT uType)
{
    const auto text{Utility::loadString(textID)};
    const auto caption{Utility::loadString(captionID)};
    return MessageBoxW(hWnd, text.c_str(), caption.c_str(), uType);
}

bool getTextExtentPoint32(HWND window, const std::wstring& str, SIZE& size)
{
    if (str.empty())
    {
        return false;
    }
	DCObject dc(window);
    if (!dc)
    {
        return false;
    }
    const auto result{GetTextExtentPoint32W(dc.get(), str.c_str(), static_cast<int>(str.length()), &size)};
    return (result != FALSE);
}

HWND CreateDialogWithAccelerators(_In_opt_ HINSTANCE hInstance, _In_ LPCWSTR dialogTemplateName, _In_opt_ HWND parent,
    _In_opt_ DLGPROC dialogFunc, _In_ LPARAM initParam, _In_ LPCWSTR acceleratorTableName, _Out_ HACCEL& acceleratorTable)
{
    acceleratorTable = nullptr;
    if (hInstance == nullptr || dialogTemplateName == nullptr || dialogFunc == nullptr || acceleratorTableName == nullptr)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return nullptr;
    }
    acceleratorTable = LoadAcceleratorsW(hInstance, acceleratorTableName);
    if (acceleratorTable == nullptr)
    {
        return nullptr;
    }
    auto* const dialogWindow{CreateDialogParamW(hInstance, dialogTemplateName, parent, dialogFunc, initParam)};
    if (dialogWindow == nullptr)
    {
        const auto error{GetLastError()};
        if (error != ERROR_SUCCESS)
        {
            const auto errorDescription{DescribeError(error)};
            MessageBoxW(nullptr, errorDescription.c_str(), L"CreateDialogParamW failed", MB_OK|MB_ICONEXCLAMATION);
        }
    }
    return dialogWindow;
}

#if defined(__GNUC__) && (__GNUC__ >= 8)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wcast-function-type"
#endif
ULONGLONG GetDllVersion(LPCWSTR dllName)
{
    wil_compat::unique_hmodule module(LoadLibraryW(dllName));
    if (!module)
    {
        return 0;
    }
    // NOLINTNEXTLINE(clang-diagnostic-cast-function-type,clang-diagnostic-cast-function-type-strict)
    auto pDllGetVersion{reinterpret_cast<DLLGETVERSIONPROC>(GetProcAddress(module.get(), "DllGetVersion"))};
    if (pDllGetVersion == nullptr)
    {
        return 0;
    }
    DLLVERSIONINFO dvi{};
    dvi.cbSize = sizeof(dvi);
    auto hr{(*pDllGetVersion)(&dvi)};
    if (FAILED(hr))
    {
        return 0;
    }
    const auto ret{MAKEDLLVERULL(dvi.dwMajorVersion, dvi.dwMinorVersion, dvi.dwBuildNumber, dvi.dwPlatformID)};
    return ret;
}
#if defined(__GNUC__) && (__GNUC__ >= 8)
#  pragma GCC diagnostic pop
#endif

HWND GetTaskbarWindow()
{
    auto* hWnd{FindWindowExW(nullptr, nullptr, L"Shell_TrayWnd", nullptr)};
    if (hWnd == nullptr)
    {
        return nullptr;
    }
    hWnd = FindWindowExW(hWnd, nullptr, L"TrayNotifyWnd", nullptr);
    return hWnd;
}

std::wstring ReplaceXMLCharacterEntities(const std::wstring& str)
{
    // NOLINTNEXTLINE(clang-diagnostic-exit-time-destructors,clang-diagnostic-global-constructors)
    static std::map<std::wstring, std::wstring> characterEntities{{ {L"<"s, L"&lt;"s}, {L">"s, L"&gt;"s},
        {L"&"s, L"&amp;"s}, {L"\""s, L"&quot;"s}, {L"'"s, L"&apos;"s} }};
    auto newStr{str};
    for (const auto& item : characterEntities)
    {
        boost::replace_all(newStr, item.first, item.second);
    }
    return newStr;
}

std::wstring ReplaceHTMLCharacterEntities(const std::wstring& str)
{
    // NOLINTNEXTLINE(clang-diagnostic-exit-time-destructors,clang-diagnostic-global-constructors)
    static std::map<std::wstring, std::wstring> characterEntities{{ {L"<"s, L"&lt;"s}, {L">"s, L"&gt;"s}, {L"&"s, L"&amp;"s},
        {L"\""s, L"&quot;"s}, {L"'"s, L"&apos;"s}, {L"¢"s, L"&cent;"s}, {L"£"s, L"&pound;"s}, {L"¥"s, L"&yen;"s},
        {L"€"s, L"&euro;"s}, {L"©"s, L"&copy;"s}, {L"®"s, L"&reg;"s} }};
    auto newStr{str};
    for (const auto& item : characterEntities)
    {
        boost::replace_all(newStr, item.first, item.second);
    }
    return newStr;
}

} // namespace Utility

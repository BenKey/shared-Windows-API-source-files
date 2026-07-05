/*
Copyright 2019 - 2026 by Benilda Key

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

// NOLINTBEGIN(clang-analyzer-optin.core.EnumCastOutOfRange)
#include "stdafx.h"
#include <array>

#include "DirectAnnotation.h"

#if defined(_MSC_VER)
#  pragma warning(default : ALL_CODE_ANALYSIS_WARNINGS)
#  pragma warning(default : ALL_CPPCORECHECK_WARNINGS)
#endif

namespace {

VARIANT to_VARIANT(bool value)
{
    VARIANT ret{};
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
    ret.vt = VT_BOOL;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
    ret.boolVal = value ? VARIANT_TRUE : VARIANT_FALSE;
    return ret;
}

} // anonymous namespace

DirectAnnotation::DirectAnnotation(HWND window):
    m_window(window)
{
}

DirectAnnotation::DirectAnnotation(HWND window, DWORD objectId):
    m_window(window), m_objectId(objectId)
{
}

// +++++++++++++++++++++++++++++++++++
// Utility.
// +++++++++++++++++++++++++++++++++++

void DirectAnnotation::SetWindow(HWND window)
{
    m_window = window;
}

void DirectAnnotation::SetObjectId(DWORD objectId)
{
    m_objectId = objectId;
}

bool DirectAnnotation::ClearProperty(VARIANT varChild, MSAAPROPID idProp)
{
    if (!CommonPrologue(varChild))
    {
        return false;
    }
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
    auto hr{ClearAccessibilityPropertyWorker(varChild.ulVal, idProp)};
    return SUCCEEDED(hr);

}

// +++++++++++++++++++++++++++++++++++
// Microsoft Active Accessibility.
// +++++++++++++++++++++++++++++++++++

bool DirectAnnotation::SetAccessibleDescription(VARIANT varChild, LPCWSTR description)
{
    if (!CommonPrologue(varChild))
    {
        return false;
    }
    if (description == nullptr || description[0] == 0)
    {
        return false;
    }
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
    auto hr{SetAccessibilityPropertyWorker(varChild.ulVal, PROPID_ACC_DESCRIPTION, description)};
    if (SUCCEEDED(hr))
    {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
        NotifyWinEvent(EVENT_OBJECT_DESCRIPTIONCHANGE, m_window, static_cast<LONG>(m_objectId), varChild.lVal);
    }
    return SUCCEEDED(hr);
}

bool DirectAnnotation::SetAccessibleDescription(VARIANT varChild, UINT nID)
{
    if (nID == 0)
    {
        return false;
    }
    const auto description{Utility::loadString(nID)};
    if (description.empty())
    {
        return false;
    }
    return SetAccessibleDescription(varChild, description.c_str());
}

bool DirectAnnotation::SetAccessibleHelp(VARIANT varChild, LPCWSTR help)
{
    if (!CommonPrologue(varChild))
    {
        return false;
    }
    if (help == nullptr || help[0] == 0)
    {
        return false;
    }
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
    auto hr{SetAccessibilityPropertyWorker(varChild.ulVal, PROPID_ACC_HELP, help)};
    if (SUCCEEDED(hr))
    {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
        NotifyWinEvent(EVENT_OBJECT_HELPCHANGE, m_window, static_cast<LONG>(m_objectId), varChild.lVal);
    }
    return SUCCEEDED(hr);
}

bool DirectAnnotation::SetAccessibleHelp(VARIANT varChild, UINT nID)
{
    if (nID == 0)
    {
        return false;
    }
    const auto help{Utility::loadString(nID)};
    if (help.empty())
    {
        return false;
    }
    return SetAccessibleHelp(varChild, help.c_str());
}

bool DirectAnnotation::SetAccessibleName(VARIANT varChild, LPCWSTR name)
{
    if (!CommonPrologue(varChild))
    {
        return false;
    }
    if (name == nullptr || name[0] == 0)
    {
        return false;
    }
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
    auto hr{SetAccessibilityPropertyWorker(varChild.ulVal, PROPID_ACC_NAME, name)};
    if (SUCCEEDED(hr))
    {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
        NotifyWinEvent(EVENT_OBJECT_NAMECHANGE, m_window, static_cast<LONG>(m_objectId), varChild.lVal);
    }
    return SUCCEEDED(hr);
}

bool DirectAnnotation::SetAccessibleName(VARIANT varChild, UINT nID)
{
    if (nID == 0)
    {
        return false;
    }
    const auto name{Utility::loadString(nID)};
    if (name.empty())
    {
        return false;
    }
    return SetAccessibleName(varChild, name.c_str());
}

bool DirectAnnotation::SetAccessibleRole(VARIANT varChild, LONG role)
{
    if (!CommonPrologue(varChild))
    {
        return false;
    }
    if (role == 0)
    {
        return false;
    }
    _variant_t var{role};
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
    auto hr{SetAccessibilityPropertyWorker(varChild.ulVal, PROPID_ACC_ROLE, var)};
    return SUCCEEDED(hr);
}

bool DirectAnnotation::SetAccessibleState(VARIANT varChild, LONG state, bool overrideMSAA)
{
    if (!CommonPrologue(varChild))
    {
        return false;
    }
    if (state == 0)
    {
        return false;
    }
    _variant_t var{};
    if (overrideMSAA)
    {
        var = state;
    }
    else
    {
        auto hrGetState{get_accState(varChild, &var)};
        if (FAILED(hrGetState))
        {
            return false;
        }
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access,hicpp-signed-bitwise)
        var.lVal |= state;
    }
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
    auto hr{SetAccessibilityPropertyWorker(varChild.ulVal, PROPID_ACC_STATE, var)};
    if (SUCCEEDED(hr))
    {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
        NotifyWinEvent(EVENT_OBJECT_STATECHANGE, m_window, static_cast<LONG>(m_objectId), varChild.lVal);
    }
    return SUCCEEDED(hr);
}

bool DirectAnnotation::SetAccessibleValue(VARIANT varChild, LPCWSTR value)
{
    if (!CommonPrologue(varChild))
    {
        return false;
    }
    if (value == nullptr || value[0] == 0)
    {
        return false;
    }
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
    auto hr{SetAccessibilityPropertyWorker(varChild.ulVal, PROPID_ACC_VALUE, value)};
    if (SUCCEEDED(hr))
    {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
        NotifyWinEvent(EVENT_OBJECT_VALUECHANGE, m_window, static_cast<LONG>(m_objectId), varChild.lVal);
    }
    return SUCCEEDED(hr);
}

bool DirectAnnotation::SetAccessibleValue(VARIANT varChild, UINT nID)
{
    if (nID == 0)
    {
        return false;
    }
    const auto value = Utility::loadString(nID);
    if (value.empty())
    {
        return false;
    }
    return SetAccessibleValue(varChild, value.c_str());
}

// +++++++++++++++++++++++++++++++++++
// UI Automation.
// +++++++++++++++++++++++++++++++++++

bool DirectAnnotation::SetControlTypeProperty(VARIANT varChild, LONG controlType)
{
    if (!CommonPrologue(varChild))
    {
        return false;
    }
    _variant_t var{controlType};
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
    auto hr{SetAccessibilityPropertyWorker(varChild.ulVal, ControlType_Property_GUID, var)};
    return SUCCEEDED(hr);
}

bool DirectAnnotation::SetFrameworkIdProperty(VARIANT varChild, LPCWSTR value)
{
    if (!CommonPrologue(varChild))
    {
        return false;
    }
    if (value == nullptr || value[0] == 0)
    {
        return false;
    }
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
    auto hr{SetAccessibilityPropertyWorker(varChild.ulVal, FrameworkId_Property_GUID, value)};
    return SUCCEEDED(hr);
}

bool DirectAnnotation::SetFrameworkIdProperty(VARIANT varChild, UINT nID)
{
    if (nID == 0)
    {
        return false;
    }
    const auto value{Utility::loadString(nID)};
    if (value.empty())
    {
        return false;
    }
    return SetFrameworkIdProperty(varChild, value.c_str());
}

bool DirectAnnotation::SetFullDescriptionProperty(VARIANT varChild, LPCWSTR description)
{
    if (!CommonPrologue(varChild))
    {
        return false;
    }
    if (description == nullptr || description[0] == 0)
    {
        return false;
    }
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
    auto hr{SetAccessibilityPropertyWorker(varChild.ulVal, FullDescription_Property_GUID, description)};
    if (SUCCEEDED(hr))
    {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
        NotifyWinEvent(EVENT_OBJECT_DESCRIPTIONCHANGE, m_window, static_cast<LONG>(m_objectId), varChild.lVal);
    }
    return SUCCEEDED(hr);
}

bool DirectAnnotation::SetFullDescriptionProperty(VARIANT varChild, UINT nID)
{
    if (nID == 0)
    {
        return false;
    }
    const auto description{Utility::loadString(nID)};
    if (description.empty())
    {
        return false;
    }
    return SetFullDescriptionProperty(varChild, description.c_str());
}

// NOLINTNEXTLINE(google-runtime-int)
bool DirectAnnotation::SetHeadingLevelProperty(VARIANT varChild, long headingLevel)
{
    if (!CommonPrologue(varChild))
    {
        return false;
    }
    if (headingLevel < HeadingLevel_None || headingLevel > HeadingLevel9)
    {
        return false;
    }
    variant_t var(headingLevel);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
    auto hr{SetAccessibilityPropertyWorker(varChild.ulVal, HeadingLevel_Property_GUID, var)};
    return SUCCEEDED(hr);
}

bool DirectAnnotation::SetHelpTextProperty(VARIANT varChild, LPCWSTR helpText)
{
    if (!CommonPrologue(varChild))
    {
        return false;
    }
    if (helpText == nullptr || helpText[0] == 0)
    {
        return false;
    }
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
    auto hr{SetAccessibilityPropertyWorker(varChild.ulVal, HelpText_Property_GUID, helpText)};
    if (SUCCEEDED(hr))
    {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
        NotifyWinEvent(EVENT_OBJECT_HELPCHANGE, m_window, static_cast<LONG>(m_objectId), varChild.lVal);
    }
    return SUCCEEDED(hr);
}

bool DirectAnnotation::SetHelpTextProperty(VARIANT varChild, UINT nID)
{
    if (nID == 0)
    {
        return false;
    }
    const auto helpText{Utility::loadString(nID)};
    if (helpText.empty())
    {
        return false;
    }
    return SetHelpTextProperty(varChild, helpText.c_str());
}


bool DirectAnnotation::SetIsContentElementProperty(VARIANT varChild, bool value)
{
    if (!CommonPrologue(varChild))
    {
        return false;
    }
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
    auto hr{SetAccessibilityPropertyWorker(varChild.ulVal, IsContentElement_Property_GUID, value)};
    return SUCCEEDED(hr);
}


bool DirectAnnotation::SetIsControlElementProperty(VARIANT varChild, bool value)
{
    if (!CommonPrologue(varChild))
    {
        return false;
    }
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
    auto hr{SetAccessibilityPropertyWorker(varChild.ulVal, IsControlElement_Property_GUID, value)};
    return SUCCEEDED(hr);
}

bool DirectAnnotation::SetIsDataValidForFormProperty(VARIANT varChild, bool value)
{
    if (!CommonPrologue(varChild))
    {
        return false;
    }
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
    auto hr{SetAccessibilityPropertyWorker(varChild.ulVal, IsDataValidForForm_Property_GUID, value)};
    return SUCCEEDED(hr);
}

bool DirectAnnotation::SetIsDialogProperty(VARIANT varChild, bool value)
{
    if (!CommonPrologue(varChild))
    {
        return false;
    }
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
    auto hr{SetAccessibilityPropertyWorker(varChild.ulVal, IsDialog_Property_GUID, value)};
    return SUCCEEDED(hr);
}

bool DirectAnnotation::SetIsPasswordProperty(VARIANT varChild, bool value)
{
    if (!CommonPrologue(varChild))
    {
        return false;
    }
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
    auto hr{SetAccessibilityPropertyWorker(varChild.ulVal, IsPassword_Property_GUID, value)};
    return SUCCEEDED(hr);
}

bool DirectAnnotation::SetIsRequiredForFormProperty(VARIANT varChild, bool value)
{
    if (!CommonPrologue(varChild))
    {
        return false;
    }
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
    auto hr{SetAccessibilityPropertyWorker(varChild.ulVal, IsRequiredForForm_Property_GUID, value)};
    return SUCCEEDED(hr);
}

bool DirectAnnotation::SetItemStatusProperty(VARIANT varChild, LPCWSTR itemStatus)
{
    if (!CommonPrologue(varChild))
    {
        return false;
    }
    if (itemStatus == nullptr || itemStatus[0] == 0)
    {
        return false;
    }
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
    auto hr{SetAccessibilityPropertyWorker(varChild.ulVal, ItemStatus_Property_GUID, itemStatus)};
    return SUCCEEDED(hr);
}

bool DirectAnnotation::SetItemStatusProperty(VARIANT varChild, UINT nID)
{
    if (nID == 0)
    {
        return false;
    }
    const auto itemStatus{Utility::loadString(nID)};
    if (itemStatus.empty())
    {
        return false;
    }
    return SetItemStatusProperty(varChild, itemStatus.c_str());
}

bool DirectAnnotation::SetItemTypeProperty(VARIANT varChild, LPCWSTR itemType)
{
    if (!CommonPrologue(varChild))
    {
        return false;
    }
    if (itemType == nullptr || itemType[0] == 0)
    {
        return false;
    }
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
    auto hr{SetAccessibilityPropertyWorker(varChild.ulVal, ItemType_Property_GUID, itemType)};
    return SUCCEEDED(hr);
}

bool DirectAnnotation::SetItemTypeProperty(VARIANT varChild, UINT nID)
{
    if (nID == 0)
    {
        return false;
    }
    const auto itemType{Utility::loadString(nID)};
    if (itemType.empty())
    {
        return false;
    }
    return SetItemTypeProperty(varChild, itemType.c_str());
}

// NOLINTNEXTLINE(google-runtime-int)
bool DirectAnnotation::SetLevelProperty(VARIANT varChild, long level)
{
    if (!CommonPrologue(varChild))
    {
        return false;
    }
    _variant_t var{level};
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
    auto hr{SetAccessibilityPropertyWorker(varChild.ulVal, Level_Property_GUID, var)};
    return SUCCEEDED(hr);
}

bool DirectAnnotation::SetLocalizedControlTypeProperty(VARIANT varChild, LPCWSTR name)
{
    if (!CommonPrologue(varChild))
    {
        return false;
    }
    if (name == nullptr || name[0] == 0)
    {
        return false;
    }
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
    auto hr{SetAccessibilityPropertyWorker(varChild.ulVal, LocalizedControlType_Property_GUID, name)};
    return SUCCEEDED(hr);
}

bool DirectAnnotation::SetLocalizedControlTypeProperty(VARIANT varChild, UINT nID)
{
    if (nID == 0)
    {
        return false;
    }
    const auto type{Utility::loadString(nID)};
    if (type.empty())
    {
        return false;
    }
    return SetLocalizedControlTypeProperty(varChild, type.c_str());
}

bool DirectAnnotation::SetNameProperty(VARIANT varChild, LPCWSTR name)
{
    if (!CommonPrologue(varChild))
    {
        return false;
    }
    if (name == nullptr || name[0] == 0)
    {
        return false;
    }
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
    auto hr{SetAccessibilityPropertyWorker(varChild.ulVal, Name_Property_GUID, name)};
    if (SUCCEEDED(hr))
    {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
        NotifyWinEvent(EVENT_OBJECT_NAMECHANGE, m_window, static_cast<LONG>(m_objectId), varChild.lVal);
    }
    return SUCCEEDED(hr);
}

bool DirectAnnotation::SetNameProperty(VARIANT varChild, UINT nID)
{
    if (nID == 0)
    {
        return false;
    }
    const auto name{Utility::loadString(nID)};
    if (name.empty())
    {
        return false;
    }
    return SetNameProperty(varChild, name.c_str());
}

// NOLINTNEXTLINE(google-runtime-int)
bool DirectAnnotation::SetPositionInSetProperty(VARIANT varChild, long positionInSet)
{
    if (!CommonPrologue(varChild))
    {
        return false;
    }
    _variant_t var{positionInSet};
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
    auto hr{SetAccessibilityPropertyWorker(varChild.ulVal, PositionInSet_Property_GUID, var)};
    return SUCCEEDED(hr);
}

// NOLINTNEXTLINE(google-runtime-int)
bool DirectAnnotation::SetSizeOfSetProperty(VARIANT varChild, long sizeOfSet)
{
    if (!CommonPrologue(varChild))
    {
        return false;
    }
    _variant_t var{sizeOfSet};
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
    auto hr{SetAccessibilityPropertyWorker(varChild.ulVal, SizeOfSet_Property_GUID, var)};
    return SUCCEEDED(hr);
}


// +++++++++++++++++++++++++++++++++++
// Protected methods.
// +++++++++++++++++++++++++++++++++++

bool DirectAnnotation::InitAccPropServices()
{
    if (m_pAccPropServices)
    {
        return true;
    }
    if (m_initAccPropServicesFailed)
    {
        return false;
    }
    // NOLINTNEXTLINE(hicpp-signed-bitwise,clang-analyzer-optin.core.EnumCastOutOfRange)
    auto hr{m_pAccPropServices.CreateInstance(CLSID_AccPropServices, nullptr, CLSCTX_SERVER)};
    if (FAILED(hr) || !m_pAccPropServices)
    {
        m_initAccPropServicesFailed = false;
        return false;
    }
    return true;
}

bool DirectAnnotation::CommonPrologue(VARIANT varChild)
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
    if (varChild.vt != VT_I4 || m_window == nullptr)
    {
        return false;
    }
    return InitAccPropServices();
}

HRESULT DirectAnnotation::get_accState(VARIANT varChild, VARIANT* pvarState)
{
    IAccessiblePtr pAccessible{nullptr};
    auto hr{AccessibleObjectFromWindow(m_window, m_objectId, __uuidof(IAccessible),
        reinterpret_cast<void**>(&pAccessible))};
    if (FAILED(hr))
    {
        return hr;
    }
    if (pAccessible == nullptr)
    {
        return E_POINTER;
    }
    hr = pAccessible->get_accState(varChild, pvarState);
    return hr;
}

HRESULT DirectAnnotation::ClearAccessibilityPropertyWorker(DWORD idChild, MSAAPROPID idProp)
{
    if (!m_pAccPropServices)
    {
        return E_POINTER;
    }
    std::array<MSAAPROPID, 1> props{idProp};
    auto hr{m_pAccPropServices->ClearHwndProps(m_window, m_objectId, idChild, props.data(),
        static_cast<int>(props.size()))};
    return hr;
}

HRESULT DirectAnnotation::SetAccessibilityPropertyWorker(DWORD idChild, MSAAPROPID idProp, LPCWSTR value)
{
    if (!m_pAccPropServices)
    {
        return E_POINTER;
    }
    auto hr{m_pAccPropServices->SetHwndPropStr(m_window, m_objectId, idChild, idProp, value)};
    return hr;
}

HRESULT DirectAnnotation::SetAccessibilityPropertyWorker(DWORD idChild, MSAAPROPID idProp, VARIANT value)
{
    if (!m_pAccPropServices)
    {
        return E_POINTER;
    }
    auto hr{m_pAccPropServices->SetHwndProp(m_window, m_objectId, idChild, idProp, value)};
    return hr;
}

HRESULT DirectAnnotation::SetAccessibilityPropertyWorker(DWORD idChild, MSAAPROPID idProp, bool value)
{
    return SetAccessibilityPropertyWorker(idChild, idProp, to_VARIANT(value));
}
// NOLINTEND(clang-analyzer-optin.core.EnumCastOutOfRange)

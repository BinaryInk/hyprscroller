#include <hyprland/src/config/ConfigManager.hpp>
#include <hyprland/src/plugins/PluginAPI.hpp>
#include <hyprlang.hpp>

#include "dispatchers.h"
#include "scroller.h"

HANDLE PHANDLE = nullptr;
std::unique_ptr<ScrollerLayout> g_ScrollerLayout;

APICALL EXPORT std::string PLUGIN_API_VERSION() {
    return HYPRLAND_API_VERSION;
}

APICALL EXPORT PLUGIN_DESCRIPTION_INFO PLUGIN_INIT(HANDLE handle) {
    PHANDLE = handle;

    const std::string HASH = __hyprland_api_get_hash();

    if (HASH != GIT_COMMIT_HASH) {
        HyprlandAPI::addNotification(PHANDLE, "[hyprscroller] Failure in initialization: Version mismatch (headers ver is not equal to running hyprland ver)",
                                     CColor{1.0, 0.2, 0.2, 1.0}, 5000);
        throw std::runtime_error("[hyprscroller] Version mismatch");
    }

#ifdef COLORS_IPC
    HyprlandAPI::addConfigValue(PHANDLE, "plugin:scroller:col.freecolumn_border", Hyprlang::CConfigValue(Hyprlang::INT(0xff9e1515)));
#endif
    g_ScrollerLayout = std::make_unique<ScrollerLayout>();
    HyprlandAPI::addLayout(PHANDLE, "scroller", g_ScrollerLayout.get());

    dispatchers::addDispatchers();

    static auto P1 = HyprlandAPI::registerCallbackDynamic(PHANDLE, "workspace", [](void* self, SCallbackInfo& info, std::any param) {
        if (!g_ScrollerLayout || !g_ScrollerLayout->is_enabled())
            return;
        auto WORKSPACE = std::any_cast<PHLWORKSPACE>(param);
        g_ScrollerLayout->post_event(WORKSPACE->m_iID, "mode");
        g_ScrollerLayout->post_event(WORKSPACE->m_iID, "overview");
    });
    static auto P2 = HyprlandAPI::registerCallbackDynamic(PHANDLE, "focusedMon", [](void* self, SCallbackInfo& info, std::any param) {
        if (!g_ScrollerLayout || !g_ScrollerLayout->is_enabled())
            return;
        auto monitor = std::any_cast<PHLMONITOR>(param);
        g_ScrollerLayout->post_event(monitor->activeWorkspaceID(), "mode");
        g_ScrollerLayout->post_event(monitor->activeWorkspaceID(), "overview");
    });

    static auto P3 = HyprlandAPI::registerCallbackDynamic(PHANDLE, "swipeBegin", [](void* self, SCallbackInfo& info, std::any param) {
        if (!g_ScrollerLayout || !g_ScrollerLayout->is_enabled())
            return;
        auto swipe_event = std::any_cast<IPointer::SSwipeBeginEvent>(param);
        g_ScrollerLayout->swipe_begin(swipe_event);
    });

    static auto P4 = HyprlandAPI::registerCallbackDynamic(PHANDLE, "swipeUpdate", [](void* self, SCallbackInfo& info, std::any param) {
        if (!g_ScrollerLayout || !g_ScrollerLayout->is_enabled())
            return;
        auto swipe_event = std::any_cast<IPointer::SSwipeUpdateEvent>(param);
        g_ScrollerLayout->swipe_update(info, swipe_event);
    });

    static auto P5 = HyprlandAPI::registerCallbackDynamic(PHANDLE, "swipeEnd", [](void* self, SCallbackInfo& info, std::any param) {
        if (!g_ScrollerLayout || !g_ScrollerLayout->is_enabled())
            return;
        auto swipe_event = std::any_cast<IPointer::SSwipeEndEvent>(param);
        g_ScrollerLayout->swipe_end(info, swipe_event);
    });

    // one value out of: { onesixth, onefourth, onethird, onehalf (default), twothirds, floating, maximized }
    HyprlandAPI::addConfigValue(PHANDLE, "plugin:scroller:column_default_width", Hyprlang::STRING{"onehalf"});
    // one value out of: { onesixth, onefourth, onethird, onehalf, twothirds, one (default) }
    HyprlandAPI::addConfigValue(PHANDLE, "plugin:scroller:window_default_height", Hyprlang::STRING{"one"});
    // 0, 1
    HyprlandAPI::addConfigValue(PHANDLE, "plugin:scroller:focus_wrap", Hyprlang::INT{1});
    // 0, 1
    HyprlandAPI::addConfigValue(PHANDLE, "plugin:scroller:overview_scale_content", Hyprlang::INT{1});
    // a list of values used as standard widths for cyclesize in row mode, and in the cycling order
    // available options: onesixth, onefourth, onethird, onehalf, twothirds, one
    // default: onehalf twothirds onethird
    HyprlandAPI::addConfigValue(PHANDLE, "plugin:scroller:column_widths", Hyprlang::STRING{"onehalf twothirds onethird"});
    // a list of values used as standard heights for cyclesize in column mode, and in the cycling order
    // available options: onesixth, onefourth, onethird, onehalf, twothirds, one
    // default: one onethird onehalf twothirds
    HyprlandAPI::addConfigValue(PHANDLE, "plugin:scroller:window_heights", Hyprlang::STRING{"one onethird onehalf twothirds"});
    // a list of values separated by ','. Each value is of the type MONITORID = ( options ),
    // where MONITORID is the name of a monitor;
    // options is a list of key = value separated by ';'.
    // Accepted keys are mode, column_default_width and window_default_height
    // default: empty, which means every monitor will get "row" mode and the default values
    // for the rest of the keys
    // monitor_options = ( DP-2 = ( mode = col; column_default_width = onethird; window_default_height = onehalf), HDMI-A-1 = ())
    HyprlandAPI::addConfigValue(PHANDLE, "plugin:scroller:monitor_options", Hyprlang::STRING{""});
    //Enable gestures
    HyprlandAPI::addConfigValue(PHANDLE, "plugin:scroller:gesture_overview_enable", Hyprlang::INT{1});
    HyprlandAPI::addConfigValue(PHANDLE, "plugin:scroller:gesture_overview_distance", Hyprlang::INT{5});
    HyprlandAPI::addConfigValue(PHANDLE, "plugin:scroller:gesture_overview_fingers", Hyprlang::INT{4});
    HyprlandAPI::addConfigValue(PHANDLE, "plugin:scroller:gesture_scroll_enable", Hyprlang::INT{1});
    HyprlandAPI::addConfigValue(PHANDLE, "plugin:scroller:gesture_scroll_distance", Hyprlang::INT{60});
    HyprlandAPI::addConfigValue(PHANDLE, "plugin:scroller:gesture_scroll_fingers", Hyprlang::INT{3});

    HyprlandAPI::reloadConfig();

    return {"hyprscroller", "scrolling window layout", "dawser", "1.0"};
}

APICALL EXPORT void PLUGIN_EXIT() {}

#include "stdafx.h"

#include "../corelib/collection_helper.h"

#include "app_config.h"

UI_NS_BEGIN

namespace
{
    AppConfigUptr AppConfig_;
}

AppConfig::AppConfig(const core::coll_helper &collection)
{
    IsServerHistoryEnabled_ = collection.get<bool>("history.is_server_history_enabled");
    IsContextMenuFeaturesUnlocked_ = collection.get<bool>("dev.unlock_context_menu_features");
    isCrashEnable_ = collection.get<bool>("enable_crash");
}

bool AppConfig::IsContextMenuFeaturesUnlocked() const
{
    return IsContextMenuFeaturesUnlocked_;
}

bool AppConfig::IsServerHistoryEnabled() const
{
    return IsServerHistoryEnabled_;
}

bool AppConfig::isCrashEnable() const
{
    return isCrashEnable_;
}

const AppConfig& GetAppConfig()
{
    assert(AppConfig_);

    return *AppConfig_;
}

void SetAppConfig(AppConfigUptr &appConfig)
{
    assert(!AppConfig_);
    assert(appConfig);

    AppConfig_ = std::move(appConfig);
}

UI_NS_END
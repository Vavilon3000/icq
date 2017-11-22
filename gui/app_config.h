#pragma once

#include "namespaces.h"

CORE_NS_BEGIN

class coll_helper;

CORE_NS_END

UI_NS_BEGIN

class AppConfig
{
public:
    AppConfig(const core::coll_helper &collection);

    bool IsContextMenuFeaturesUnlocked() const;

    bool IsServerHistoryEnabled() const;

    bool isCrashEnable() const;

private:
    bool IsContextMenuFeaturesUnlocked_;

    bool IsServerHistoryEnabled_;

    bool isCrashEnable_;

};

typedef std::unique_ptr<AppConfig> AppConfigUptr;

const AppConfig& GetAppConfig();

void SetAppConfig(AppConfigUptr &appConfig);

UI_NS_END
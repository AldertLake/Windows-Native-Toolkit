// -----------------------------------------------------
// Copyright   (c) 2025 AldertLake. All Rights Reserved.
// GitHub:     https://github.com/AldertLake/
// Discord:    https://discord.gg/QpPPfh6WVn
// -----------------------------------------------------

#pragma once
#include "Modules/ModuleInterface.h"

class FNetworkUtilityModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;

    virtual void ShutdownModule() override;
};


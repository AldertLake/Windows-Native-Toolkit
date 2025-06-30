// ---------------------------------------------------
// Copyright (c) 2025 AldertLake. All Rights Reserved.
// GitHub:   https://github.com/AldertLake/
// Support:  https://ko-fi.com/aldertlake
// ---------------------------------------------------

#pragma once
#include "Modules/ModuleInterface.h"

class FDeviceFrameworkModule : public IModuleInterface
{
public:
    /** Called when the module is loaded */
    virtual void StartupModule() override;

    /** Called before the module is unloaded */
    virtual void ShutdownModule() override;
};

// ---------------------------------------------------
// Copyright (c) 2025 AldertLake. All Rights Reserved.
// GitHub:   https://github.com/AldertLake/
// Support:  https://ko-fi.com/aldertlake
// ---------------------------------------------------

#pragma once
#include "Modules/ModuleInterface.h"

class FSystemUtilityModule : public IModuleInterface
{
public:
    /** Called when the module is loaded into memory */
    virtual void StartupModule() override;

    /** Called before the module is unloaded */
    virtual void ShutdownModule() override;
};

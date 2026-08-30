#pragma once

#include "Modules/ModuleManager.h"

class FResonanceForgeWwiseModule final : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};

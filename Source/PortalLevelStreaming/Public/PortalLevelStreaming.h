#pragma once

#include <CoreMinimal.h>
#include <Modules/ModuleManager.h>

class FPortalLevelStreamingModule final : public IModuleInterface
{
public:
    void StartupModule() override;
    void ShutdownModule() override;
};

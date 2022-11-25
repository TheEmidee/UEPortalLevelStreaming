#pragma once

#include <CoreMinimal.h>
#include <Kismet/BlueprintAsyncActionBase.h>

#include "PLSAsyncActionWaitAllRequestsFinished.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE( FPLSOnAllRequestsFinishedDynamicDelegate );

UCLASS()
class PORTALLEVELSTREAMING_API UPLSAsyncActionWaitAllRequestsFinished final : public UBlueprintAsyncActionBase
{
    GENERATED_BODY()

public:
    // Waits for the experience to be determined and loaded
    UFUNCTION( BlueprintCallable, meta = ( WorldContext = "world_context_object", BlueprintInternalUseOnly = "true" ) )
    static UPLSAsyncActionWaitAllRequestsFinished * WaitForAllPortalLevelStreaminRequestsToBeFinished( UObject * world_context_object );

    void Activate() override;

protected:
    // Called when the experience has been determined and is ready/loaded
    UPROPERTY( BlueprintAssignable )
    FPLSOnAllRequestsFinishedDynamicDelegate OnAllRequestsFinishedDelegate;

private:
    void OnAllRequestsFinished();

    TWeakObjectPtr< UWorld > WorldPtr;
};

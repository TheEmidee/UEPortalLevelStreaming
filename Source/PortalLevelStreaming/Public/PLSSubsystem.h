#pragma once

#include "PLSRequest.h"

#include <CoreMinimal.h>
#include <Subsystems/WorldSubsystem.h>

#include "PLSSubsystem.generated.h"

class UPLSRequest;
class UPLSLevelGroup;
class ULevelStreaming;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam( FPLSOnRequestExecutedDynamicMulticastDelegate, FPLSLevelStreamingRequestHandle, handle );
DECLARE_MULTICAST_DELEGATE( FPLSOnAllRequestsFinishedDelegate );

UCLASS()
class PORTALLEVELSTREAMING_API UPLSSubsystem final : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    FPLSOnRequestExecutedDynamicMulticastDelegate & OnRequestExecuted();

    UFUNCTION( BlueprintCallable, BlueprintAuthorityOnly, meta = ( DisplayName = "Add Streaming Request", AutoCreateRefTerm = "request_executed_delegate" ) )
    FPLSLevelStreamingRequestHandle K2_AddRequest( const FPLSLevelStreamingInfos & infos, const FPLSOnRequestExecutedDynamicDelegate & request_executed_delegate, bool cancel_existing_requests = false );

    FPLSLevelStreamingRequestHandle AddRequest( const FPLSLevelStreamingInfos & infos, FPLSOnRequestExecutedDelegate request_executed_delegate = FPLSOnRequestExecutedDelegate(), bool cancel_existing_requests = false );

    TOptional< FPLSLevelStreamingInfos > GetRequestInfos( FPLSLevelStreamingRequestHandle request_handle ) const;

    void CallOrRegister_OnAllRequestsFinished( FPLSOnAllRequestsFinishedDelegate::FDelegate delegate );

private:
    void OnRequestExecuted( FPLSLevelStreamingRequestHandle handle );
    void ProcessNextRequest();

    UPROPERTY()
    TArray< UPLSRequest * > Requests;

    TMap< FPLSLevelStreamingRequestHandle, FPLSLevelStreamingInfos > RequestHandleToInfosMap;
    FPLSOnRequestExecutedDynamicMulticastDelegate OnRequestExecutedDelegate;
    FPLSOnAllRequestsFinishedDelegate OnAllRequestsFinishedDelegate;
};

FORCEINLINE FPLSOnRequestExecutedDynamicMulticastDelegate & UPLSSubsystem::OnRequestExecuted()
{
    return OnRequestExecutedDelegate;
}
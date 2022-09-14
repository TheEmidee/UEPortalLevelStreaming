#pragma once

#include "PLSTypes.h"

#include <CoreMinimal.h>

#include "PLSRequest.generated.h"

class ULevelStreaming;

USTRUCT( BlueprintType )
struct FPLSLevelStreamingRequestHandle
{
    GENERATED_USTRUCT_BODY()

    FPLSLevelStreamingRequestHandle() :
        Handle( INDEX_NONE )
    {
    }

    /** True if GenerateNewHandle was called on this handle */
    bool IsValid() const
    {
        return Handle != INDEX_NONE;
    }

    /** Sets this to a valid handle */
    void GenerateNewHandle()
    {
        static int32 GHandle = 0;
        Handle = ++GHandle;
    }

    bool operator==( const FPLSLevelStreamingRequestHandle & Other ) const
    {
        return Handle == Other.Handle;
    }

    bool operator!=( const FPLSLevelStreamingRequestHandle & Other ) const
    {
        return Handle != Other.Handle;
    }

    friend uint32 GetTypeHash( const FPLSLevelStreamingRequestHandle & Handle )
    {
        return ::GetTypeHash( Handle.Handle );
    }

    FString ToString() const
    {
        return IsValid() ? FString::FromInt( Handle ) : TEXT( "Invalid" );
    }

private:
    UPROPERTY()
    int32 Handle;
};

struct FUnloadLevelInfos
{
    FUnloadLevelInfos( const uint8 block_on_unload, const EPLSLevelStreamingUnloadType unload_type ) :
        bBlockOnUnload( block_on_unload ),
        UnloadType( unload_type )
    {
    }

    uint8 bBlockOnUnload : 1;
    EPLSLevelStreamingUnloadType UnloadType;
};

struct FLoadLevelInfos
{
    FLoadLevelInfos( const uint8 block_on_load, const EPLSLevelStreamingLoadType load_type ) :
        bBlockOnLoad( block_on_load ),
        LoadType( load_type )
    {
    }

    uint8 bBlockOnLoad : 1;
    EPLSLevelStreamingLoadType LoadType;
};

DECLARE_DELEGATE_OneParam( FPLSOnRequestExecutedDelegate, FPLSLevelStreamingRequestHandle handle );
DECLARE_DYNAMIC_DELEGATE_OneParam( FPLSOnRequestExecutedDynamicDelegate, FPLSLevelStreamingRequestHandle, handle );

UCLASS()
class UPLSRequest : public UObject
{
    GENERATED_BODY()

public:
    FPLSLevelStreamingRequestHandle GetHandle() const;
    bool IsExecuting() const;

    void Initialize( const FPLSLevelStreamingInfos & infos, const FPLSOnRequestExecutedDelegate & on_request_executed );
    void Cancel();
    void Process();
    UWorld * GetWorld() const override;

private:
    ULevelStreaming * FindLevelStreaming( const FSoftObjectPath & soft_object_path ) const;
    void UnloadLevels( bool load_levels_when_finished );
    void LoadLevels( bool unload_levels_when_finished );

    UFUNCTION()
    void OnLevelStreamingUnloaded();

    UFUNCTION()
    void OnLevelStreamingLoadedOrVisible();

    void BroadcastExecutedEvent() const;
    void UnbindLevelStreamingEvents() const;

    TMap< ULevelStreaming *, FUnloadLevelInfos > LevelsToUnloadMap;
    TMap< ULevelStreaming *, FLoadLevelInfos > LevelsToLoadMap;
    int LevelToUnloadCount;
    int LevelToLoadCount;
    EPLSLoadOrder LoadOrder;
    FPLSLevelStreamingRequestHandle Handle;
    FPLSOnRequestExecutedDelegate OnRequestExecutedDelegate;
};

FORCEINLINE FPLSLevelStreamingRequestHandle UPLSRequest::GetHandle() const
{
    return Handle;
}

FORCEINLINE bool UPLSRequest::IsExecuting() const
{
    return LevelToLoadCount + LevelToUnloadCount > 0;
}
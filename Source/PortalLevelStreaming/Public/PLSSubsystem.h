#pragma once

#include <CoreMinimal.h>
#include <Subsystems/WorldSubsystem.h>

#include "PLSSubsystem.generated.h"

class UPLSLevelGroup;
class ULevelStreaming;

UENUM()
enum class EPLSLoadOrder : uint8
{
    UnloadThenLoad,
    LoadThenUnload,
    UnloadAndLoadInParallel
};

USTRUCT( BlueprintType )
struct PORTALLEVELSTREAMING_API FPLSLevelStreamingLevelInfos
{
    GENERATED_USTRUCT_BODY()

    UPROPERTY( EditAnywhere, meta = ( AllowedClasses = "World" ) )
    TArray< FSoftObjectPath > IndividualLevels;

    UPROPERTY( EditAnywhere )
    TArray< UPLSLevelGroup * > LevelGroups;
};

UENUM()
enum class EPLSLevelStreamingLoadType : uint8
{
    Load,
    LoadAndMakeVisible
};

USTRUCT( BlueprintType )
struct PORTALLEVELSTREAMING_API FPLSLevelStreamingLevelToLoadInfos
{
    GENERATED_USTRUCT_BODY()

    FPLSLevelStreamingLevelToLoadInfos() :
        bBlockOnLoad( false ),
        LoadType( EPLSLevelStreamingLoadType::LoadAndMakeVisible )
    {
    }

    UPROPERTY( EditAnywhere )
    FPLSLevelStreamingLevelInfos Levels;

    UPROPERTY( EditAnywhere )
    uint8 bBlockOnLoad : 1;

    UPROPERTY( EditAnywhere )
    EPLSLevelStreamingLoadType LoadType;
};

UENUM()
enum class EPLSLevelStreamingUnloadType : uint8
{
    Hide,
    HideAndUnload
};

UENUM()
enum class EPLSLevelStreamingAlwaysLoadedLevelsUnloadType : uint8
{
    Hide,
    Nothing
};

USTRUCT( BlueprintType )
struct PORTALLEVELSTREAMING_API FPLSLevelStreamingLevelToUnloadInfos
{
    GENERATED_USTRUCT_BODY()

    FPLSLevelStreamingLevelToUnloadInfos() :
        bBlockOnUnload( false ),
        UnloadType( EPLSLevelStreamingUnloadType::HideAndUnload )
    {
    }

    UPROPERTY( EditAnywhere )
    FPLSLevelStreamingLevelInfos Levels;

    UPROPERTY( EditAnywhere )
    uint8 bBlockOnUnload : 1;

    UPROPERTY( EditAnywhere )
    EPLSLevelStreamingUnloadType UnloadType;
};

USTRUCT( BlueprintType )
struct PORTALLEVELSTREAMING_API FPLSUnloadCurrentStreamingLevelInfos
{
    GENERATED_USTRUCT_BODY()

    FPLSUnloadCurrentStreamingLevelInfos() :
        bUnloadCurrentlyLoadedStreamingLevels( true ),
        bBlockOnUnload( false ),
        UnloadType( EPLSLevelStreamingUnloadType::HideAndUnload )
    {}

    UPROPERTY( EditAnywhere )
    uint8 bUnloadCurrentlyLoadedStreamingLevels : 1;

    UPROPERTY( EditAnywhere, meta = ( EditCondition = "bUnloadCurrentlyLoadedStreamingLevels" ) )
    uint8 bBlockOnUnload : 1;

    UPROPERTY( EditAnywhere, meta = ( EditCondition = "bUnloadCurrentlyLoadedStreamingLevels" ) )
    EPLSLevelStreamingUnloadType UnloadType;
};

USTRUCT( BlueprintType )
struct PORTALLEVELSTREAMING_API FPLSLevelStreamingInfos
{
    GENERATED_USTRUCT_BODY()

    FPLSLevelStreamingInfos() :
        LoadOrder( EPLSLoadOrder::UnloadThenLoad ),
        AlwaysLoadedLevelsUnloadType( EPLSLevelStreamingAlwaysLoadedLevelsUnloadType::Nothing )
    {
    }

    UPROPERTY( EditAnywhere )
    TArray< FPLSLevelStreamingLevelToLoadInfos > LevelsToLoad;

    UPROPERTY( EditAnywhere, meta = ( EditCondition = "UnloadCurrentStreamingLevelsInfos.bUnloadCurrentlyLoadedStreamingLevels == false" ) )
    TArray< FPLSLevelStreamingLevelToUnloadInfos > LevelsToUnload;

    UPROPERTY( EditAnywhere )
    EPLSLoadOrder LoadOrder;

    UPROPERTY( EditAnywhere )
    EPLSLevelStreamingAlwaysLoadedLevelsUnloadType AlwaysLoadedLevelsUnloadType;

    UPROPERTY( EditAnywhere )
    FPLSUnloadCurrentStreamingLevelInfos UnloadCurrentStreamingLevelsInfos;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam( FPLSOnStreamedLevelsRequestFinished, const FPLSLevelStreamingInfos &, infos );

UCLASS()
class PORTALLEVELSTREAMING_API UPLSSubsystem final : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    FPLSOnStreamedLevelsRequestFinished & GetOnRequestFinishedDelegate();
    const FPLSLevelStreamingInfos & GetCurrentLevelStreamingInfos();

    UPLSSubsystem();

    UFUNCTION( BlueprintCallable )
    void UpdateStreamedLevels( const FPLSLevelStreamingInfos & infos );

private:
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

    ULevelStreaming * FindLevelStreaming( const FSoftObjectPath & soft_object_path ) const;
    void FinishProcessingRequest();
    void UnloadLevels( bool load_levels_when_finished );
    void LoadLevels( bool unload_levels_when_finished );

    UFUNCTION()
    void OnLevelStreamingUnloaded();

    UFUNCTION()
    void OnLevelStreamingLoadedOrVisible();

    UPROPERTY( BlueprintAssignable )
    FPLSOnStreamedLevelsRequestFinished OnRequestFinishedDelegate;

    UPROPERTY()
    FPLSLevelStreamingInfos CurrentLevelStreamingInfos;

    TMap< ULevelStreaming *, FUnloadLevelInfos > LevelsToUnloadMap;
    TMap< ULevelStreaming *, FLoadLevelInfos > LevelsToLoadMap;

    int LevelToUnloadCount;
    int LevelToLoadCount;
    uint8 bIsProcessingRequest : 1;
};

FORCEINLINE FPLSOnStreamedLevelsRequestFinished & UPLSSubsystem::GetOnRequestFinishedDelegate()
{
    return OnRequestFinishedDelegate;
}

FORCEINLINE const FPLSLevelStreamingInfos & UPLSSubsystem::GetCurrentLevelStreamingInfos()
{
    return CurrentLevelStreamingInfos;
}

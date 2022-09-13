#pragma once

#include <CoreMinimal.h>
#include <Engine/DataAsset.h>

#include "PLSTypes.generated.h"

UCLASS()
class PORTALLEVELSTREAMING_API UPLSLevelGroup final : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY( EditDefaultsOnly, BlueprintReadWrite, meta = ( AllowedClasses = "World" ) )
    TArray< FSoftObjectPath > Levels;
};

UENUM()
enum class EPLSLoadOrder : uint8
{
    UnloadThenLoad,
    LoadThenUnload
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
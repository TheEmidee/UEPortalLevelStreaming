#include "PLSSubsystem.h"

#include "PLSLevelGroup.h"

#include <Engine/LevelStreaming.h>
#include <Kismet/GameplayStatics.h>

UPLSSubsystem::UPLSSubsystem() :
    LevelToUnloadCount( 0 ),
    LevelToLoadCount( 0 ),
    bIsProcessingRequest( false )
{
}

void UPLSSubsystem::UpdateStreamedLevels( const FPLSLevelStreamingInfos & infos )
{
    UpdateStreamedLevelsWithCallback( infos, FPLSOnStreamedLevelsRequestFinished::FDelegate() );
}

void UPLSSubsystem::UpdateStreamedLevelsWithCallback( const FPLSLevelStreamingInfos & infos, FPLSOnStreamedLevelsRequestFinished::FDelegate delegate )
{
    if ( bIsProcessingRequest )
    {
        return;
    }

    OnRequestFinishedDelegate = delegate;

    bIsProcessingRequest = true;

    const auto & streaming_levels = GetWorld()->GetStreamingLevels();

    for ( auto * level_streaming : streaming_levels )
    {
        level_streaming->OnLevelShown.RemoveAll( this );
        level_streaming->OnLevelHidden.RemoveAll( this );
        level_streaming->OnLevelLoaded.RemoveAll( this );
    }

    const auto add_level_to_unload = [ this, &infos ]( ULevelStreaming * level_streaming, const auto & params ) {
        if ( level_streaming->ShouldBeAlwaysLoaded() && infos.AlwaysLoadedLevelsUnloadType == EPLSLevelStreamingAlwaysLoadedLevelsUnloadType::Nothing )
        {
            return;
        }
        LevelsToUnloadMap.FindOrAdd( level_streaming, { params.bBlockOnUnload, params.UnloadType } );
    };

    if ( infos.UnloadCurrentStreamingLevelsInfos.bUnloadCurrentlyLoadedStreamingLevels )
    {
        for ( auto * level_streaming : streaming_levels )
        {
            add_level_to_unload( level_streaming, infos.UnloadCurrentStreamingLevelsInfos );
        }
    }
    else
    {
        for ( const auto & levels_to_unload : infos.LevelsToUnload )
        {
            for ( auto * level_group : levels_to_unload.Levels.LevelGroups )
            {
                for ( const auto & level_to_unload : level_group->Levels )
                {
                    if ( auto * level_streaming = FindLevelStreaming( level_to_unload ) )
                    {
                        add_level_to_unload( level_streaming, levels_to_unload );
                    }
                }
            }

            for ( const auto & level_to_unload : levels_to_unload.Levels.IndividualLevels )
            {
                if ( auto * level_streaming = FindLevelStreaming( level_to_unload ) )
                {
                    add_level_to_unload( level_streaming, levels_to_unload );
                }
            }
        }
    }

    for ( const auto & levels_to_load : infos.LevelsToLoad )
    {
        const auto add_level_to_load = [ &levels_to_load, this ]( ULevelStreaming * level_streaming ) {
            LevelsToLoadMap.FindOrAdd( level_streaming, { levels_to_load.bBlockOnLoad, levels_to_load.LoadType } );
            LevelsToUnloadMap.Remove( level_streaming );
        };

        for ( auto * level_group : levels_to_load.Levels.LevelGroups )
        {
            for ( const auto & level_to_unload : level_group->Levels )
            {
                if ( auto * level_streaming = FindLevelStreaming( level_to_unload ) )
                {
                    add_level_to_load( level_streaming );
                }
            }
        }

        for ( const auto & level_to_unload : levels_to_load.Levels.IndividualLevels )
        {
            if ( auto * level_streaming = FindLevelStreaming( level_to_unload ) )
            {
                add_level_to_load( level_streaming );
            }
        }
    }

    switch ( infos.LoadOrder )
    {
        case EPLSLoadOrder::UnloadAndLoadInParallel:
        {
            UnloadLevels( false );
            LoadLevels( false );
            bIsProcessingRequest = false;
            OnRequestFinishedDelegate.ExecuteIfBound();
        }
        break;
        case EPLSLoadOrder::LoadThenUnload:
        {
            LoadLevels( true );
        }
        break;
        case EPLSLoadOrder::UnloadThenLoad:
        {
            UnloadLevels( true );
        }
        break;
        default:
        {
            checkNoEntry();
        }
        break;
    }
}

ULevelStreaming * UPLSSubsystem::FindLevelStreaming( const FSoftObjectPath & soft_object_path ) const
{
    const auto level_name = FName( *FPackageName::ObjectPathToPackageName( soft_object_path.ToString() ) );
    const auto safe_level_name = FStreamLevelAction::MakeSafeLevelName( level_name, GetWorld() );
    const auto & streaming_levels = GetWorld()->GetStreamingLevels();

    for ( auto * level_streaming : streaming_levels )
    {
        if ( level_streaming != nullptr && level_streaming->GetWorldAssetPackageName().EndsWith( safe_level_name, ESearchCase::IgnoreCase ) )
        {
            return level_streaming;
        }
    }

    return nullptr;
}

void UPLSSubsystem::FinishProcessingRequest()
{
    bIsProcessingRequest = false;
    OnRequestFinishedDelegate.ExecuteIfBound();
}

void UPLSSubsystem::UnloadLevels( const bool load_levels_when_finished )
{
    for ( const auto & pair : LevelsToUnloadMap )
    {
        auto * level_streaming = pair.Key;

        if ( !level_streaming->IsLevelLoaded() )
        {
            continue;
        }

        const auto should_be_loaded = pair.Value.UnloadType == EPLSLevelStreamingUnloadType::HideAndUnload;

        level_streaming->SetShouldBeLoaded( should_be_loaded );
        level_streaming->SetShouldBeVisible( false );
        level_streaming->bShouldBlockOnUnload = pair.Value.bBlockOnUnload;

        for ( FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator )
        {
            if ( APlayerController * PlayerController = Iterator->Get() )
            {
                PlayerController->LevelStreamingStatusChanged(
                    level_streaming,
                    should_be_loaded,
                    false,
                    pair.Value.bBlockOnUnload,
                    INDEX_NONE );
            }
        }

        LevelToUnloadCount++;

        level_streaming->OnLevelHidden.AddDynamic( this, &UPLSSubsystem::OnLevelStreamingUnloaded );
    }

    if ( LevelToUnloadCount == 0 )
    {
        if ( load_levels_when_finished )
        {
            LoadLevels( false );
        }
        else
        {
            FinishProcessingRequest();
        }
    }
}

void UPLSSubsystem::LoadLevels( const bool unload_levels_when_finished )
{
    for ( const auto & pair : LevelsToLoadMap )
    {
        auto * level_streaming = pair.Key;

        if ( level_streaming->IsLevelVisible() && pair.Value.LoadType == EPLSLevelStreamingLoadType::LoadAndMakeVisible )
        {
            continue;
        }

        if ( level_streaming->IsLevelLoaded() && pair.Value.LoadType == EPLSLevelStreamingLoadType::Load )
        {
            continue;
        }

        const auto make_visible = pair.Value.LoadType == EPLSLevelStreamingLoadType::LoadAndMakeVisible;

        level_streaming->SetShouldBeLoaded( true );
        level_streaming->SetShouldBeVisible( make_visible );
        level_streaming->bShouldBlockOnLoad = pair.Value.bBlockOnLoad;

        for ( FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator )
        {
            if ( APlayerController * PlayerController = Iterator->Get() )
            {
                PlayerController->LevelStreamingStatusChanged(
                    level_streaming,
                    true,
                    make_visible,
                    pair.Value.bBlockOnLoad,
                    INDEX_NONE );
            }
        }

        LevelToLoadCount++;

        if ( make_visible )
        {
            level_streaming->OnLevelShown.AddDynamic( this, &UPLSSubsystem::OnLevelStreamingLoadedOrVisible );
        }
        else
        {
            level_streaming->OnLevelLoaded.AddDynamic( this, &UPLSSubsystem::OnLevelStreamingLoadedOrVisible );
        }
    }

    if ( LevelToLoadCount == 0 )
    {
        if ( unload_levels_when_finished )
        {
            UnloadLevels( false );
        }
        else
        {
            FinishProcessingRequest();
        }
    }
}

void UPLSSubsystem::OnLevelStreamingUnloaded()
{
    LevelToUnloadCount--;

    if ( LevelToUnloadCount == 0 )
    {
        LoadLevels( false );
    }
}

void UPLSSubsystem::OnLevelStreamingLoadedOrVisible()
{
    LevelToLoadCount--;

    if ( LevelToLoadCount == 0 )
    {
        UnloadLevels( false );
    }
}

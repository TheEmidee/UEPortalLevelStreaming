#include "PLSRequest.h"

#include <Engine/LevelStreaming.h>

void UPLSRequest::Initialize( const FPLSLevelStreamingInfos & infos, const FPLSOnRequestExecutedDelegate & on_request_executed )
{
    LevelToLoadCount = 0.0f;
    LevelToUnloadCount = 0.0f;
    LoadOrder = infos.LoadOrder;
    Handle.GenerateNewHandle();
    OnRequestExecutedDelegate = on_request_executed;

    const auto & streaming_levels = GetWorld()->GetStreamingLevels();

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
}

void UPLSRequest::Cancel()
{
    UnbindLevelStreamingEvents();
    LevelsToUnloadMap.Reset();
    LevelsToLoadMap.Reset();
    LevelToUnloadCount = 0;
    LevelToLoadCount = 0;
}

void UPLSRequest::Process()
{
    switch ( LoadOrder )
    {
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

UWorld * UPLSRequest::GetWorld() const
{
    if ( IsTemplate() )
    {
        return nullptr;
    }

    if ( const auto * outer = GetOuter() )
    {
        return outer->GetWorld();
    }

    return nullptr;
}

ULevelStreaming * UPLSRequest::FindLevelStreaming( const FSoftObjectPath & soft_object_path ) const
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

void UPLSRequest::UnloadLevels( const bool load_levels_when_finished )
{
    for ( const auto & pair : LevelsToUnloadMap )
    {
        auto * level_streaming = pair.Key;
        const auto should_be_unloaded = pair.Value.UnloadType == EPLSLevelStreamingUnloadType::HideAndUnload;

        if ( should_be_unloaded && !level_streaming->IsLevelLoaded() || pair.Value.UnloadType == EPLSLevelStreamingUnloadType::Hide && !level_streaming->IsLevelVisible() )
        {
            continue;
        }

        level_streaming->SetShouldBeLoaded( !should_be_unloaded );
        level_streaming->SetShouldBeVisible( false );
        level_streaming->bShouldBlockOnUnload = pair.Value.bBlockOnUnload;

        for ( FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator )
        {
            if ( APlayerController * PlayerController = Iterator->Get() )
            {
                PlayerController->LevelStreamingStatusChanged(
                    level_streaming,
                    !should_be_unloaded,
                    false,
                    pair.Value.bBlockOnUnload,
                    INDEX_NONE );
            }
        }

        LevelToUnloadCount++;

        level_streaming->OnLevelHidden.AddDynamic( this, &UPLSRequest::OnLevelStreamingUnloaded );
    }

    if ( LevelToUnloadCount == 0 )
    {
        if ( load_levels_when_finished )
        {
            LoadLevels( false );
        }
        else
        {
            BroadcastExecutedEvent();
        }
    }
}

void UPLSRequest::LoadLevels( const bool unload_levels_when_finished )
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
            level_streaming->OnLevelShown.AddDynamic( this, &ThisClass::OnLevelStreamingLoadedOrVisible );
        }
        else
        {
            level_streaming->OnLevelLoaded.AddDynamic( this, &ThisClass::OnLevelStreamingLoadedOrVisible );
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
            BroadcastExecutedEvent();
        }
    }
}

void UPLSRequest::OnLevelStreamingUnloaded()
{
    LevelToUnloadCount--;

    // LevelToUnloadCount can become negative if active requests are cancelled
    if ( LevelToUnloadCount <= 0 )
    {
        LevelsToUnloadMap.Reset();
        LoadLevels( false );
    }
}

void UPLSRequest::OnLevelStreamingLoadedOrVisible()
{
    LevelToLoadCount--;

    // LevelToLoadCount can become negative if active requests are cancelled
    if ( LevelToLoadCount <= 0 )
    {
        LevelsToLoadMap.Reset();
        UnloadLevels( false );
    }
}

void UPLSRequest::BroadcastExecutedEvent() const
{
    UnbindLevelStreamingEvents();
    OnRequestExecutedDelegate.ExecuteIfBound( Handle );
}

void UPLSRequest::UnbindLevelStreamingEvents() const
{
    const auto & streaming_levels = GetWorld()->GetStreamingLevels();

    for ( auto * level_streaming : streaming_levels )
    {
        level_streaming->OnLevelShown.RemoveAll( this );
        level_streaming->OnLevelHidden.RemoveAll( this );
        level_streaming->OnLevelLoaded.RemoveAll( this );
    }
}

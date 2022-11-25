#include "PLSAsyncActionWaitAllRequestsFinished.h"

#include "PLSSubsystem.h"

UPLSAsyncActionWaitAllRequestsFinished * UPLSAsyncActionWaitAllRequestsFinished::WaitForAllPortalLevelStreaminRequestsToBeFinished( UObject * world_context_object )
{
    UPLSAsyncActionWaitAllRequestsFinished * action = nullptr;

    if ( auto * world = GEngine->GetWorldFromContextObject( world_context_object, EGetWorldErrorMode::LogAndReturnNull ) )
    {
        action = NewObject< UPLSAsyncActionWaitAllRequestsFinished >();
        action->WorldPtr = world;
        action->RegisterWithGameInstance( world );
    }

    return action;
}

void UPLSAsyncActionWaitAllRequestsFinished::Activate()
{
    if ( const auto * world = WorldPtr.Get() )
    {
        if ( auto * pls_system = world->GetSubsystem< UPLSSubsystem >() )
        {
            FPLSOnAllRequestsFinishedDelegate::FDelegate delegate = FPLSOnAllRequestsFinishedDelegate::FDelegate::CreateUObject( this, &ThisClass::OnAllRequestsFinished );
            pls_system->CallOrRegister_OnAllRequestsFinished( delegate );
        }
        else
        {
            // No world so we'll never finish naturally
            SetReadyToDestroy();
        }
    }
}

void UPLSAsyncActionWaitAllRequestsFinished::OnAllRequestsFinished()
{
    OnAllRequestsFinishedDelegate.Broadcast();
    SetReadyToDestroy();
}

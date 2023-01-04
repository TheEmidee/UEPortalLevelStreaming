#include "PLSSubsystem.h"

#include <Engine/LevelStreaming.h>

FPLSLevelStreamingRequestHandle UPLSSubsystem::K2_AddRequest( const FPLSLevelStreamingInfos & infos, const FPLSOnRequestExecutedDynamicDelegate & request_executed_delegate, bool cancel_existing_requests )
{
    const auto executed_delegate = FPLSOnRequestExecutedDelegate::CreateWeakLambda( const_cast< UObject * >( request_executed_delegate.GetUObject() ), [ this, request_executed_delegate ]( const auto handle ) {
        request_executed_delegate.ExecuteIfBound( handle );
        OnRequestExecuted( handle );
    } );

    return AddRequest( infos, executed_delegate, cancel_existing_requests );
}

FPLSLevelStreamingRequestHandle UPLSSubsystem::AddRequest( const FPLSLevelStreamingInfos & infos, FPLSOnRequestExecutedDelegate request_executed_delegate, bool cancel_existing_requests )
{
    const auto * world = GetWorld();

    if ( cancel_existing_requests )
    {
        for ( const auto & request : Requests )
        {
            request->Cancel();
        }
        Requests.Reset();
    }

    auto * request = NewObject< UPLSRequest >( this );

    const auto executed_delegate = FPLSOnRequestExecutedDelegate::CreateWeakLambda( this, [ this, request_executed_delegate ]( const auto handle ) {
        request_executed_delegate.ExecuteIfBound( handle );
        OnRequestExecuted( handle );
    } );

    request->Initialize( infos, executed_delegate );

    Requests.Emplace( request );

    world->GetTimerManager().SetTimerForNextTick( this, &ThisClass::ProcessNextRequest );

    return request->GetHandle();
}

TOptional< FPLSLevelStreamingInfos > UPLSSubsystem::GetRequestInfos( FPLSLevelStreamingRequestHandle request_handle ) const
{
    if ( auto * infos = RequestHandleToInfosMap.Find( request_handle ) )
    {
        return *infos;
    }

    return TOptional< FPLSLevelStreamingInfos >();
}

void UPLSSubsystem::CallOrRegister_OnAllRequestsFinished( FPLSOnAllRequestsFinishedDelegate::FDelegate delegate )
{
    if ( Requests.IsEmpty() )
    {
        delegate.Execute();
    }
    else
    {
        OnAllRequestsFinishedDelegate.Add( MoveTemp( delegate ) );
    }
}

void UPLSSubsystem::OnRequestExecuted( FPLSLevelStreamingRequestHandle handle )
{
    Requests.RemoveAll( [ handle ]( auto * request ) {
        const auto result = request->GetHandle() == handle;
        check( !request->IsExecuting() );
        return result;
    } );

    RequestHandleToInfosMap.Remove( handle );

    OnRequestExecutedDelegate.Broadcast( handle );
    ProcessNextRequest();
}

void UPLSSubsystem::ProcessNextRequest()
{
    if ( Requests.IsEmpty() )
    {
        OnAllRequestsFinishedDelegate.Broadcast();
        OnAllRequestsFinishedDelegate.Clear();
        return;
    }

    auto * request = Requests[ 0 ];

    if ( request->IsExecuting() )
    {
        return;
    }

    request->Process();
}
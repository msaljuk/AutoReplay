// Copyright 2024 JukiTech. All Rights Reserved.

#include "InputPlayer/JTInputPlayer.h"

#include "JTAutoReplayConsoleMenu.h"
#include "InputRecorder/JTInputRecorder.h"
#include "InputSerializer/JTInputSerializer.h"

#include "Engine/LocalPlayer.h"
#include "Engine/GameViewportClient.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Slate/SceneViewport.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY(LogJTInputPlayer);

void UJTInputPlayer::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UJTInputPlayer::Deinitialize()
{
	Super::Deinitialize();

	StopPlaying();
}

void UJTInputPlayer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	TickCurrentSession();
	DrawDebug();
}

TStatId UJTInputPlayer::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UJTInputPlayer, STATGROUP_Tickables);
}

bool UJTInputPlayer::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return (WorldType == EWorldType::Game || WorldType == EWorldType::PIE);
}

void UJTInputPlayer::RequestPlay(const FJTInputPlayerRequestParams RequestParams)
{
	RequestPlay_Internal(RequestParams, true);
}

void UJTInputPlayer::StopPlaying()
{
	StopPlaying_Internal(true);
}

void UJTInputPlayer::StartPlaying()
{
	ResetStartTimerHandle();

	if (CachedCurrentRequestParams.bRestorePlayerSpatialDataOnStart)
	{
		const bool bRestoredPlayerSpatialData = TryRestorePlayerSpatialData();
		if (!bRestoredPlayerSpatialData)
		{
			UE_LOG(LogJTInputPlayer,
				Error,
				TEXT("Cannot start playing %s. Unable to restore player spatial data on start. Is the player count the same as the recording?"), *CachedCurrentRequestParams.RecordingFilePath.FilePath);
			return;
		}
	}

	SessionStartFrame = GFrameCounter;
	LastTimelineEventIndex = INDEX_NONE;
	bCurrentlyPlayingSession = true;

	UE_LOG(LogJTInputPlayer, Log, TEXT("Play Started"));

	OnStartedPlaying.Broadcast();
}

void UJTInputPlayer::RequestPlay_Internal(
	const FJTInputPlayerRequestParams& RequestParams,
	bool bShouldResetExistingRequest)
{
	if (bCurrentlyPlayingSession)
	{
		StopPlaying_Internal(bShouldResetExistingRequest);
	}

	UE_LOG(LogJTInputPlayer, Log, TEXT("Play Requested"));

	CachedCurrentRequestParams = RequestParams;

	const bool bImportedSession = FJTInputSerializer::ImportSessionFromJson(CachedCurrentRequestParams.RecordingFilePath, CurrentSession);
	if (!bImportedSession)
	{
		UE_LOG(
			LogJTInputPlayer, Error, TEXT("Cannot complete play request %s. Unable to import session from file"), *CachedCurrentRequestParams.RecordingFilePath.FilePath);
		return;
	}

	if (CachedCurrentRequestParams.TimeDelayBeforePlaying > 0.f)
	{
		GetWorld()->GetTimerManager().SetTimer(
			CurrentSessionStartTimerHandle, this, &UJTInputPlayer::StartPlaying, CachedCurrentRequestParams.TimeDelayBeforePlaying);
	}
	else
	{
		StartPlaying();
	}
}

void UJTInputPlayer::StopPlaying_Internal(bool bShouldResetExistingRequest)
{
	if (!bCurrentlyPlayingSession)
	{
		return;
	}

	ResetStartTimerHandle();

	StopOngoingInput();

	CurrentSession.ClearSessionData();
	SessionStopFrame = GFrameCounter;
	LastTimelineEventIndex = INDEX_NONE;
	bCurrentlyPlayingSession = false;
	if (bShouldResetExistingRequest)
	{
		CachedCurrentRequestParams = FJTInputPlayerRequestParams();
		CurrentRecordingPlayCount = 0;
	}

	UE_LOG(LogJTInputPlayer, Log, TEXT("Play Stopped"));

	OnStoppedPlaying.Broadcast();
}

void UJTInputPlayer::TickCurrentSession()
{
	if (!bCurrentlyPlayingSession)
	{
		return;
	}

	int32 NextTimelineEventIndex = 0;
	if (LastTimelineEventIndex != INDEX_NONE)
	{
		NextTimelineEventIndex = LastTimelineEventIndex + 1;
	}

	if (!CurrentSession.InputTimeline.IsValidIndex(NextTimelineEventIndex))
	{
		++CurrentRecordingPlayCount;
		if ((CachedCurrentRequestParams.NumTimesToPlay < 0)
			|| (CurrentRecordingPlayCount < CachedCurrentRequestParams.NumTimesToPlay))
		{
			RequestPlay_Internal(CachedCurrentRequestParams, false);
		}
		else
		{
			StopPlaying_Internal(true);
		}

		return;
	}

	const FJTFrameDelta CurrentFrameDelta = (GFrameCounter - SessionStartFrame);
	const FJTFrameDelta NextTimelineEventFrameDelta = CurrentSession.InputTimeline[NextTimelineEventIndex].FrameDelta;

	if (CurrentFrameDelta != NextTimelineEventFrameDelta)
	{
		return;
	}

	UGameViewportClient* GameViewportClient = GetWorld()->GetGameInstance()->GetGameViewportClient();
	if (!IsValid(GameViewportClient))
	{
		return;
	}

	FSceneViewport* GameViewport = GameViewportClient->GetGameViewport();
	if (!GameViewport)
	{
		return;
	}

	for (const FJTInputTimelineEvent& TimelineEvent : CurrentSession.InputTimeline[NextTimelineEventIndex].FrameEvents)
	{
		if (TimelineEvent.EventType == EJTInputEventType::Key)
		{
			const FJTInputKeyEventArgs& KeyEventArgs = TimelineEvent.KeyEventArgs;
			GameViewportClient->InputKey(FInputKeyEventArgs(GameViewport, KeyEventArgs.ControllerId, KeyEventArgs.Key, KeyEventArgs.Event, KeyEventArgs.AmountDepressed, KeyEventArgs.bIsTouchEvent));
		}
		else if (TimelineEvent.EventType == EJTInputEventType::Axis)
		{
			const FJTInputAxisEventArgs& AxisEventArgs = TimelineEvent.AxisEventArgs;

			IPlatformInputDeviceMapper& DeviceMapper = IPlatformInputDeviceMapper::Get();
			FPlatformUserId UserId = PLATFORMUSERID_NONE;
			FInputDeviceId DeviceId = INPUTDEVICEID_NONE;
			DeviceMapper.RemapControllerIdToPlatformUserAndDevice(AxisEventArgs.ControllerId, UserId, DeviceId);

			GameViewportClient->InputAxis(GameViewport, DeviceId, AxisEventArgs.Key, AxisEventArgs.Delta, AxisEventArgs.DeltaTime, AxisEventArgs.NumSamples, AxisEventArgs.bGamepad);
		}
	}

	LastTimelineEventIndex = NextTimelineEventIndex;
}

bool UJTInputPlayer::TryRestorePlayerSpatialData()
{
	const TArray<ULocalPlayer*>& LocalPlayers = GetWorld()->GetGameInstance()->GetLocalPlayers();

	// Ensure that we can actually start the session in the same state
	{
		int32 CountOfValidPlayers = 0;
		for (const ULocalPlayer* LocalPlayer : LocalPlayers)
		{
			if (APlayerController* PlayerController = LocalPlayer->PlayerController)
			{
				if (APawn* PlayerPawn = PlayerController->GetPawn())
				{
					++CountOfValidPlayers;
				}
			}
		}

		if (CountOfValidPlayers != CurrentSession.PlayersSpatialDataCollection.Num())
		{
			return false;
		}
	}

	// Setup players in the same state
	{
		int32 Index = 0;
		for (const ULocalPlayer* LocalPlayer : LocalPlayers)
		{
			if (APlayerController* PlayerController = LocalPlayer->PlayerController)
			{
				if (APawn* PlayerPawn = PlayerController->GetPawn())
				{
					PlayerPawn->SetActorTransform(CurrentSession.PlayersSpatialDataCollection[Index].PawnTransform);
					PlayerController->SetControlRotation(CurrentSession.PlayersSpatialDataCollection[Index].ControlRotation);

					++Index;
				}
			}
		}
	}

	return true;
}

void UJTInputPlayer::StopOngoingInput()
{
	const TArray<ULocalPlayer*>& LocalPlayers = GetWorld()->GetGameInstance()->GetLocalPlayers();
	for (const ULocalPlayer* LocalPlayer : LocalPlayers)
	{
		if (APlayerController* PlayerController = LocalPlayer->PlayerController)
		{
			PlayerController->FlushPressedKeys();
		}
	}
}

void UJTInputPlayer::ResetStartTimerHandle()
{
	GetWorld()->GetTimerManager().ClearTimer(CurrentSessionStartTimerHandle);
	CurrentSessionStartTimerHandle.Invalidate();
}

void UJTInputPlayer::DrawDebug() const
{
#if UE_ENABLE_DEBUG_DRAWING
	if (bCurrentlyPlayingSession && JT::AutoReplay::InputPlayer::CVarShowPlayStatus.GetValueOnGameThread())
	{
		static const uint64 PlayStatusHashKey = GetTypeHash(FString("JTInputPlayerPlayStatus"));
		static const FColor PlayStatusColor = FColor::Green;

		const FString PlayStatusString
			= FString::Printf(TEXT("Playing Recording Session: %s"), *FPaths::GetPathLeaf(CachedCurrentRequestParams.RecordingFilePath.FilePath));

		GEngine->AddOnScreenDebugMessage(PlayStatusHashKey, 0.f, PlayStatusColor, PlayStatusString);
	}
#endif // UE_ENABLE_DEBUG_DRAWING
}

// Copyright 2024 JukiTech. All Rights Reserved.

#include "InputRecorder/JTInputRecorder.h"

#include "JTAutoReplayConsoleMenu.h"
#include "InputSerializer/JTInputSerializer.h"

#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"
#include "UnrealClient.h"

DEFINE_LOG_CATEGORY(LogJTInputRecorder);

void UJTInputRecorder::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UJTInputRecorder::Deinitialize()
{
	Super::Deinitialize();

	if (bIsCurrentlyRecording)
	{
		StopRecording();
	}
}

void UJTInputRecorder::Tick(float DeltaTime)
{
	DrawDebug();
}

TStatId UJTInputRecorder::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UJTInputRecorder, STATGROUP_Tickables);
}

void UJTInputRecorder::RequestRecording(const FJTInputRecorderRequestParams RequestParams)
{
	if (bIsCurrentlyRecording)
	{
		StopRecording();
	}

	UE_LOG(LogJTInputRecorder, Log, TEXT("Input Recording Requested"));

	CachedCurrentRequestParams = RequestParams;

	if (CachedCurrentRequestParams.TimeDelayBeforeRecording > 0.f)
	{
		GetWorld()->GetTimerManager().SetTimer(
			CurrentSessionStartTimerHandle, this, &UJTInputRecorder::StartRecording, CachedCurrentRequestParams.TimeDelayBeforeRecording);
	}
	else
	{
		StartRecording();
	}
}

void UJTInputRecorder::StartRecording()
{
	ResetStartTimerHandle();

	FJTPlayersSpatialDataCollection CurrentPlayersSpatialDataCollection;
	{
		const TArray<ULocalPlayer*>& LocalPlayers = GetGameInstance()->GetLocalPlayers();
		for (const ULocalPlayer* LocalPlayer : LocalPlayers)
		{
			// Only count local players with an actual PC
			if (const APlayerController* PlayerController = LocalPlayer->PlayerController)
			{
				FJTPlayerSpatialData CurrentPlayerSpatialData;
				if (const APawn* PlayerPawn = PlayerController->GetPawn())
				{
					CurrentPlayerSpatialData.ControlRotation = PlayerController->GetControlRotation();
					CurrentPlayerSpatialData.PawnTransform = PlayerPawn->GetActorTransform();

					CurrentPlayersSpatialDataCollection.Emplace(CurrentPlayerSpatialData);
				}
			}
		}
	}

	CurrentRecordingSession.StartSession(CurrentPlayersSpatialDataCollection);
	UpdateEventArgsDelegates(true);

	bIsCurrentlyRecording = true;
	bIsCurrentlyEscaped = false;

	UE_LOG(LogJTInputRecorder, Log, TEXT("Input Recording Started"));

	OnStartedRecording.Broadcast();
}

void UJTInputRecorder::StopRecording()
{
	ResetStartTimerHandle();

	UpdateEventArgsDelegates(false);
	CurrentRecordingSession.StopSession();

	if (CachedCurrentRequestParams.RecordingFilePath.FilePath.IsEmpty())
	{
		FJTInputSerializer::ExportSessionToJsonWithDefaultPath(CurrentRecordingSession);
	}
	else
	{
		FJTInputSerializer::ExportSessionToJson(CachedCurrentRequestParams.RecordingFilePath, CurrentRecordingSession);
	}

	CachedCurrentRequestParams = FJTInputRecorderRequestParams();
	bIsCurrentlyRecording = false;
	bIsCurrentlyEscaped = false;

	UE_LOG(LogJTInputRecorder, Log, TEXT("Input Recording Stopped"));

	OnStoppedRecording.Broadcast();
}

void UJTInputRecorder::RecordKeyInput(const FInputKeyEventArgs& EventArgs)
{
	const bool ShouldRecordKey = DetermineIfKeyShouldBeRecorded(EventArgs.Key, EventArgs.Event);
	if (!ShouldRecordKey)
	{
		return;
	}

	FJTInputKeyEventArgs KeyEventArgs(EventArgs);
	CurrentRecordingSession.RecordKey(KeyEventArgs);
}

void UJTInputRecorder::RecordAxisInput(
	FViewport* InViewport,
	int32      ControllerID,
	FKey       Key,
	float      Delta,
	float      DeltaTime,
	int32      NumSamples,
	bool       bGamepad)
{
	const bool ShouldRecordKey = DetermineIfKeyShouldBeRecorded(Key, EInputEvent::IE_Axis);
	if (!ShouldRecordKey)
	{
		return;
	}

	FJTInputAxisEventArgs AxisEventArgs(Key, Delta, DeltaTime, ControllerID, NumSamples, bGamepad);
	CurrentRecordingSession.RecordAxis(AxisEventArgs);
}

void UJTInputRecorder::UpdateEventArgsDelegates(bool bShouldBind)
{
	UGameInstance* GameInstance = GetGameInstance();
	if (!IsValid(GameInstance))
	{
		return;
	}

	UGameViewportClient* GameViewportClient = GameInstance->GetGameViewportClient();
	if (!IsValid(GameViewportClient))
	{
		return;
	}

	if (bShouldBind)
	{
		GameViewportClient->OnInputKey().AddUObject(this, &UJTInputRecorder::RecordKeyInput);
		GameViewportClient->OnInputAxis().AddUObject(this, &UJTInputRecorder::RecordAxisInput);
	}
	else
	{
		GameViewportClient->OnInputKey().RemoveAll(this);
		GameViewportClient->OnInputAxis().RemoveAll(this);
	}
}

void UJTInputRecorder::ResetStartTimerHandle()
{
	GetWorld()->GetTimerManager().ClearTimer(CurrentSessionStartTimerHandle);
	CurrentSessionStartTimerHandle.Invalidate();
}

bool UJTInputRecorder::DetermineIfKeyShouldBeRecorded(const FKey& Key, const TEnumAsByte<EInputEvent> InputEvent)
{
	const UJTAutoReplaySettings* Settings = UJTAutoReplaySettings::GetSettings();
	if (ensure(IsValid(Settings)))
	{
		const bool IsCurrentKeyEscapeKey = (Key == Settings->RecordingEscapeKey);

		if (IsCurrentKeyEscapeKey)
		{
			if (InputEvent == EInputEvent::IE_Pressed)
			{
				bIsCurrentlyEscaped = !bIsCurrentlyEscaped;
			}

			return false;
		}
		else
		{
			if (bIsCurrentlyEscaped)
			{
				return false;
			}
		}
	}

	if (GetWorld()->IsPaused() && !CachedCurrentRequestParams.bRecordInputWhenGamePaused)
	{
		return false;
	}

	return true;
}

void UJTInputRecorder::DrawDebug() const
{
#if UE_ENABLE_DEBUG_DRAWING
	if (bIsCurrentlyRecording && JT::AutoReplay::InputRecorder::CVarShowRecordingStatus.GetValueOnGameThread())
	{
		static const uint64 RecordingStatusHashKey = GetTypeHash(FString("JTInputRecorderStatus"));
		static const FColor RecordingStatusColor = FColor::Red;

		GEngine->AddOnScreenDebugMessage(
			RecordingStatusHashKey,
			0.f,
			RecordingStatusColor,
			FString("Recording Session In Progress"));

		if (bIsCurrentlyEscaped)
		{
			static const uint64 RecordingEscapeHashKey = GetTypeHash(FString("JTInputRecorderEscape"));

			GEngine->AddOnScreenDebugMessage(
				RecordingEscapeHashKey,
				0.f,
				RecordingStatusColor,
				FString("Currently Escaped. Skipping recording inputs"));
		}

		if (GetWorld()->IsPaused() && !CachedCurrentRequestParams.bRecordInputWhenGamePaused)
		{
			static const uint64 RecordingPausedHashKey = GetTypeHash(FString("JTInputRecorderPaused"));

			GEngine->AddOnScreenDebugMessage(
				RecordingPausedHashKey,
				0.f,
				RecordingStatusColor,
				FString("Currently Paused. Skipping recording inputs"));
		}
	}
#endif // UE_ENABLE_DEBUG_DRAWING
}
// Copyright 2024 JukiTech. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "InputCoreTypes.h"
#include "InputKeyEventArgs.h"
#include "Misc/DateTime.h"
#include "Misc/Paths.h"

#include <type_traits>

#include "JTAutoReplayCommonTypes.generated.h"

#ifndef ASSERT_ON_VAR_TYPE
	#define ASSERT_ON_VAR_TYPE(VAR, TYPE) static_assert(std::is_same<decltype(VAR), TYPE>::value, "Variable " #VAR " must have type " #TYPE);
#endif

/**
 * Wrapper struct for FInputKeyEventArgs to allow
 * UProperty serializations
 */
USTRUCT()
struct AUTOREPLAY_API FJTInputKeyEventArgs
{
	GENERATED_BODY()

public:
	FJTInputKeyEventArgs() = default;

	FJTInputKeyEventArgs(const FInputKeyEventArgs& InEventArgs)
		: Key(InEventArgs.Key)
		, InputDevice(InEventArgs.InputDevice)
		, ControllerId(InEventArgs.ControllerId)
		, AmountDepressed(InEventArgs.AmountDepressed)
		, Event(InEventArgs.Event)
		, bIsTouchEvent(InEventArgs.bIsTouchEvent)
	{
	}

public:
	UPROPERTY()
	FKey Key = FKey();

	UPROPERTY()
	FInputDeviceId InputDevice = FInputDeviceId();

	UPROPERTY()
	int32 ControllerId = 0;

	UPROPERTY()
	float AmountDepressed = 0.f;

	UPROPERTY()
	TEnumAsByte<EInputEvent> Event = EInputEvent::IE_MAX;

	UPROPERTY()
	bool bIsTouchEvent = false;
};

/**
 * Parallel struct to FJTInputKeyEventArgs for axis input events
 */
USTRUCT()
struct AUTOREPLAY_API FJTInputAxisEventArgs
{
	GENERATED_BODY()

public:
	FJTInputAxisEventArgs() = default;

	FJTInputAxisEventArgs(const FKey& InKey, float InDelta, float InDeltaTime, int32 InControllerId, int32 InNumSamples, bool bInGamepad)
		: Key(InKey)
		, Delta(InDelta)
		, DeltaTime(InDeltaTime)
		, ControllerId(InControllerId)
		, NumSamples(InNumSamples)
		, bGamepad(bInGamepad)
	{
	}

public:
	UPROPERTY()
	FKey Key = FKey();

	UPROPERTY()
	float Delta = 0.f;

	UPROPERTY()
	float DeltaTime = 0.f;

	UPROPERTY()
	int32 ControllerId = 0;

	UPROPERTY()
	int32 NumSamples = 0;

	UPROPERTY()
	bool bGamepad = false;
};

/**
 * Used to indicate whether an input event was a key
 * or axis one
 */
UENUM()
enum class EJTInputEventType : uint8
{
	Key,
	Axis,

	Invalid = 255
};

/**
 * A single event on an input timeline
 */
USTRUCT()
struct AUTOREPLAY_API FJTInputTimelineEvent
{
	GENERATED_BODY()

public:
	FJTInputTimelineEvent() = default;

	FJTInputTimelineEvent(const FJTInputKeyEventArgs& InKeyEventArgs)
		: KeyEventArgs(InKeyEventArgs)
		, EventType(EJTInputEventType::Key)
	{}

	FJTInputTimelineEvent(const FJTInputAxisEventArgs& InAxisEventArgs)
		: AxisEventArgs(InAxisEventArgs)
		, EventType(EJTInputEventType::Axis)
	{}

public:
	UPROPERTY()
	FJTInputKeyEventArgs KeyEventArgs;

	UPROPERTY()
	FJTInputAxisEventArgs AxisEventArgs;

	UPROPERTY()
	EJTInputEventType EventType = EJTInputEventType::Invalid;
};

typedef uint32 FJTFrameDelta;

USTRUCT()
struct AUTOREPLAY_API FJTInputTimelineFrame
{
	GENERATED_BODY()

public:
	UPROPERTY()
	uint32 FrameDelta = 0;
	ASSERT_ON_VAR_TYPE(FrameDelta, FJTFrameDelta);

	UPROPERTY()
	TArray<FJTInputTimelineEvent> FrameEvents;
};

USTRUCT()
struct AUTOREPLAY_API FJTPlayerSpatialData
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FTransform PawnTransform = FTransform::Identity;

	UPROPERTY()
	FRotator ControlRotation = FRotator::ZeroRotator;
};

typedef TArray<FJTInputTimelineFrame> FJTInputTimeline;
typedef TArray<FJTPlayerSpatialData>  FJTPlayersSpatialDataCollection;

enum class EJTInputRecordingFormatVersion : uint8
{
	Initial = 0,

	Count,
	Latest = Count - 1
};

USTRUCT(BlueprintType)
struct AUTOREPLAY_API FJTInputRecordingSession
{
	GENERATED_BODY()

public:
	FJTInputRecordingSession()
		: RecordingFormatVersion(static_cast<uint8>(EJTInputRecordingFormatVersion::Latest))
	{
	}

	FORCEINLINE void StartSession(const FJTPlayersSpatialDataCollection& InPlayersSpatialDataCollection)
	{
		ClearSessionData();
		PlayersSpatialDataCollection = InPlayersSpatialDataCollection;
		StartTime = FDateTime::Now().ToString();
		StartFrameCounter = GFrameCounter;
	};

	FORCEINLINE void StopSession()
	{
		StopTime = FDateTime::Now().ToString();
		StopFrameCounter = GFrameCounter;
	}

	FORCEINLINE void ClearSessionData()
	{
		InputTimeline.Reset();
		PlayersSpatialDataCollection.Reset();

		StartFrameCounter = 0;
		StopFrameCounter = 0;
	}

	FORCEINLINE void RecordKey(const FJTInputKeyEventArgs& InKeyEventArgs)
	{
		FJTInputTimelineEvent TimelineEvent(InKeyEventArgs);
		RecordTimelineEvent(TimelineEvent);
	}

	FORCEINLINE void RecordAxis(const FJTInputAxisEventArgs& InAxisEventArgs)
	{
		FJTInputTimelineEvent TimelineEvent(InAxisEventArgs);
		RecordTimelineEvent(TimelineEvent);
	}

public:
	UPROPERTY()
	TArray<FJTInputTimelineFrame> InputTimeline;
	ASSERT_ON_VAR_TYPE(InputTimeline, FJTInputTimeline);

	UPROPERTY()
	TArray<FJTPlayerSpatialData> PlayersSpatialDataCollection;
	ASSERT_ON_VAR_TYPE(PlayersSpatialDataCollection, FJTPlayersSpatialDataCollection);

	UPROPERTY()
	FString StartTime;

	UPROPERTY()
	FString StopTime;

	UPROPERTY()
	uint64 StartFrameCounter = 0;

	UPROPERTY()
	uint64 StopFrameCounter = 0;

	UPROPERTY()
	uint8 RecordingFormatVersion = 0;

private:
	FORCEINLINE void RecordTimelineEvent(const FJTInputTimelineEvent& TimelineEvent)
	{
		const FJTFrameDelta CurrentTimelineEventFrameDelta = (GFrameCounter - StartFrameCounter);

		if (!InputTimeline.IsEmpty())
		{
			const FJTFrameDelta LastTimelineEventFrameDelta = InputTimeline.Last().FrameDelta;
			if (LastTimelineEventFrameDelta == CurrentTimelineEventFrameDelta)
			{
				InputTimeline[InputTimeline.Num() - 1].FrameEvents.Emplace(TimelineEvent);
				return;
			}
		}

		FJTInputTimelineFrame NewTimelineFrame;
		NewTimelineFrame.FrameDelta = CurrentTimelineEventFrameDelta;
		NewTimelineFrame.FrameEvents.Emplace(TimelineEvent);

		InputTimeline.Emplace(NewTimelineFrame);
	}
};

UCLASS(config = Plugins, BlueprintType, defaultconfig)
class AUTOREPLAY_API UJTAutoReplaySettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UJTAutoReplaySettings()
	{
		static const FString DefaultInputRecordingSessionsDirectory = (FPaths::ProjectContentDir() + FString("JTInputRecordingSessions/"));
		RecordingSessionExportDirectory = DefaultInputRecordingSessionsDirectory;
	}

	static const UJTAutoReplaySettings* GetSettings() { return GetDefault<UJTAutoReplaySettings>(); }

public:
	UPROPERTY(EditAnywhere, config, Category = "Input Recording")
	FString RecordingSessionExportDirectory = FString();

	/**
	 * Can be used to skip input recording for any sequence that
	 * comes between a pair of this escape key presses
	 */
	UPROPERTY(EditAnywhere, config, Category = "Input Recording")
	FKey RecordingEscapeKey;
};

#ifdef ASSERT_ON_VAR_TYPE
	#undef ASSERT_ON_VAR_TYPE
#endif

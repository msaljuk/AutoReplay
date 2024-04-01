// Copyright 2024 JukiTech. All Rights Reserved.

#pragma once

#include "JTAutoReplayCommonTypes.h"

#include "Engine/TimerHandle.h"
#include "Subsystems/WorldSubsystem.h"

#include "JTInputPlayer.generated.h"

AUTOREPLAY_API DECLARE_LOG_CATEGORY_EXTERN(LogJTInputPlayer, Log, All);

class FViewport;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FJTInputPlayerDelegate);

/**
 * Used to define how an input play session should be conducted
 */
USTRUCT(BlueprintType)
struct FJTInputPlayerRequestParams
{
	GENERATED_BODY()

public:
	/** The file path of the recorded input session that needs to be played */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Parameters")
	FFilePath RecordingFilePath = FFilePath();

	/** If true, all players will be restored to the transforms they held at the time of recording */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Parameters")
	bool bRestorePlayerSpatialDataOnStart = true;

	/** The amount of time (in seconds) after the request is sent when play should start */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Parameters")
	float TimeDelayBeforePlaying = 0.0f;

	/** The number of times to play the recording (if negative, will be looped infinitely) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Parameters")
	int32 NumTimesToPlay = 1;
};

/**
 * The input player subsystem is responsible for fielding requests to play previously recorded
 * input sessions
 */
UCLASS(MinimalAPI)
class UJTInputPlayer : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	/** UTickableWorldSubsystem Interface - BEGIN  */
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
	/** UTickableWorldSubsystem Interface - END */

	/** Call to request the start of a play session */
	UFUNCTION(BlueprintCallable, Category = "Scripting")
	AUTOREPLAY_API void RequestPlay(const FJTInputPlayerRequestParams RequestParams);

	/** Call to request termination of an ongoing play session */
	UFUNCTION(BlueprintCallable, Category = "Scripting")
	AUTOREPLAY_API void StopPlaying();

public:
	/** Called when a new play session is started */
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FJTInputPlayerDelegate OnStartedPlaying;

	/** Called when an ongoing play session ends */
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FJTInputPlayerDelegate OnStoppedPlaying;

private:
	void StartPlaying();
	void RequestPlay_Internal(const FJTInputPlayerRequestParams& RequestParams, bool bShouldResetExistingRequest);
	void StopPlaying_Internal(bool bShouldResetExistingRequest);
	void TickCurrentSession();
	bool TryRestorePlayerSpatialData();
	void StopOngoingInput();
	void ResetStartTimerHandle();
	void DrawDebug() const;

private:
	FJTInputPlayerRequestParams CachedCurrentRequestParams;
	FJTInputRecordingSession CurrentSession;
	FTimerHandle CurrentSessionStartTimerHandle;
	uint64 SessionStartFrame = 0;
	uint64 SessionStopFrame = 0;
	int32 LastTimelineEventIndex = INDEX_NONE;
	int32 CurrentRecordingPlayCount = 0;
	bool bCurrentlyPlayingSession = false;
};

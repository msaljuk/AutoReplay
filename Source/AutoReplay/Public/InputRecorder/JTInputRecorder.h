// Copyright 2024 JukiTech. All Rights Reserved.

#pragma once

#include "JTAutoReplayCommonTypes.h"

#include "Engine/TimerHandle.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"

#include "JTInputRecorder.generated.h"

AUTOREPLAY_API DECLARE_LOG_CATEGORY_EXTERN(LogJTInputRecorder, Log, All);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FJTInputRecorderDelegate);

/**
 * Used to define how an input recording session should be conducted
 */
USTRUCT(BlueprintType)
struct FJTInputRecorderRequestParams
{
	GENERATED_BODY()

public:
	/** The file path to where the recording should be saved */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Parameters")
	FFilePath RecordingFilePath = FFilePath();

	/** The amount of time (in seconds) after the request is sent when recording should start */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Parameters")
	float TimeDelayBeforeRecording = 0.0f;

	/** Whether or not inputs should be recorded when the game is paused */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Parameters")
	bool bRecordInputWhenGamePaused = false;
};

/**
 * The input recorder subsystem is responsible for fielding requests to record input sessions
 */
UCLASS(MinimalAPI)
class UJTInputRecorder : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	/** UGameInstanceSubsystem Interface - BEGIN  */
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	/** UGameInstanceSubsystem Interface - END */

	/** FTickableGameObject Interface - BEGIN */
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const;
	virtual UWorld* GetTickableGameObjectWorld() const override { return GetWorld(); }
	virtual bool IsTickableWhenPaused() const override { return true; }
	/** FTickableGameObject Interface - END */

	/** Call to request the start of a recording session */
	UFUNCTION(BlueprintCallable, Category = "Scripting")
	AUTOREPLAY_API void RequestRecording(const FJTInputRecorderRequestParams RequestParams);

	/** Call to terminate an ongoing recording session */
	UFUNCTION(BlueprintCallable, Category = "Scripting")
	AUTOREPLAY_API void StopRecording();

public:
	/** Called when a new recording session is started */
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FJTInputRecorderDelegate OnStartedRecording;

	/** Called when an ongoing recording session stops */
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FJTInputRecorderDelegate OnStoppedRecording;

protected:
	void RecordKeyInput(const FInputKeyEventArgs& EventArgs);

	void RecordAxisInput(
		FViewport* InViewport,
		int32      ControllerID,
		FKey       Key,
		float      Delta,
		float      DeltaTime,
		int32      NumSamples,
		bool       bGamepad
	);

private:
	void StartRecording();
	void UpdateEventArgsDelegates(bool bShouldBind);
	void ResetStartTimerHandle();
	bool DetermineIfKeyShouldBeRecorded(const FKey& Key, const TEnumAsByte<EInputEvent> InputEvent);
	void DrawDebug() const;

private:
	FJTInputRecorderRequestParams CachedCurrentRequestParams;
	FJTInputRecordingSession CurrentRecordingSession;
	FTimerHandle CurrentSessionStartTimerHandle;
	bool bIsCurrentlyRecording = false;
	bool bIsCurrentlyEscaped = false;
};

// Copyright 2024 JukiTech. All Rights Reserved.

#pragma once

#include "HAL/IConsoleManager.h"

namespace JT
{
	namespace AutoReplay
	{
		namespace InputPlayer
		{
			extern FAutoConsoleCommandWithWorldAndArgs CCommandRequestPlay;
			extern FAutoConsoleCommandWithWorldAndArgs CCommandStopPlaying;
			extern TAutoConsoleVariable<bool> CVarShowPlayStatus;
		} // Input Player

		namespace InputRecorder
		{
			extern FAutoConsoleCommandWithWorldAndArgs CCommandRequestRecording;
			extern FAutoConsoleCommandWithWorldAndArgs CCommandStopRecording;
			extern TAutoConsoleVariable<bool> CVarShowRecordingStatus;
		} // Input Recorder
	} // namespace AutoReplay
} // namespace JT

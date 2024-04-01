// Copyright 2024 JukiTech. All Rights Reserved.

#pragma once

#include "JTAutoReplayCommonTypes.h"

#include "Misc/Paths.h"
#include "UObject/SoftObjectPath.h"

AUTOREPLAY_API DECLARE_LOG_CATEGORY_EXTERN(LogJTInputSerializer, Log, All);

class FJTInputSerializer
{
public:
	/**
	 * Exports the given recording session to a json file
	 *
	 * @param InJsonFilePath the file path where the json will be exported
	 * @param InSession the recording session to export
	 *
	 * @return whether or not the session exported successfully
	 */
	AUTOREPLAY_API static bool ExportSessionToJson(const FFilePath& InJsonFilePath, const FJTInputRecordingSession& InSession);

	/**
	 * Exports the given recording session to a json at the default file path
	 *
	 * @param InSession the recording session to export
	 *
	 * @return whether or not the session exported successfully
	 */
	AUTOREPLAY_API static bool ExportSessionToJsonWithDefaultPath(const FJTInputRecordingSession& InSession);

	/**
	 * Imports the given recording session from the given json file
	 *
	 * @param InJsonFilePath the file path where the recording session json is stored
	 * @param OutSession the recording session to import
	 *
	 * @return whether or not the session imported successfully
	 */
	AUTOREPLAY_API static bool ImportSessionFromJson(const FFilePath& InJsonFilePath, FJTInputRecordingSession& OutSession);

private:
	static bool TryConstructFinalPath(const FFilePath& InJsonFilePath, FFilePath& OutFinalPath);
};

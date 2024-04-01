// Copyright 2024 JukiTech. All Rights Reserved.

#include "InputSerializer/JTInputSerializer.h"

#include "JsonObjectConverter.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"

static const FString DefaultInputRecordingSessionPrefix = "IRS";
static const FString JsonFileExtension = "json";

DEFINE_LOG_CATEGORY(LogJTInputSerializer);

bool FJTInputSerializer::ExportSessionToJson(const FFilePath& InJsonFilePath, const FJTInputRecordingSession& InSession)
{
	FString SessionJsonString;
	FJsonObjectConverter::UStructToJsonObjectString<FJTInputRecordingSession>(InSession, SessionJsonString);

	FFilePath FinalPath;
	const bool bConstructedFinalPath = TryConstructFinalPath(InJsonFilePath, FinalPath);
	if (!bConstructedFinalPath)
	{
		UE_LOG(LogJTInputSerializer, Error, TEXT("Could not export session to Json. Unable to construct final path"));
		return false;
	}

	const bool bSavedJsonStringToFile = FFileHelper::SaveStringToFile(SessionJsonString, *FinalPath.FilePath);
	if (!bSavedJsonStringToFile)
	{
		UE_LOG(LogJTInputSerializer, Error, TEXT("Could not export session to Json. Unable to save Json string to file"));
		return false;
	}

	return true;
}

bool FJTInputSerializer::ExportSessionToJsonWithDefaultPath(const FJTInputRecordingSession& InSession)
{
	FFilePath DefaultPath;
	DefaultPath.FilePath =
		(UJTAutoReplaySettings::GetSettings()->RecordingSessionExportDirectory
			+ DefaultInputRecordingSessionPrefix + FDateTime::Now().ToString());

	return ExportSessionToJson(DefaultPath, InSession);
}

bool FJTInputSerializer::ImportSessionFromJson(const FFilePath& InJsonFilePath, FJTInputRecordingSession& OutSession)
{
	FFilePath FinalPath;
	const bool bConstructedFinalPath = TryConstructFinalPath(InJsonFilePath, FinalPath);
	if (!bConstructedFinalPath)
	{
		UE_LOG(LogJTInputSerializer, Error, TEXT("Could not import session from Json. Unable to construct final path"));
		return false;
	}

	FString SessionJsonString;
	const bool bLoadedJsonToString = FFileHelper::LoadFileToString(SessionJsonString, *FinalPath.FilePath);
	if (!bLoadedJsonToString)
	{
		UE_LOG(LogJTInputSerializer, Error, TEXT("Could not import session from Json. Unable to load Json file to string"));
		return false;
	}

	OutSession.ClearSessionData();
	FJsonObjectConverter::JsonObjectStringToUStruct<FJTInputRecordingSession>(SessionJsonString, &OutSession);

	return true;
}

bool FJTInputSerializer::TryConstructFinalPath(const FFilePath& InJsonFilePath, FFilePath& OutFinalPath)
{
	if (InJsonFilePath.FilePath.IsEmpty())
	{
		return false;
	}

	OutFinalPath.FilePath = (UJTAutoReplaySettings::GetSettings()->RecordingSessionExportDirectory + InJsonFilePath.FilePath);

	if (FPaths::GetExtension(OutFinalPath.FilePath) != JsonFileExtension)
	{
		OutFinalPath.FilePath += FString(".") + FString(JsonFileExtension);
	}

	return true;
}

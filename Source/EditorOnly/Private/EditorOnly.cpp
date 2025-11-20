// ---------------------------------------------------
// Copyright (c) 2025 AldertLake. All Rights Reserved.
// GitHub:   https://github.com/AldertLake/
// Support:  https://ko-fi.com/aldertlake
// ---------------------------------------------------

//To reduce junk, You can delete this whole module (EditorOnly). It has only logic for rating
//MessageBox, delete the EditorOnly folder inside plugin source & remove the module from the .uplugin file.
//This will not be packaged in your game automaticaly so don't worry about it if you didnt remove it.

#include "EditorOnly.h"

#if WITH_EDITOR
#include "Misc/MessageDialog.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/Paths.h"
#include "HAL/PlatformProcess.h"
#include "Misc/FileHelper.h"
#include "HAL/FileManager.h"
#endif

#define LOCTEXT_NAMESPACE "EditorOnlyModule"

namespace
{
	FString EnsureConfigFile(const FString& FileName)
	{
		const FString Dir = FPaths::ProjectConfigDir();
		IFileManager::Get().MakeDirectory(*Dir, /*Tree=*/ true);
		FString FullPath = Dir / FileName;
		FullPath = FullPath.Replace(TEXT("\\"), TEXT("/"));
		return FullPath;
	}

	void PersistBoolToIni(const FString& IniPath, const FString& Section, const FString& Key, bool Value)
	{
		GConfig->SetBool(*Section, *Key, Value, *IniPath);
		GConfig->Flush(false, *IniPath);
		GConfig->LoadFile(*IniPath);

		bool Verified = false;
		GConfig->GetBool(*Section, *Key, Verified, *IniPath);
		if (Verified == Value)
		{
			return;
		}

		FString Contents;
		FFileHelper::LoadFileToString(Contents, *IniPath);

		const FString SectionHeader = FString::Printf(TEXT("[%s]\n"), *Section);
		const FString KeyLine = FString::Printf(TEXT("%s=%s\n"), *Key, Value ? TEXT("True") : TEXT("False"));

		if (!Contents.Contains(FString::Printf(TEXT("[%s]"), *Section)))
		{
			if (!Contents.IsEmpty() && !Contents.EndsWith(TEXT("\n")))
			{
				Contents += TEXT("\n");
			}
			Contents += SectionHeader;
		}

		Contents += KeyLine;

		FFileHelper::SaveStringToFile(Contents, *IniPath);
		GConfig->LoadFile(*IniPath);
	}
}

void FEditorOnlyModule::ShowRatingDialogIfNeeded()
{
#if WITH_EDITOR
	if (!GIsEditor || IsRunningCommandlet())
	{
		return;
	}

	const FString ConfigSection = TEXT("/Script/WindowsNativeToolkit.WNTSettings");
	const FString ConfigKey = TEXT("bDontShowRating");

	const FString ConfigFileName = TEXT("WNT.ini");
	const FString ConfigPath = EnsureConfigFile(ConfigFileName);

	GConfig->LoadFile(*ConfigPath);

	bool bDontShowRating = false;
	GConfig->GetBool(*ConfigSection, *ConfigKey, bDontShowRating, *ConfigPath);

	if (!bDontShowRating)
	{
		const FText Message = FText::FromString(
			TEXT("Thank you for using the Windows Native Toolkit !\n\n")
			TEXT("If you find the WNT helpful, please consider leaving a positive rating. ")
			TEXT("Your support helps future development and is greatly appreciated!\n\n")
			TEXT("---------------------------------------------------\n")
			TEXT("  - Click 'Yes' to Rate Now & Never Show Again\n")
			TEXT("  - Click 'No' to Remind Me Later\n")
			TEXT("  - Click 'Cancel' to Never Show Again\n")
			TEXT("---------------------------------------------------\n")
		);

		const FText Title = FText::FromString(TEXT("Windows Native Toolkit 2.3"));
		const EAppReturnType::Type Response = FMessageDialog::Open(
			EAppMsgType::YesNoCancel,
			Message,
			Title
		);

		bool bShouldPersistTrue = false;
		switch (Response)
		{
		case EAppReturnType::Yes:
			FPlatformProcess::LaunchURL(TEXT("https://www.fab.com/listings/db1cb6ed-ac7e-4408-a901-e45d6694cb0b"), nullptr, nullptr);
			bShouldPersistTrue = true;
			break;

		case EAppReturnType::Cancel:
			bShouldPersistTrue = true;
			break;

		case EAppReturnType::No:
		default:
			bShouldPersistTrue = false;
			break;
		}

		if (bShouldPersistTrue)
		{
			PersistBoolToIni(ConfigPath, ConfigSection, ConfigKey, true);
		}
	}
#endif
}


void FEditorOnlyModule::StartupModule()
{
#if WITH_EDITOR
	ShowRatingDialogIfNeeded();
#endif
}

void FEditorOnlyModule::ShutdownModule()
{
}
#undef LOCTEXT_NAMESPACE
IMPLEMENT_MODULE(FEditorOnlyModule, EditorOnly)
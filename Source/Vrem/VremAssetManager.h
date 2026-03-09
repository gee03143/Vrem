// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "VremLogChannels.h"
#include "Engine/AssetManager.h"
#include "Misc/AssertionMacros.h"
#include "VremAssetManager.generated.h"

/**
 * 
 */
UCLASS()
class VREM_API UVremAssetManager : public UAssetManager
{
	GENERATED_BODY()
	
public:
	static UVremAssetManager& Get();
	
	template<typename T>
	TSharedPtr<FStreamableHandle> LoadAssetAsync(const TSoftObjectPtr<T>& AssetPtr, TFunction<void(T*)> OnLoaded);

	template<typename T>
	T* LoadAssetSync(const TSoftObjectPtr<T>& AssetPtr);
};

template<typename T>
TSharedPtr<FStreamableHandle> UVremAssetManager::LoadAssetAsync(const TSoftObjectPtr<T>& AssetPtr, TFunction<void(T*)> OnLoaded)
{
	if (AssetPtr.IsNull())
	{
		UE_LOG(LogVremAssetManager, Warning, TEXT("LoadAssetAsync: Null Asset"));
		return nullptr;
	}

	FStreamableManager& Streamable = GetStreamableManager();

	return Streamable.RequestAsyncLoad(
		AssetPtr.ToSoftObjectPath(),
		[AssetPtr, OnLoaded]()
		{
			T* LoadedAsset = AssetPtr.Get();
			OnLoaded(LoadedAsset);
		});
}

template <typename T>
T* UVremAssetManager::LoadAssetSync(const TSoftObjectPtr<T>& AssetPtr)
{
#if !UE_BUILD_SHIPPING
	const double StartTime = FPlatformTime::Seconds();
#endif

	T* Result = AssetPtr.LoadSynchronous();

#if !UE_BUILD_SHIPPING
	const double EndTime = FPlatformTime::Seconds();
	const double DurationMs = (EndTime - StartTime) * 1000.0;

	UE_LOG(LogVremAssetManager, Warning, TEXT("Sync Load 사용 감지: %s (%.2f ms)"), *AssetPtr.ToString(), DurationMs);
	if (DurationMs > 5.0) // 임계값
	{
		UE_LOG(LogTemp, Warning, TEXT("🔥 Heavy Sync Load Detected"));
		FDebug::DumpStackTraceToLog(ELogVerbosity::Type::Warning);
	}
#endif

	return Result;
}

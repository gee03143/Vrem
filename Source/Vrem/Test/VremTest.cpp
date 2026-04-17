// Fill out your copyright notice in the Description page of Project Settings.

#if WITH_AUTOMATION_WORKER
#include "VremTest.h"
#include "Engine/World.h"

namespace VremTestHelper
{
    UWorld* CreateTestWorld()
    {
        UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
        FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
        WorldContext.SetCurrentWorld(World);
        World->InitializeActorsForPlay(FURL());
        World->BeginPlay();
        return World;
    }

    void DestroyTestWorld(UWorld* World)
    {
        GEngine->DestroyWorldContext(World);
        World->DestroyWorld(false);
    }
}

#endif // WITH_AUTOMATION_WORKER
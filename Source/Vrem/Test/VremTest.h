// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#if WITH_AUTOMATION_WORKER

class UWorld;

namespace VremTestHelper
{
    UWorld* CreateTestWorld();
    void DestroyTestWorld(UWorld* World);
}

#endif
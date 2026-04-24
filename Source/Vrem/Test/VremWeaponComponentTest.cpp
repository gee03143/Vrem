// Fill out your copyright notice in the Description page of Project Settings.

#if WITH_AUTOMATION_WORKER

#include "VremWeaponComponentTest.h"
#include "VremTest.h"
#include "VremTestTagPawn.h"
#include "Vrem/Equipment/Weapon/VremWeaponComponent.h"
#include "Vrem/Equipment/Weapon/VremWeaponDefinition.h"
#include "Vrem/VremGameplayTags.h"

namespace VremWeaponTestHelper
{
    struct FWeaponTestContext
    {
        ATestTagPawn* OwnerPawn = nullptr;          
        AActor* EquipmentActor = nullptr;           
        UVremWeaponComponent* WeaponComp = nullptr;  
        UVremWeaponDefinition* Definition = nullptr; 
    };

    UVremWeaponDefinition* CreateTestWeaponDefinition()
    {
        UVremWeaponDefinition* Def = NewObject<UVremWeaponDefinition>();
        Def->FireRate = 600.f;
        Def->Range = 10000.f;
        Def->BaseDamage = 20.f;
        Def->FireMode = EWeaponFireMode::SemiAuto;

        // spread
        Def->SpreadProfile.BaseSpread = 1.0f;
        Def->SpreadProfile.MovingSpreadMultiplier = 2.0f;
        Def->SpreadProfile.InAirSpreadMultiplier = 3.0f;
        Def->SpreadProfile.BloomPerShot = 0.5f;
        Def->SpreadProfile.MaxBloom = 5.0f;
        Def->SpreadProfile.BloomRecoverSpeed = 3.0f;
        return Def;
    }

    FWeaponTestContext CreateTestContext(UWorld* World)
    {
        FWeaponTestContext Context;

        Context.OwnerPawn = World->SpawnActor<ATestTagPawn>();

        FActorSpawnParameters Params;
        Params.Owner = Context.OwnerPawn;
        Context.EquipmentActor = World->SpawnActor<AActor>(AActor::StaticClass(), Params);

        Context.WeaponComp = NewObject<UVremWeaponComponent>(Context.EquipmentActor);
        Context.WeaponComp->RegisterComponent();

        Context.Definition = CreateTestWeaponDefinition();
        Context.WeaponComp->SetWeaponDefinition_ForTest(Context.Definition);

        return Context;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWeaponBloomAccumulationTest,
    "Vrem.Weapon.Bloom.Accumulation",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter
)

bool FWeaponBloomAccumulationTest::RunTest(const FString& Parameters)
{
    UWorld* World = VremTestHelper::CreateTestWorld();
    VremWeaponTestHelper::FWeaponTestContext Context = VremWeaponTestHelper::CreateTestContext(World);

    TestEqual(TEXT("Initial bloom should be 0"), Context.WeaponComp->GetCurrentBloom_ForTest(), 0.f);

    Context.WeaponComp->AccumulateBloom_ForTest(); 
    TestEqual(TEXT("Bloom after 1 shot BloomPerShot(0.5)"), Context.WeaponComp->GetCurrentBloom_ForTest(), 0.5f);

    Context.WeaponComp->AccumulateBloom_ForTest();
    Context.WeaponComp->AccumulateBloom_ForTest();
    TestEqual(TEXT("Bloom after 3 shots"), Context.WeaponComp->GetCurrentBloom_ForTest(), 1.5f);

    // check clamp MaxBloom(5.0)
    for (int32 i = 0; i < 20; ++i)
    {
        Context.WeaponComp->AccumulateBloom_ForTest();
    }
    TestEqual(TEXT("Bloom clamped at MaxBloom(5.0)"), Context.WeaponComp->GetCurrentBloom_ForTest(), 5.0f);

    VremTestHelper::DestroyTestWorld(World);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWeaponBloomRecoverTest,
    "Vrem.Weapon.Bloom.Recover",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter
)

bool FWeaponBloomRecoverTest::RunTest(const FString& Parameters)
{
    UWorld* World = VremTestHelper::CreateTestWorld();
    VremWeaponTestHelper::FWeaponTestContext Context = VremWeaponTestHelper::CreateTestContext(World);

    // accumulate bloom (BloomPerShot(0.5) * 4)
    for (int32 i = 0; i < 4; ++i)
    {
        Context.WeaponComp->AccumulateBloom_ForTest(); 
    }
    TestEqual(TEXT("Accumulated bloom BloomPerShot(0.5) * 4 = 2"), Context.WeaponComp->GetCurrentBloom_ForTest(), 2.0f);

    Context.WeaponComp->SimulateBloomRecover_ForTest(0.5f);
    TestEqual(TEXT("Bloom after 0.5s recovery InitlaiBloom(2) - BloomRecoverSpeed(3.0) * 0.5s = 0.5"), Context.WeaponComp->GetCurrentBloom_ForTest(), 0.5f);

    Context.WeaponComp->SimulateBloomRecover_ForTest(1.0f);
    TestEqual(TEXT("Bloom after 1s recovery clamped at 0"), Context.WeaponComp->GetCurrentBloom_ForTest(), 0.f);

    VremTestHelper::DestroyTestWorld(World);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWeaponCooldownTransitionTest,
    "Vrem.Weapon.Cooldown.Transition",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter
)

bool FWeaponCooldownTransitionTest::RunTest(const FString& Parameters)
{
    UWorld* World = VremTestHelper::CreateTestWorld();
    VremWeaponTestHelper::FWeaponTestContext Context = VremWeaponTestHelper::CreateTestContext(World);

    TestTrue(TEXT("Initially CanFire"), Context.WeaponComp->GetCanFire_ForTest());

    Context.WeaponComp->StartFireCooldown_ForTest();
    TestFalse(TEXT("During cooldown CanFire=false"), Context.WeaponComp->GetCanFire_ForTest());

    Context.WeaponComp->OnFireCooldownFinished_ForTest();
    TestTrue(TEXT("After cooldown CanFire=true"), Context.WeaponComp->GetCanFire_ForTest());

    VremTestHelper::DestroyTestWorld(World);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWeaponSpreadBaseTest,
    "Vrem.Weapon.Spread.Base",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter
)

bool FWeaponSpreadBaseTest::RunTest(const FString& Parameters)
{
    UWorld* World = VremTestHelper::CreateTestWorld();
    VremWeaponTestHelper::FWeaponTestContext Context = VremWeaponTestHelper::CreateTestContext(World);

    TestEqual(TEXT("Base spread no bloom, no tags BaseSpread(1.0) + CurrentBloom(0) = 1.0"), Context.WeaponComp->GetCurrentSpread(), 1.0f);

    Context.WeaponComp->AccumulateBloom_ForTest();
    TestEqual(TEXT("Base spread with bloom BaseSpread(1.0) + CurrentBloom(0.5) = 1.5"), Context.WeaponComp->GetCurrentSpread(), 1.5f);

    VremTestHelper::DestroyTestWorld(World);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWeaponSpreadMovingTest,
    "Vrem.Weapon.Spread.Moving",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter
)

bool FWeaponSpreadMovingTest::RunTest(const FString& Parameters)
{
    UWorld* World = VremTestHelper::CreateTestWorld();
    VremWeaponTestHelper::FWeaponTestContext Context = VremWeaponTestHelper::CreateTestContext(World);

    Context.OwnerPawn->StateTags.AddTag(FVremGameplayTags::State_Movement_Moving);
    TestEqual(TEXT("Moving spread: (BaseSpread + Bloom) * MovingMultiplier = (1.0 + 0) * 2.0 = 2.0"), Context.WeaponComp->GetCurrentSpread(), 2.0f);

    VremTestHelper::DestroyTestWorld(World);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWeaponSpreadScopedTest,
    "Vrem.Weapon.Spread.Scoped",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter
)

bool FWeaponSpreadScopedTest::RunTest(const FString& Parameters)
{
    UWorld* World = VremTestHelper::CreateTestWorld();
    VremWeaponTestHelper::FWeaponTestContext Context = VremWeaponTestHelper::CreateTestContext(World);

    Context.WeaponComp->AccumulateBloom_ForTest();
    Context.WeaponComp->AccumulateBloom_ForTest();
    Context.OwnerPawn->StateTags.AddTag(FVremGameplayTags::State_Aiming_Scoped);

    TestEqual(TEXT("Scoped spread is 0"), Context.WeaponComp->GetCurrentSpread(), 0.f);

    VremTestHelper::DestroyTestWorld(World);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWeaponSpreadCompositeTest,
    "Vrem.Weapon.Spread.Composite",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter
)

bool FWeaponSpreadCompositeTest::RunTest(const FString& Parameters)
{
    UWorld* World = VremTestHelper::CreateTestWorld();
    VremWeaponTestHelper::FWeaponTestContext Context = VremWeaponTestHelper::CreateTestContext(World);

    Context.WeaponComp->AccumulateBloom_ForTest();
    Context.WeaponComp->AccumulateBloom_ForTest();

    Context.OwnerPawn->StateTags.AddTag(FVremGameplayTags::State_Movement_Moving);
    Context.OwnerPawn->StateTags.AddTag(FVremGameplayTags::State_Movement_InAir);

    TestEqual(TEXT("Composite spread: (BaseSpread + Bloom) * Moving(2.0) * InAir(3.0) = (1.0 + 1.0) * 2.0 * 3.0 = 12.0"),
        Context.WeaponComp->GetCurrentSpread(), 12.0f);

    VremTestHelper::DestroyTestWorld(World);
    return true;
}
#endif // WITH_AUTOMATION_WORKER
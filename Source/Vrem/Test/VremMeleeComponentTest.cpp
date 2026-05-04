// Fill out your copyright notice in the Description page of Project Settings.

#if WITH_AUTOMATION_WORKER

#include "VremMeleeComponentTest.h"
#include "VremTest.h"
#include "VremTestTagPawn.h"
#include "GameFramework/Actor.h"
#include "Vrem/Equipment/MeleeWeapon/VremMeleeComponent.h"
#include "Vrem/Equipment/MeleeWeapon/VremMeleeWeaponDefinition.h"

namespace VremMeleeTestHelper
{
    struct FMeleeTestContext
    {
        ATestTagPawn* OwnerPawn = nullptr;
        AActor* EquipmentActor = nullptr;
        UVremMeleeComponent* MeleeComp = nullptr;
        UVremMeleeWeaponDefinition* Definition = nullptr;
    };

    UVremMeleeWeaponDefinition* CreateTestMeleeDefinition(int32 ComboCount = 3)
    {
        UVremMeleeWeaponDefinition* Def = NewObject<UVremMeleeWeaponDefinition>();

        for (int32 i = 0; i < ComboCount; ++i)
        {
            FAttackSequence Seq;
            Seq.Damage = 30.f + i * 10.f;
            Seq.Range = 150.f;
            Seq.TraceRadius = 40.f;
            Seq.AttackDuration = 0.9f;
            Seq.CancelTime = 0.5f;
            Def->MeleeProfile.AttackSequences.Add(Seq);
        }

        return Def;
    }

    FMeleeTestContext CreateTestContext(UWorld* World, int32 ComboCount = 3)
    {
        FMeleeTestContext Context;

        Context.OwnerPawn = World->SpawnActor<ATestTagPawn>();

        FActorSpawnParameters Params;
        Params.Owner = Context.OwnerPawn;
        Context.EquipmentActor = World->SpawnActor<AActor>(AActor::StaticClass(), Params);

        Context.MeleeComp = NewObject<UVremMeleeComponent>(Context.EquipmentActor);
        Context.MeleeComp->RegisterComponent();

        Context.Definition = CreateTestMeleeDefinition(ComboCount);
        Context.MeleeComp->SetMeleeDefinition_ForTest(Context.Definition);

        return Context;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMeleeInitialStateTest,
    "Vrem.Melee.InitialState",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter
)

bool FMeleeInitialStateTest::RunTest(const FString& Parameters)
{
    UWorld* World = VremTestHelper::CreateTestWorld();
    VremMeleeTestHelper::FMeleeTestContext Context = VremMeleeTestHelper::CreateTestContext(World);

    TestFalse(TEXT("Initially not attacking"), Context.MeleeComp->IsAttacking_ForTest());
    TestFalse(TEXT("Initially cannot cancel"), Context.MeleeComp->CanCancel_ForTest());
    TestEqual(TEXT("Initial combo index is 0"), Context.MeleeComp->GetCurrentComboIndex_ForTest(), 0);

    VremTestHelper::DestroyTestWorld(World);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMeleeAttackStateTransitionTest,
    "Vrem.Melee.AttackStateTransition",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter
)

bool FMeleeAttackStateTransitionTest::RunTest(const FString& Parameters)
{
    UWorld* World = VremTestHelper::CreateTestWorld();
    VremMeleeTestHelper::FMeleeTestContext Context = VremMeleeTestHelper::CreateTestContext(World);

    // in attack duration (is attacking (ignores attackstart input))
    Context.MeleeComp->SimulateAttackStart_ForTest();
    TestTrue(TEXT("IsAttacking after start"), Context.MeleeComp->IsAttacking_ForTest());
    TestFalse(TEXT("Not in combo window during attack"), Context.MeleeComp->CanCancel_ForTest());

    // cancel time started (is attacking, can cancel and go to next sequence)
    Context.MeleeComp->TriggerCancelTimeStarted_ForTest();
    TestTrue(TEXT("Can Cancel after Cancel time Started"), Context.MeleeComp->CanCancel_ForTest());
    TestTrue(TEXT("Still Attacking after Cancel time Started"), Context.MeleeComp->IsAttacking_ForTest());

    // attack duration finished (is not attacking, is in combo window)
    Context.MeleeComp->TriggerAttackDurationFinished_ForTest();
    TestFalse(TEXT("Not attacking after duration ends"), Context.MeleeComp->IsAttacking_ForTest());
    TestFalse(TEXT("Cannot Cancel after attack duration"), Context.MeleeComp->CanCancel_ForTest());

    VremTestHelper::DestroyTestWorld(World);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMeleeComboProgressionTest,
    "Vrem.Melee.ComboProgression",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter
)

bool FMeleeComboProgressionTest::RunTest(const FString& Parameters)
{
    UWorld* World = VremTestHelper::CreateTestWorld();
    VremMeleeTestHelper::FMeleeTestContext Context = VremMeleeTestHelper::CreateTestContext(World, 3 /* ComboCount */);

    Context.MeleeComp->SimulateAttackStart_ForTest();
    TestEqual(TEXT("After 1st attack, combo index advances to 1"), Context.MeleeComp->GetCurrentComboIndex_ForTest(), 1);
    Context.MeleeComp->TriggerAttackDurationFinished_ForTest();

    Context.MeleeComp->SimulateAttackStart_ForTest();
    TestEqual(TEXT("After 2nd attack, combo index advances to 2"), Context.MeleeComp->GetCurrentComboIndex_ForTest(), 2);
    Context.MeleeComp->TriggerAttackDurationFinished_ForTest();

    Context.MeleeComp->SimulateAttackStart_ForTest();
    TestEqual(TEXT("After 3rd attack(last attack), combo index wraps to (ComboIndex(2)+1)%ComboCount(3) = 0"), Context.MeleeComp->GetCurrentComboIndex_ForTest(), 0);

    VremTestHelper::DestroyTestWorld(World);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMeleeComboTimeoutResetTest,
    "Vrem.Melee.ComboTimeoutReset",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter
)

bool FMeleeComboTimeoutResetTest::RunTest(const FString& Parameters)
{
    UWorld* World = VremTestHelper::CreateTestWorld();
    VremMeleeTestHelper::FMeleeTestContext Context = VremMeleeTestHelper::CreateTestContext(World, 3 /* ComboCount */);

    Context.MeleeComp->SimulateAttackStart_ForTest();
    Context.MeleeComp->TriggerAttackDurationFinished_ForTest();
    TestEqual(TEXT("after attackstart: Combo index = 1"), Context.MeleeComp->GetCurrentComboIndex_ForTest(), 1);

    Context.MeleeComp->TriggerCancelTimeStarted_ForTest();
    TestEqual(TEXT("combo window timeout: Combo index reset to 0"), Context.MeleeComp->GetCurrentComboIndex_ForTest(), 0);

    Context.MeleeComp->SimulateAttackStart_ForTest();
    TestEqual(TEXT("New attack starts: index 0 -> advances to 1"), Context.MeleeComp->GetCurrentComboIndex_ForTest(), 1);

    VremTestHelper::DestroyTestWorld(World);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMeleeReentrantBlockTest,
    "Vrem.Melee.ReentrantBlock",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter
)

bool FMeleeReentrantBlockTest::RunTest(const FString& Parameters)
{
    UWorld* World = VremTestHelper::CreateTestWorld();
    VremMeleeTestHelper::FMeleeTestContext Context = VremMeleeTestHelper::CreateTestContext(World);

    Context.MeleeComp->SimulateAttackStart_ForTest();
    const int32 IndexAfterFirst = Context.MeleeComp->GetCurrentComboIndex_ForTest();
    TestEqual(TEXT("Combo index 1 after first attack"), IndexAfterFirst, 1);

    Context.MeleeComp->SimulateAttackStart_ForTest();
    TestEqual(TEXT("Re-entry should be blocked when bIsAttacking == true, index unchanged"), Context.MeleeComp->GetCurrentComboIndex_ForTest(), IndexAfterFirst);

    VremTestHelper::DestroyTestWorld(World);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMeleeInvalidDefinitionTest,
    "Vrem.Melee.InvalidDefinition",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter
)

bool FMeleeInvalidDefinitionTest::RunTest(const FString& Parameters)
{
    UWorld* World = VremTestHelper::CreateTestWorld();
    VremMeleeTestHelper::FMeleeTestContext Context = VremMeleeTestHelper::CreateTestContext(World, 0 /* ComboCount */);  // no combo

    // trying attack when ComboCount == 0
    Context.MeleeComp->SimulateAttackStart_ForTest();

    TestFalse(TEXT("No attack with empty combo"), Context.MeleeComp->IsAttacking_ForTest());
    TestEqual(TEXT("Combo index stays 0"), Context.MeleeComp->GetCurrentComboIndex_ForTest(), 0);

    // trying attack when MeleeDefinition == nullptr
    Context.MeleeComp->SetMeleeDefinition_ForTest(nullptr);
    Context.MeleeComp->SimulateAttackStart_ForTest();

    TestFalse(TEXT("No attack when MeleeDefinition == nullptr"), Context.MeleeComp->IsAttacking_ForTest());

    VremTestHelper::DestroyTestWorld(World);
    return true;
}

#endif // WITH_AUTOMATION_WORKER
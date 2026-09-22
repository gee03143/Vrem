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
// ============================================
// 권위 측 사격 승인 (ServerFire 재검증)
//
// ServerFire 는 클라가 직접 호출할 수 있는 RPC 다. 아래 테스트들은 서버가
// 클라의 요청을 재검증한다는 계약을 고정한다.
// ============================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWeaponAuthorityFirstShotTest,
    "Vrem.Weapon.Authority.FirstShot",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter
)

bool FWeaponAuthorityFirstShotTest::RunTest(const FString& Parameters)
{
    UWorld* World = VremTestHelper::CreateTestWorld();
    VremWeaponTestHelper::FWeaponTestContext Context = VremWeaponTestHelper::CreateTestContext(World);

    Context.WeaponComp->SetMagazineAmmo_ForTest(30);

    TestTrue(TEXT("Deadline starts unset (negative)"), Context.WeaponComp->GetNextAllowedFireTime_ForTest() < 0.f);
    TestTrue(TEXT("First shot is allowed regardless of world time"), Context.WeaponComp->IsFireAllowedAt_ForTest(0.f));

    VremTestHelper::DestroyTestWorld(World);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWeaponAuthorityEmptyMagazineTest,
    "Vrem.Weapon.Authority.EmptyMagazine",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter
)

bool FWeaponAuthorityEmptyMagazineTest::RunTest(const FString& Parameters)
{
    UWorld* World = VremTestHelper::CreateTestWorld();
    VremWeaponTestHelper::FWeaponTestContext Context = VremWeaponTestHelper::CreateTestContext(World);

    Context.WeaponComp->SetMagazineAmmo_ForTest(0);
    TestFalse(TEXT("Empty magazine is rejected on authority"), Context.WeaponComp->IsFireAllowedAt_ForTest(0.f));

    Context.WeaponComp->SetMagazineAmmo_ForTest(1);
    TestTrue(TEXT("One round left is allowed"), Context.WeaponComp->IsFireAllowedAt_ForTest(0.f));

    VremTestHelper::DestroyTestWorld(World);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWeaponAuthorityWhileReloadingTest,
    "Vrem.Weapon.Authority.WhileReloading",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter
)

bool FWeaponAuthorityWhileReloadingTest::RunTest(const FString& Parameters)
{
    UWorld* World = VremTestHelper::CreateTestWorld();
    VremWeaponTestHelper::FWeaponTestContext Context = VremWeaponTestHelper::CreateTestContext(World);

    Context.WeaponComp->SetMagazineAmmo_ForTest(30);
    Context.WeaponComp->SetIsReloading_ForTest(true);

    TestFalse(TEXT("Reloading is rejected even with ammo"), Context.WeaponComp->IsFireAllowedAt_ForTest(0.f));

    Context.WeaponComp->SetIsReloading_ForTest(false);
    TestTrue(TEXT("Allowed again once reload ends"), Context.WeaponComp->IsFireAllowedAt_ForTest(0.f));

    VremTestHelper::DestroyTestWorld(World);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWeaponAuthorityRateLimitTest,
    "Vrem.Weapon.Authority.RateLimit",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter
)

bool FWeaponAuthorityRateLimitTest::RunTest(const FString& Parameters)
{
    UWorld* World = VremTestHelper::CreateTestWorld();
    VremWeaponTestHelper::FWeaponTestContext Context = VremWeaponTestHelper::CreateTestContext(World);

    Context.WeaponComp->SetMagazineAmmo_ForTest(30);

    // FireRate 600 RPM -> FireInterval 0.1s, Tolerance 25% -> 0.025s
    const float Interval = Context.Definition->GetFireInterval();
    const float Tolerance = Interval * 0.25f;
    const float Epsilon = Interval * 0.01f;

    // 첫 발을 t=0 에 승인
    Context.WeaponComp->AdvanceFireDeadline_ForTest(0.f);
    TestEqual(TEXT("Deadline advances by one interval"), Context.WeaponComp->GetNextAllowedFireTime_ForTest(), Interval);

    // 허용 오차보다 더 이른 요청은 거부
    TestFalse(TEXT("Well before the deadline is rejected"),
        Context.WeaponComp->IsFireAllowedAt_ForTest(Interval - Tolerance - Epsilon));

    // 기한 도달 시 허용
    TestTrue(TEXT("At the deadline is allowed"),
        Context.WeaponComp->IsFireAllowedAt_ForTest(Interval));

    // 기한을 한참 넘겨도 허용
    TestTrue(TEXT("Well past the deadline is allowed"),
        Context.WeaponComp->IsFireAllowedAt_ForTest(Interval * 10.f));

    VremTestHelper::DestroyTestWorld(World);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWeaponAuthorityJitterToleranceTest,
    "Vrem.Weapon.Authority.JitterTolerance",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter
)

bool FWeaponAuthorityJitterToleranceTest::RunTest(const FString& Parameters)
{
    UWorld* World = VremTestHelper::CreateTestWorld();
    VremWeaponTestHelper::FWeaponTestContext Context = VremWeaponTestHelper::CreateTestContext(World);

    Context.WeaponComp->SetMagazineAmmo_ForTest(30);

    const float Interval = Context.Definition->GetFireInterval();
    const float Tolerance = Interval * 0.25f;
    const float Epsilon = Interval * 0.01f;

    Context.WeaponComp->AdvanceFireDeadline_ForTest(0.f);

    // 지터로 조금 일찍 도착한 정상 클라의 요청은 받아준다
    TestTrue(TEXT("Just inside the jitter tolerance is allowed"),
        Context.WeaponComp->IsFireAllowedAt_ForTest(Interval - Tolerance + Epsilon));

    // 허용 오차 바깥은 거부한다
    TestFalse(TEXT("Just outside the jitter tolerance is rejected"),
        Context.WeaponComp->IsFireAllowedAt_ForTest(Interval - Tolerance - Epsilon));

    VremTestHelper::DestroyTestWorld(World);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWeaponAuthorityLateShotDoesNotBankTest,
    "Vrem.Weapon.Authority.LateShotDoesNotBank",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter
)

bool FWeaponAuthorityLateShotDoesNotBankTest::RunTest(const FString& Parameters)
{
    UWorld* World = VremTestHelper::CreateTestWorld();
    VremWeaponTestHelper::FWeaponTestContext Context = VremWeaponTestHelper::CreateTestContext(World);

    Context.WeaponComp->SetMagazineAmmo_ForTest(30);

    const float Interval = Context.Definition->GetFireInterval();
    const float Tolerance = Interval * 0.25f;
    const float Epsilon = Interval * 0.01f;

    // 첫 발 이후 한참 쉬었다가 쏜다
    Context.WeaponComp->AdvanceFireDeadline_ForTest(0.f);
    const float LateTime = 5.f;
    TestTrue(TEXT("Shot after a long idle is allowed"), Context.WeaponComp->IsFireAllowedAt_ForTest(LateTime));
    Context.WeaponComp->AdvanceFireDeadline_ForTest(LateTime);

    // 쉰 시간이 여유로 적립되지 않는다 — 기한은 LateTime 기준으로 다시 선다.
    // Max() 없이 기한만 누적했다면 여기서 통과해 폭발적 연사가 가능해진다.
    TestEqual(TEXT("Deadline is rebased on the late shot, not accumulated"),
        Context.WeaponComp->GetNextAllowedFireTime_ForTest(), LateTime + Interval);

    TestFalse(TEXT("Immediate follow-up shot is still rejected"),
        Context.WeaponComp->IsFireAllowedAt_ForTest(LateTime + Interval - Tolerance - Epsilon));

    TestTrue(TEXT("Follow-up shot at the new deadline is allowed"),
        Context.WeaponComp->IsFireAllowedAt_ForTest(LateTime + Interval));

    VremTestHelper::DestroyTestWorld(World);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWeaponAuthorityRapidFireCheatTest,
    "Vrem.Weapon.Authority.RapidFireCheat",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter
)

bool FWeaponAuthorityRapidFireCheatTest::RunTest(const FString& Parameters)
{
    UWorld* World = VremTestHelper::CreateTestWorld();
    VremWeaponTestHelper::FWeaponTestContext Context = VremWeaponTestHelper::CreateTestContext(World);

    Context.WeaponComp->SetMagazineAmmo_ForTest(100);

    const float Interval = Context.Definition->GetFireInterval();
    const float Tolerance = Interval * 0.25f;
    const float Epsilon = Interval * 0.01f;

    // 허용 오차를 매번 끝까지 쓰는 클라를 재현한다.
    // 기한이 승인마다 Interval 만큼만 전진하므로, 이 이득은 발사 수에 비례해
    // 누적되지 않고 오차 1회분에서 멈춘다 — 그것이 이 설계의 요점이다.
    const int32 ShotCount = 20;
    float LastFireTime = 0.f;

    TestTrue(TEXT("First cheat shot allowed"), Context.WeaponComp->IsFireAllowedAt_ForTest(0.f));
    Context.WeaponComp->AdvanceFireDeadline_ForTest(0.f);

    for (int32 Shot = 1; Shot < ShotCount; ++Shot)
    {
        const float Deadline = Context.WeaponComp->GetNextAllowedFireTime_ForTest();
        const float EarliestAccepted = Deadline - Tolerance + Epsilon;

        if (Context.WeaponComp->IsFireAllowedAt_ForTest(EarliestAccepted) == false)
        {
            AddError(FString::Printf(TEXT("Shot %d should be accepted at %f (deadline %f)"), Shot, EarliestAccepted, Deadline));
            break;
        }

        Context.WeaponComp->AdvanceFireDeadline_ForTest(EarliestAccepted);
        LastFireTime = EarliestAccepted;
    }

    // 정상 발사율이라면 마지막 발은 (ShotCount - 1) * Interval 에 나간다.
    const float HonestLastFireTime = (ShotCount - 1) * Interval;
    const float TimeGained = HonestLastFireTime - LastFireTime;

    TestTrue(
        FString::Printf(TEXT("Cheat gain stays within one tolerance step, not proportional to shot count (gained %f, tolerance %f)"), TimeGained, Tolerance),
        TimeGained <= Tolerance + Epsilon * 2.f);

    VremTestHelper::DestroyTestWorld(World);
    return true;
}

#endif // WITH_AUTOMATION_WORKER
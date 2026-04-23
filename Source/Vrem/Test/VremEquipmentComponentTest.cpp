// Fill out your copyright notice in the Description page of Project Settings.

#if WITH_AUTOMATION_WORKER

#include "VremEquipmentComponentTest.h"
#include "VremTest.h"
#include "Misc/AutomationTest.h"
#include "GameFramework/Actor.h"
#include "Vrem/Equipment/VremEquipmentComponent.h"
#include "Vrem/Equipment/VremEquipmentDefinition.h"

namespace VremEquipmentTestHelper
{
    AActor* CreateActorWithEquipment(UWorld* World)
    {
        AActor* Actor = World->SpawnActor<AActor>();
        UVremEquipmentComponent* Comp = NewObject<UVremEquipmentComponent>(Actor);
        Comp->RegisterComponent();
        return Actor;
    }

    UVremEquipmentDefinition* CreateTestDefinition()
    {
        UVremEquipmentDefinition* Def = NewObject<UVremEquipmentDefinition>();
        Def->AttachSocketName = TEXT("hand_r");
        Def->HolsterSocketName = TEXT("spine_01");
        return Def;
    }
}

// ============================================
// 장비 장착 테스트
// ============================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEquipmentTryEquipItemTest,
    "Vrem.Equipment.TryEquipItem",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter
)

bool FEquipmentTryEquipItemTest::RunTest(const FString& Parameters)
{
    UWorld* World = VremTestHelper::CreateTestWorld();
    AActor* Actor = VremEquipmentTestHelper::CreateActorWithEquipment(World);
    UVremEquipmentComponent* EquipComp = Actor->FindComponentByClass<UVremEquipmentComponent>();
    EquipComp->InitializeFromOwner();

    UVremEquipmentDefinition* Def = VremEquipmentTestHelper::CreateTestDefinition();

    // 장착 전 확인
    TestEqual(TEXT("Initial equipment count should be 0"), EquipComp->GetEquipmentItemNum(), 0);

    // 장착
    EquipComp->TryEquipItem(Def, 1);
    TestEqual(TEXT("Equipment count should be 1 after equip"), EquipComp->GetEquipmentItemNum(), 1);

    // 장착 직후 상태는 Holstered여야 함
    EEquipmentState State = EquipComp->GetEquipmentStateAtSlot(1);
    TestEqual(TEXT("Initial state should be Holstered"), State, EEquipmentState::Holstered);

    // 같은 슬롯에 다시 장착 → 교체
    UVremEquipmentDefinition* Def2 = VremEquipmentTestHelper::CreateTestDefinition();
    EquipComp->TryEquipItem(Def2, 1);
    TestEqual(TEXT("Equipment count should still be 1 after replace"), EquipComp->GetEquipmentItemNum(), 1);

    // 다른 슬롯에 장착
    EquipComp->TryEquipItem(Def, 2);
    TestEqual(TEXT("Equipment count should be 2"), EquipComp->GetEquipmentItemNum(), 2);

    VremTestHelper::DestroyTestWorld(World);
    return true;
}

// ============================================
// 장비 해제 테스트
// ============================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEquipmentTryUnequipItemTest,
    "Vrem.Equipment.TryUnequipItem",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter
)

bool FEquipmentTryUnequipItemTest::RunTest(const FString& Parameters)
{
    UWorld* World = VremTestHelper::CreateTestWorld();
    AActor* Actor = VremEquipmentTestHelper::CreateActorWithEquipment(World);
    UVremEquipmentComponent* EquipComp = Actor->FindComponentByClass<UVremEquipmentComponent>();
    EquipComp->InitializeFromOwner();

    // 빈 상태에서 교체 시도 → 크래시 없어야 함
    EquipComp->SetCurrentWeapon(99);
    TestEqual(TEXT("Count should remain 0"), EquipComp->GetEquipmentItemNum(), 0);

    UVremEquipmentDefinition* Def = VremEquipmentTestHelper::CreateTestDefinition();

    EquipComp->TryEquipItem(Def, 1);
    TestEqual(TEXT("Equipment count should be 1"), EquipComp->GetEquipmentItemNum(), 1);

    // 해제
    EquipComp->TryUnequipItem(1);
    TestEqual(TEXT("Equipment count should be 0 after unequip"), EquipComp->GetEquipmentItemNum(), 0);

    // 없는 슬롯 해제 시도 → 크래시 없이 통과
    EquipComp->TryUnequipItem(99);
    TestEqual(TEXT("Equipment count should remain 0"), EquipComp->GetEquipmentItemNum(), 0);

    VremTestHelper::DestroyTestWorld(World);
    return true;
}

// ============================================
// 무기 교체(SetCurrentWeapon) 테스트
// ============================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEquipmentSetCurrentWeaponTest,
    "Vrem.Equipment.SetCurrentWeapon",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter
)

bool FEquipmentSetCurrentWeaponTest::RunTest(const FString& Parameters)
{
    UWorld* World = VremTestHelper::CreateTestWorld();
    AActor* Actor = VremEquipmentTestHelper::CreateActorWithEquipment(World);
    UVremEquipmentComponent* EquipComp = Actor->FindComponentByClass<UVremEquipmentComponent>();
    EquipComp->InitializeFromOwner();

    UVremEquipmentDefinition* Rifle = VremEquipmentTestHelper::CreateTestDefinition();
    UVremEquipmentDefinition* Pistol = VremEquipmentTestHelper::CreateTestDefinition();

    int32 AttachedCount = 0;
    int32 DetachedCount = 0;
    EquipComp->OnEquipmenntAttached.AddLambda([&AttachedCount](const TSubclassOf<UAnimInstance>) { AttachedCount++; });
    EquipComp->OnEquipmenntDetached.AddLambda([&DetachedCount](const TSubclassOf<UAnimInstance>) { DetachedCount++; });

    // 장착 → Holstered 상태이므로 Detached 발생
    EquipComp->TryEquipItem(Rifle, 1);
    TestEqual(TEXT("Detached should fire on initial equip (Holstered)"), DetachedCount, 1);
    TestEqual(TEXT("Slot 1 should be Holstered"), EquipComp->GetEquipmentStateAtSlot(1), EEquipmentState::Holstered);

    // 슬롯 1을 현재 무기로 설정
    EquipComp->SetCurrentWeapon(1);
    TestEqual(TEXT("Attached should fire on SetCurrentWeapon"), AttachedCount, 1);
    TestEqual(TEXT("Slot 1 should be Equipped"), EquipComp->GetEquipmentStateAtSlot(1), EEquipmentState::OnHand);

   
    EquipComp->TryEquipItem(Pistol, 2);
    TestEqual(TEXT("Detached should fire on initial equip (Holstered)"), DetachedCount, 2);
    TestEqual(TEXT("Slot 1 should be Holstered"), EquipComp->GetEquipmentStateAtSlot(2), EEquipmentState::Holstered);
    
    // 다른 슬롯으로 교체 → 이전 무기 Detached
    EquipComp->SetCurrentWeapon(2);
    TestEqual(TEXT("Slot 1 should be Holstered"), EquipComp->GetEquipmentStateAtSlot(1), EEquipmentState::Holstered);
    TestEqual(TEXT("Slot 2 should be Equipped"), EquipComp->GetEquipmentStateAtSlot(2), EEquipmentState::OnHand);
    TestEqual(TEXT("Detached should fire for previous weapon"), DetachedCount, 3);
    TestEqual(TEXT("Attached should fire for new weapon"), AttachedCount, 2);

    // 장비 수는 변하지 않아야 함
    TestEqual(TEXT("Equipment count should remain 2"), EquipComp->GetEquipmentItemNum(), 2);

    VremTestHelper::DestroyTestWorld(World);
    return true;
}

// ============================================
// Definition으로 해제 테스트
// ============================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEquipmentUnequipByDefinitionTest,
    "Vrem.Equipment.TryUnequipItemByDefinition",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter
)

bool FEquipmentUnequipByDefinitionTest::RunTest(const FString& Parameters)
{
    UWorld* World = VremTestHelper::CreateTestWorld();
    AActor* Actor = VremEquipmentTestHelper::CreateActorWithEquipment(World);
    UVremEquipmentComponent* EquipComp = Actor->FindComponentByClass<UVremEquipmentComponent>();
    EquipComp->InitializeFromOwner();

    UVremEquipmentDefinition* Def = VremEquipmentTestHelper::CreateTestDefinition();
    EquipComp->TryEquipItem(Def, 1);

    // Definition으로 해제
    EquipComp->TryUnequipItem(Def);
    TestEqual(TEXT("Equipment count should be 0"), EquipComp->GetEquipmentItemNum(), 0);

    VremTestHelper::DestroyTestWorld(World);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEquipmentReplicationSimTest,
    "Vrem.Equipment.ReplicationSim.EquipItem",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter
)

bool FEquipmentReplicationSimTest::RunTest(const FString& Parameters)
{
    UWorld* World = VremTestHelper::CreateTestWorld();

    // 서버 액터 + 컴포넌트
    AActor* ServerActor = VremEquipmentTestHelper::CreateActorWithEquipment(World);
    UVremEquipmentComponent* ServerComp = ServerActor->FindComponentByClass<UVremEquipmentComponent>();
    ServerComp->InitializeFromOwner();

    // 클라이언트 액터 + 컴포넌트 (같은 월드지만 별도 컴포넌트로 시뮬레이션)
    AActor* ClientActor = VremEquipmentTestHelper::CreateActorWithEquipment(World);
    UVremEquipmentComponent* ClientComp = ClientActor->FindComponentByClass<UVremEquipmentComponent>();
    ClientComp->InitializeFromOwner();

    UVremEquipmentDefinition* Def = VremEquipmentTestHelper::CreateTestDefinition();

    // 서버에서 장착
    ServerComp->TryEquipItem(Def, 1);
    TestEqual(TEXT("Server should have 1 item"), ServerComp->GetEquipmentItemNum(), 1);
    TestEqual(TEXT("Client should have 0 items before replication"), ClientComp->GetEquipmentItemNum(), 0);

    ClientComp->SimulateReplicateFrom(ServerComp);
    TestEqual(TEXT("Client should have 1 item after replication"), ClientComp->GetEquipmentItemNum(), 1);

    // 복제 후 클라이언트 측 EquipmentState역시 갱신되어야 함
    ServerComp->SetCurrentWeapon(1);
    ClientComp->SimulateReplicateFrom(ServerComp);
    TestEqual(TEXT("Client slot 1 should be Equipped after replication"), ClientComp->GetEquipmentStateAtSlot(1), EEquipmentState::OnHand);

    VremTestHelper::DestroyTestWorld(World);
    return true;
}

#endif // WITH_AUTOMATION_WORKER
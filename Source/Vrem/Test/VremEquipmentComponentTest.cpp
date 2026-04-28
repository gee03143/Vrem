// Fill out your copyright notice in the Description page of Project Settings.

#if WITH_AUTOMATION_WORKER

#include "VremEquipmentComponentTest.h"
#include "VremTest.h"
#include "Misc/AutomationTest.h"
#include "GameFramework/Actor.h"
#include "Vrem/Equipment/VremEquipmentComponent.h"
#include "Vrem/Equipment/VremEquipmentDefinition.h"
#include "Vrem/Equipment/VremEquipmentActor.h"

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
// TryEquipItem Test
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

    UVremEquipmentDefinition* Def = VremEquipmentTestHelper::CreateTestDefinition();

    // 장착 전 확인
    TestEqual(TEXT("Initial equipment count should be 0"), EquipComp->GetEquipmentItemNum(), 0);

    // 장착
    EquipComp->TryEquipItem(Def, 1);
    TestEqual(TEXT("Equipment count should be 1 after equip"), EquipComp->GetEquipmentItemNum(), 1);

    // 장착 직후 상태는 Holstered여야 함
    EEquipmentState State = EquipComp->GetEquipmentStateAtSlot(1);
    TestEqual(TEXT("Initial state should be Stowed"), State, EEquipmentState::Stowed);

    // 같은 슬롯에 다시 장착 -> 교체
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
// TryUnequipItem Test
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

    // 빈 상태에서 교체 시도 -> 크래시 없어야 함
    EquipComp->SetCurrentWeapon(99);
    TestEqual(TEXT("Count should remain 0"), EquipComp->GetEquipmentItemNum(), 0);

    UVremEquipmentDefinition* Def = VremEquipmentTestHelper::CreateTestDefinition();

    EquipComp->TryEquipItem(Def, 1);
    TestEqual(TEXT("Equipment count should be 1"), EquipComp->GetEquipmentItemNum(), 1);

    // 해제
    EquipComp->TryUnequipItem(1);
    TestEqual(TEXT("Equipment count should be 0 after unequip"), EquipComp->GetEquipmentItemNum(), 0);

    // 없는 슬롯 해제 시도 -> 크래시 없이 통과
    EquipComp->TryUnequipItem(99);
    TestEqual(TEXT("Equipment count should remain 0"), EquipComp->GetEquipmentItemNum(), 0);

    VremTestHelper::DestroyTestWorld(World);
    return true;
}

// ============================================
// SlotQuery Test
// ============================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEquipmentSlotQueryTest,
    "Vrem.Equipment.SlotQuery",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter
)

bool FEquipmentSlotQueryTest::RunTest(const FString& Parameters)
{
    UWorld* World = VremTestHelper::CreateTestWorld();
    AActor* Actor = VremEquipmentTestHelper::CreateActorWithEquipment(World);
    UVremEquipmentComponent* EquipComp = Actor->FindComponentByClass<UVremEquipmentComponent>();

    // 빈 상태
    TestEqual(TEXT("Empty: No OnHand"), EquipComp->GetOnHandSlotIndex(), INDEX_NONE);
    TestEqual(TEXT("Empty: No Holstered"), EquipComp->GetHolsteredSlotIndex(), INDEX_NONE);

    UVremEquipmentDefinition* A = VremEquipmentTestHelper::CreateTestDefinition();
    EquipComp->TryEquipItem(A, 1);

    // Stowed 상태만 있을 때
    TestEqual(TEXT("Only Stowed: No OnHand"), EquipComp->GetOnHandSlotIndex(), INDEX_NONE);
	TestEqual(TEXT("Only Stowed: No Holstered"), EquipComp->GetHolsteredSlotIndex(), INDEX_NONE);

    EquipComp->SetCurrentWeapon(1);

    // A가 OnHand
    TestEqual(TEXT("OnHand slot is 1"), EquipComp->GetOnHandSlotIndex(), 1);
    TestEqual(TEXT("No Holstered yet"), EquipComp->GetHolsteredSlotIndex(), INDEX_NONE);

    UVremEquipmentDefinition* B = VremEquipmentTestHelper::CreateTestDefinition();
    EquipComp->TryEquipItem(B, 2);
    EquipComp->SetCurrentWeapon(2, EEquipmentState::Holstered);  // A -> Holstered

    TestEqual(TEXT("OnHand slot is now 2"), EquipComp->GetOnHandSlotIndex(), 2);
    TestEqual(TEXT("Holstered slot is 1"), EquipComp->GetHolsteredSlotIndex(), 1);

    VremTestHelper::DestroyTestWorld(World);
    return true;
}

// ============================================
// SetCurrentWeapon Test
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

    UVremEquipmentDefinition* Rifle = VremEquipmentTestHelper::CreateTestDefinition();
    UVremEquipmentDefinition* Pistol = VremEquipmentTestHelper::CreateTestDefinition();

    UVremEquipmentTestListener* Listener = NewObject<UVremEquipmentTestListener>(EquipComp);
    EquipComp->OnEquipmentAttached.AddDynamic(Listener, &UVremEquipmentTestListener::HandleAttached);
    EquipComp->OnEquipmentDetached.AddDynamic(Listener, &UVremEquipmentTestListener::HandleDetached);

    // 장착 -> Stowed 상태이므로 Detached 발생
    EquipComp->TryEquipItem(Rifle, 1);
    TestEqual(TEXT("Detached should fire on initial equip (Stowed)"), Listener->DetachedCount, 1);
    TestEqual(TEXT("Slot 1 should be Stowed"), EquipComp->GetEquipmentStateAtSlot(1), EEquipmentState::Stowed);

    // 슬롯 1을 현재 무기로 설정
    EquipComp->SetCurrentWeapon(1);
    TestEqual(TEXT("Attached should fire on SetCurrentWeapon"), Listener->AttachedCount, 1);
    TestEqual(TEXT("Slot 1 should be OnHand"), EquipComp->GetEquipmentStateAtSlot(1), EEquipmentState::OnHand);

    EquipComp->TryEquipItem(Pistol, 2);
    TestEqual(TEXT("Detached should fire on initial equip (Stowed)"), Listener->DetachedCount, 2);
    TestEqual(TEXT("Slot 2 should be Stowed"), EquipComp->GetEquipmentStateAtSlot(2), EEquipmentState::Stowed);
    
    // 다른 슬롯으로 교체 -> 이전 무기 Detached
    EquipComp->SetCurrentWeapon(2);
    TestEqual(TEXT("Slot 1 should be Stowed"), EquipComp->GetEquipmentStateAtSlot(1), EEquipmentState::Stowed);
    TestEqual(TEXT("Slot 2 should be OnHand"), EquipComp->GetEquipmentStateAtSlot(2), EEquipmentState::OnHand);
    TestEqual(TEXT("Detached should fire for previous weapon"), Listener->DetachedCount, 3);
    TestEqual(TEXT("Attached should fire for new weapon"), Listener->AttachedCount, 2);

    // 장비 수는 변하지 않아야 함
    TestEqual(TEXT("Equipment count should remain 2"), EquipComp->GetEquipmentItemNum(), 2);

    VremTestHelper::DestroyTestWorld(World);
    return true;
}

// ============================================
// TryUnequipItem By Definition Test
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

    UVremEquipmentDefinition* Def = VremEquipmentTestHelper::CreateTestDefinition();
    EquipComp->TryEquipItem(Def, 1);

    // Definition으로 해제
    EquipComp->TryUnequipItem(Def);
    TestEqual(TEXT("Equipment count should be 0"), EquipComp->GetEquipmentItemNum(), 0);

    VremTestHelper::DestroyTestWorld(World);
    return true;
}

// ============================================
// Component Replication Test
// ============================================
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

    // 클라이언트 액터 + 컴포넌트 (같은 월드지만 별도 컴포넌트로 시뮬레이션)
    AActor* ClientActor = VremEquipmentTestHelper::CreateActorWithEquipment(World);
    UVremEquipmentComponent* ClientComp = ClientActor->FindComponentByClass<UVremEquipmentComponent>();

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
    TestEqual(TEXT("Client slot 1 should be OnHand after replication"), ClientComp->GetEquipmentStateAtSlot(1), EEquipmentState::OnHand);

    VremTestHelper::DestroyTestWorld(World);
    return true;
}

// ============================================
// SetCurrentWeapon PrevOnHandDest Test
// ============================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEquipmentSetCurrentWeaponHolsteredDestTest,
    "Vrem.Equipment.SetCurrentWeapon.HolsteredDest",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter
)

bool FEquipmentSetCurrentWeaponHolsteredDestTest::RunTest(const FString& Parameters)
{
    UWorld* World = VremTestHelper::CreateTestWorld();
    AActor* Actor = VremEquipmentTestHelper::CreateActorWithEquipment(World);
    UVremEquipmentComponent* EquipComp = Actor->FindComponentByClass<UVremEquipmentComponent>();

    UVremEquipmentDefinition* Rifle = VremEquipmentTestHelper::CreateTestDefinition();
    UVremEquipmentDefinition* Knife = VremEquipmentTestHelper::CreateTestDefinition();

    EquipComp->TryEquipItem(Rifle, 1);
    EquipComp->SetCurrentWeapon(1);  // Rifle -> OnHand
    TestEqual(TEXT("Slot 1 should be OnHand"), EquipComp->GetEquipmentStateAtSlot(1), EEquipmentState::OnHand);

    EquipComp->TryEquipItem(Knife, 2);  // Knife -> Stowed
    TestEqual(TEXT("Slot 2 should be Stowed"), EquipComp->GetEquipmentStateAtSlot(2), EEquipmentState::Stowed);

    // Knife로 전환하면서 Rifle을 Holstered로 보냄
    EquipComp->SetCurrentWeapon(2, EEquipmentState::Holstered);
    TestEqual(TEXT("Slot 2 should be OnHand"), EquipComp->GetEquipmentStateAtSlot(2), EEquipmentState::OnHand);
    TestEqual(TEXT("Slot 1 should be Holstered"),EquipComp->GetEquipmentStateAtSlot(1), EEquipmentState::Holstered);

    VremTestHelper::DestroyTestWorld(World);
    return true;
}

// ============================================
// SetCurrentWeapon Demotion Test
// ============================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEquipmentHolsteredDemotionTest,
    "Vrem.Equipment.SetCurrentWeapon.HolsteredDemotion",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter
)

bool FEquipmentHolsteredDemotionTest::RunTest(const FString& Parameters)
{
    UWorld* World = VremTestHelper::CreateTestWorld();
    AActor* Actor = VremEquipmentTestHelper::CreateActorWithEquipment(World);
    UVremEquipmentComponent* EquipComp = Actor->FindComponentByClass<UVremEquipmentComponent>();

    UVremEquipmentDefinition* A = VremEquipmentTestHelper::CreateTestDefinition();
    UVremEquipmentDefinition* B = VremEquipmentTestHelper::CreateTestDefinition();
    UVremEquipmentDefinition* C = VremEquipmentTestHelper::CreateTestDefinition();

    // 초기 세팅: A(OnHand), B(Holstered), C(Stowed)
    EquipComp->TryEquipItem(A, 1);                               // A -> Stowed
    EquipComp->TryEquipItem(B, 2);                               // B -> Stowed
    EquipComp->TryEquipItem(C, 3);                               // C -> Stowed
    EquipComp->SetCurrentWeapon(2);                              // B -> OnHand
    EquipComp->SetCurrentWeapon(1, EEquipmentState::Holstered);  // A -> OnHand, B -> Holstered

    TestEqual(TEXT("Initial Setting: A OnHand"), EquipComp->GetEquipmentStateAtSlot(1), EEquipmentState::OnHand);
    TestEqual(TEXT("Initial Setting: B Holstered"), EquipComp->GetEquipmentStateAtSlot(2), EEquipmentState::Holstered);
    TestEqual(TEXT("Initial Setting: C Stowed"), EquipComp->GetEquipmentStateAtSlot(3), EEquipmentState::Stowed);

    // 이제 C를 장착 후 OnHand 장비를 Holstered 로 밀어냄 (C -> OnHand, A -> Holstered, B -> Stowed)
    EquipComp->SetCurrentWeapon(3, EEquipmentState::Holstered);

    TestEqual(TEXT("C OnHand"), EquipComp->GetEquipmentStateAtSlot(3), EEquipmentState::OnHand);
    TestEqual(TEXT("A (was OnHand) demoted to Holstered"), EquipComp->GetEquipmentStateAtSlot(1), EEquipmentState::Holstered);
    TestEqual(TEXT("B (was Holstered) demoted to Stowed"), EquipComp->GetEquipmentStateAtSlot(2), EEquipmentState::Stowed);

    VremTestHelper::DestroyTestWorld(World);
    return true;
}

// ============================================
// SetCurrentWeapon EquipmentState Swap Test
// ============================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEquipmentSwapHolsteredToOnHandTest,
    "Vrem.Equipment.SetCurrentWeapon.Swap",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter
)

bool FEquipmentSwapHolsteredToOnHandTest::RunTest(const FString& Parameters)
{
    UWorld* World = VremTestHelper::CreateTestWorld();
    AActor* Actor = VremEquipmentTestHelper::CreateActorWithEquipment(World);
    UVremEquipmentComponent* EquipComp = Actor->FindComponentByClass<UVremEquipmentComponent>();

    UVremEquipmentDefinition* A = VremEquipmentTestHelper::CreateTestDefinition();
    UVremEquipmentDefinition* B = VremEquipmentTestHelper::CreateTestDefinition();

    // 초기 세팅: A(OnHand), B(Holstered) 세팅
    EquipComp->TryEquipItem(A, 1);
    EquipComp->TryEquipItem(B, 2);
    EquipComp->SetCurrentWeapon(2);  // B -> OnHand
    EquipComp->SetCurrentWeapon(1, EEquipmentState::Holstered);  // A -> OnHand, B -> Holstered
    TestEqual(TEXT("Initial Setting: A should be OnHand"), EquipComp->GetEquipmentStateAtSlot(1), EEquipmentState::OnHand);
    TestEqual(TEXT("Initial Setting: B should be Holstered"), EquipComp->GetEquipmentStateAtSlot(2), EEquipmentState::Holstered);

    // Holster 상태의 B 장비를 OnHand로 이동 -> 원래 OnHand 상태였던 A 장비가 Holster로 이동
    EquipComp->SetCurrentWeapon(2, EEquipmentState::Holstered);

    TestEqual(TEXT("B (was Holstered) should now be OnHand"), EquipComp->GetEquipmentStateAtSlot(2), EEquipmentState::OnHand);
    TestEqual(TEXT("A (was OnHand) should now be Holstered"), EquipComp->GetEquipmentStateAtSlot(1), EEquipmentState::Holstered);

    VremTestHelper::DestroyTestWorld(World);
    return true;
}
#endif // WITH_AUTOMATION_WORKER
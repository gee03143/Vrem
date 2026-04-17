// Fill out your copyright notice in the Description page of Project Settings.

#if WITH_AUTOMATION_WORKER

#include "VremInventoryComponentTest.h"
#include "Misc/AutomationTest.h"
#include "VremTest.h"
#include "Vrem/Inventory/VremInventoryComponent.h"
#include "Vrem/Inventory/VremItemDefinition.h"

namespace VremInventoryTestHelper
{
    AActor* CreateActorWithInventory(UWorld* World)
    {
        AActor* Actor = World->SpawnActor<AActor>();
        UVremInventoryComponent* Comp = NewObject<UVremInventoryComponent>(Actor);
        Comp->RegisterComponent();
        return Actor;
    }

    UVremItemDefinition* CreateTestItemDefinition(FName Name = TEXT("TestItem"))
    {
        UVremItemDefinition* Def = NewObject<UVremItemDefinition>();
        return Def;
    }
}

// ============================================
// 아이템 추가 테스트
// ============================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FInventoryAddItemTest,
    "Vrem.Inventory.AddItem",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter
)

bool FInventoryAddItemTest::RunTest(const FString& Parameters)
{
    UWorld* World = VremTestHelper::CreateTestWorld();
    AActor* Actor = VremInventoryTestHelper::CreateActorWithInventory(World);
    UVremInventoryComponent* InvComp = Actor->FindComponentByClass<UVremInventoryComponent>();
    InvComp->InitializeFromOwner();

    UVremItemDefinition* ItemDef = VremInventoryTestHelper::CreateTestItemDefinition();

    // 추가 전
    TestEqual(TEXT("Initial item count should be 0"), InvComp->GetInventoryItemNum(), 0);

    // 아이템 추가
    InvComp->TestAddItem(ItemDef);
    TestEqual(TEXT("Item count should be 1"), InvComp->GetInventoryItemNum(), 1);

    VremTestHelper::DestroyTestWorld(World);
    return true;
}

// ============================================
// 동일 아이템 스택 테스트
// ============================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FInventoryStackItemTest,
    "Vrem.Inventory.StackItem",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter
)

bool FInventoryStackItemTest::RunTest(const FString& Parameters)
{
    UWorld* World = VremTestHelper::CreateTestWorld();
    AActor* Actor = VremInventoryTestHelper::CreateActorWithInventory(World);
    UVremInventoryComponent* InvComp = Actor->FindComponentByClass<UVremInventoryComponent>();
    InvComp->InitializeFromOwner();

    UVremItemDefinition* ItemDef = VremInventoryTestHelper::CreateTestItemDefinition();

    InvComp->TestAddItem(ItemDef);
    InvComp->TestAddItem(ItemDef);

    // 같은 아이템이면 Entry는 1개, Count가 2
    TestEqual(TEXT("Entry count should be 1"), InvComp->GetInventoryItemNum(), 1);

    VremTestHelper::DestroyTestWorld(World);
    return true;
}

// ============================================
// 아이템 제거 테스트
// ============================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FInventoryRemoveItemTest,
    "Vrem.Inventory.RemoveItem",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter
)

bool FInventoryRemoveItemTest::RunTest(const FString& Parameters)
{
    UWorld* World = VremTestHelper::CreateTestWorld();
    AActor* Actor = VremInventoryTestHelper::CreateActorWithInventory(World);
    UVremInventoryComponent* InvComp = Actor->FindComponentByClass<UVremInventoryComponent>();
    InvComp->InitializeFromOwner();

    // 빈 인벤토리에서 제거 시도 → 크래시 없어야 함
    FPrimaryAssetId FakeId(TEXT("VremItemDefinition"), TEXT("FakeItem"));
    InvComp->TestRemoveItem(FakeId);
    TestEqual(TEXT("Count should remain 0"), InvComp->GetInventoryItemNum(), 0);

    UVremItemDefinition* ItemDef = VremInventoryTestHelper::CreateTestItemDefinition();

    InvComp->TestAddItem(ItemDef, 3);
    TestEqual(TEXT("Entry count should be 1"), InvComp->GetInventoryItemNum(), 1);

    // Count 3 → 2 → 1 → 0 (제거)
    InvComp->TestRemoveItem(ItemDef->GetPrimaryAssetId());
    TestEqual(TEXT("Entry should still exist at count 2"), InvComp->GetInventoryItemNum(), 1);

    InvComp->TestRemoveItem(ItemDef->GetPrimaryAssetId());
    InvComp->TestRemoveItem(ItemDef->GetPrimaryAssetId());
    TestEqual(TEXT("Entry should be removed at count 0"), InvComp->GetInventoryItemNum(), 0);

    VremTestHelper::DestroyTestWorld(World);
    return true;
}

// ============================================
// 델리게이트 호출 테스트
// ============================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FInventoryDelegateTest,
    "Vrem.Inventory.Delegate",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter
)

bool FInventoryDelegateTest::RunTest(const FString& Parameters)
{
    UWorld* World = VremTestHelper::CreateTestWorld();
    AActor* Actor = VremInventoryTestHelper::CreateActorWithInventory(World);
    UVremInventoryComponent* InvComp = Actor->FindComponentByClass<UVremInventoryComponent>();
    InvComp->InitializeFromOwner();

    bool bDelegateFired = false;
    InvComp->OnItemInstanceCreated.AddLambda([&bDelegateFired](UVremItemInstance*)
        {
            bDelegateFired = true;
        });

    UVremItemDefinition* ItemDef = VremInventoryTestHelper::CreateTestItemDefinition();
    InvComp->TestAddItem(ItemDef);

    TestTrue(TEXT("OnItemInstanceCreated should have fired"), bDelegateFired);

    FPrimaryAssetId RemovedId;
    InvComp->OnItemInstanceRemoved.AddLambda([&RemovedId](const FPrimaryAssetId& ItemId)
        {
            RemovedId = ItemId;
        });

    FPrimaryAssetId ExpectedId = ItemDef->GetPrimaryAssetId();
    InvComp->TestRemoveItem(ExpectedId);

    TestEqual(TEXT("OnItemInstanceRemoved should broadcast correct ItemId"), RemovedId, ExpectedId);

    VremTestHelper::DestroyTestWorld(World);
    return true;
}

// ============================================
// 복제 시뮬레이션 테스트
// ============================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FInventoryReplicationSimTest,
    "Vrem.Inventory.ReplicationSim",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter
)

bool FInventoryReplicationSimTest::RunTest(const FString& Parameters)
{
    UWorld* World = VremTestHelper::CreateTestWorld();

    AActor* ServerActor = VremInventoryTestHelper::CreateActorWithInventory(World);
    UVremInventoryComponent* ServerComp = ServerActor->FindComponentByClass<UVremInventoryComponent>();
    ServerComp->InitializeFromOwner();

    AActor* ClientActor = VremInventoryTestHelper::CreateActorWithInventory(World);
    UVremInventoryComponent* ClientComp = ClientActor->FindComponentByClass<UVremInventoryComponent>();
    ClientComp->InitializeFromOwner();

    UVremItemDefinition* ItemDef = VremInventoryTestHelper::CreateTestItemDefinition();

    ServerComp->TestAddItem(ItemDef, 2);
    TestEqual(TEXT("Server should have 1 entry"), ServerComp->GetInventoryItemNum(), 1);
    TestEqual(TEXT("Client should have 0 entries before replication"), ClientComp->GetInventoryItemNum(), 0);

    ClientComp->SimulateReplicateFrom(ServerComp);
    TestEqual(TEXT("Client should have 1 entry after replication"), ClientComp->GetInventoryItemNum(), 1);

    VremTestHelper::DestroyTestWorld(World);
    return true;
}

#endif // WITH_AUTOMATION_WORKER
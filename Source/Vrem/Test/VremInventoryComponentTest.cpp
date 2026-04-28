// Fill out your copyright notice in the Description page of Project Settings.

#if WITH_AUTOMATION_WORKER

#include "VremInventoryComponentTest.h"
#include "Misc/AutomationTest.h"
#include "VremTest.h"
#include "Vrem/Inventory/VremInventoryComponent.h"

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
// ������ �߰� �׽�Ʈ
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

    UVremItemDefinition* ItemDef = VremInventoryTestHelper::CreateTestItemDefinition();

    // �߰� ��
    TestEqual(TEXT("Initial item count should be 0"), InvComp->GetInventoryItemNum(), 0);

    // ������ �߰�
    InvComp->TestAddItem(ItemDef);
    TestEqual(TEXT("Item count should be 1"), InvComp->GetInventoryItemNum(), 1);

    VremTestHelper::DestroyTestWorld(World);
    return true;
}

// ============================================
// ���� ������ ���� �׽�Ʈ
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

    UVremItemDefinition* ItemDef = VremInventoryTestHelper::CreateTestItemDefinition();

    InvComp->TestAddItem(ItemDef);
    InvComp->TestAddItem(ItemDef);

    // ���� �������̸� Entry�� 1��, Count�� 2
    TestEqual(TEXT("Entry count should be 1"), InvComp->GetInventoryItemNum(), 1);

    VremTestHelper::DestroyTestWorld(World);
    return true;
}

// ============================================
// ������ ���� �׽�Ʈ
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

    // �� �κ��丮���� ���� �õ� -> ũ���� ����� ��
    FPrimaryAssetId FakeId(TEXT("VremItemDefinition"), TEXT("FakeItem"));
    InvComp->TestRemoveItem(FakeId);
    TestEqual(TEXT("Count should remain 0"), InvComp->GetInventoryItemNum(), 0);

    UVremItemDefinition* ItemDef = VremInventoryTestHelper::CreateTestItemDefinition();

    InvComp->TestAddItem(ItemDef, 3);
    TestEqual(TEXT("Entry count should be 1"), InvComp->GetInventoryItemNum(), 1);

    // Count 3 -> 2 -> 1 -> 0 (����)
    InvComp->TestRemoveItem(ItemDef->GetPrimaryAssetId());
    TestEqual(TEXT("Entry should still exist at count 2"), InvComp->GetInventoryItemNum(), 1);

    InvComp->TestRemoveItem(ItemDef->GetPrimaryAssetId());
    InvComp->TestRemoveItem(ItemDef->GetPrimaryAssetId());
    TestEqual(TEXT("Entry should be removed at count 0"), InvComp->GetInventoryItemNum(), 0);

    VremTestHelper::DestroyTestWorld(World);
    return true;
}

// ============================================
// ��������Ʈ ȣ�� �׽�Ʈ
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

    UVremInventoryTestListener* Listener = NewObject<UVremInventoryTestListener>(InvComp);
    InvComp->OnItemInstanceCreated.AddDynamic(Listener, &UVremInventoryTestListener::HandleItemInstanceCreated);
    InvComp->OnItemInstanceRemoved.AddDynamic(Listener, &UVremInventoryTestListener::HandleItemInstanceRemoved);

    UVremItemDefinition* ItemDef = VremInventoryTestHelper::CreateTestItemDefinition();
    InvComp->TestAddItem(ItemDef);

    TestTrue(TEXT("OnItemInstanceCreated should have fired"), Listener->bCreatedFired);

    FString ExpectedDefName = ItemDef->GetName();
    InvComp->TestRemoveItem(ItemDef->GetPrimaryAssetId());

    TestEqual(TEXT("OnItemInstanceRemoved should broadcast correct Definition"), Listener->LastRemovedDef, ExpectedDefName);

    VremTestHelper::DestroyTestWorld(World);
    return true;
}

// ============================================
// ���� �ùķ��̼� �׽�Ʈ
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

    AActor* ClientActor = VremInventoryTestHelper::CreateActorWithInventory(World);
	UVremInventoryComponent* ClientComp = ClientActor->FindComponentByClass<UVremInventoryComponent>();

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
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "VremSkillBehavior.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "VremSkillComponent.generated.h"

class UVremSkillComponent;
class UVremSkillDefinition;
class AController;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSkillActivated, int32, SlotIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSkillListChanged);

USTRUCT()
struct FSkillEntry : public FFastArraySerializerItem
{
    GENERATED_BODY()

    UPROPERTY()
	TWeakObjectPtr<const UVremSkillDefinition> SkillDefinition;

    UPROPERTY()
    int32 SkillSlotIndex = INDEX_NONE;

    // Local Cooldown 종료 시각 (World seconds)
    UPROPERTY(NotReplicated, Transient)
    float CooldownEndTime = 0.f;

    bool operator==(const FSkillEntry& Other) const
    {
        return SkillSlotIndex == Other.SkillSlotIndex;
    }
};

USTRUCT()
struct FSkillList : public FFastArraySerializer
{
    GENERATED_BODY()

    void SetOwner(UVremSkillComponent* InOwner);

    FSkillEntry* GetEntryFromIndex(const int32 InIndex)
    {
        return const_cast<FSkillEntry*>(static_cast<const FSkillList*>(this)->GetEntryFromIndex(InIndex));
    }

    const FSkillEntry* GetEntryFromIndex(const int32 InSlotIndex) const
    {
        const FSkillEntry* FoundEntry = Entries.FindByPredicate(
            [InSlotIndex](const FSkillEntry& Entry)
            {
                return Entry.SkillSlotIndex == InSlotIndex;
            });

        return FoundEntry;
    }

    void AddEntry(const UVremSkillDefinition* InSkillDefinition, int32 InSlotIndex);
    void RemoveEntry(int32 InSlotIndex);
    int32 GetNumEntries() const { return Entries.Num(); }

    void PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize);
    void PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize);
    void PreReplicatedRemove(const TArrayView<int32>& RemovedIndices, int32 FinalSize);

    bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParams)
    {
        return FastArrayDeltaSerialize<FSkillEntry, FSkillList>(Entries, DeltaParams, *this);
    }

private:
    UPROPERTY()
    TArray<FSkillEntry> Entries;

    TWeakObjectPtr<UVremSkillComponent> OwnerComponent;
};

template<>
struct TStructOpsTypeTraits<FSkillList> : public TStructOpsTypeTraitsBase2<FSkillList>
{
    enum
    {
        WithNetDeltaSerializer = true,
    };
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class VREM_API UVremSkillComponent : public UActorComponent
{
	GENERATED_BODY()

    friend struct FSkillList;

public:	
	// Sets default values for this component's properties
	UVremSkillComponent();

protected:
    virtual void InitializeComponent() override;
    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;

    // Blueprint API
public:
    // Add/Remove
    UFUNCTION(BlueprintCallable, Category = "Vrem|Skill")
    void RequestAddSkill(const UVremSkillDefinition* InSkillDefinition, int32 InSlotIndex);
    UFUNCTION(BlueprintCallable, Category = "Vrem|Skill")
    void RequestRemoveSkill(int32 InSlotIndex);

    // Targeting
    UFUNCTION(BlueprintCallable, Category = "Vrem|Skill")
    void RequestCancelTargeting();

    // Activation
    UFUNCTION(BlueprintCallable, Category = "Vrem|Skill")
    void RequestActivateSkill(int32 SlotIndex);

    // Query
    UFUNCTION(BlueprintPure, Category = "Vrem|Skill")
    bool IsTargeting() const { return CurrentTargetingSlotIndex != INDEX_NONE; }
    UFUNCTION(BlueprintPure, Category = "Vrem|Skill")
    int32 GetTargetingSlotIndex() const { return CurrentTargetingSlotIndex; }
    UFUNCTION(BlueprintPure, Category = "Vrem|Skill")
    bool CanActivateSkill(int32 SlotIndex) const;
    UFUNCTION(BlueprintPure, Category = "Vrem|Skill")
    float GetCooldownRemaining(int32 SlotIndex) const;
    UFUNCTION(BlueprintPure, Category = "Vrem|Skill")
    int32 GetSkillNum() const { return SkillList.GetNumEntries(); }
    UFUNCTION(BlueprintPure, Category = "Vrem|Skill")
    const UVremSkillDefinition* GetSkillDefinition(int32 SlotIndex) const;
    UPROPERTY(BlueprintAssignable, Category = "Vrem|Skill")
    FOnSkillActivated OnSkillActivated;
    UPROPERTY(BlueprintAssignable, Category = "Vrem|Skill")
    FOnSkillListChanged OnSkillListChanged;
    // RPC
protected:
    UFUNCTION(Server, Reliable)
    void ServerActivateSkill(int32 SlotIndex, FVector AimLocation, FVector AimDirection);

    UFUNCTION(Server, Reliable)
    void ServerAddSkill(const UVremSkillDefinition* SkillDefinition, int32 SlotIndex);

    UFUNCTION(Server, Reliable)
    void ServerRemoveSkill(int32 SlotIndex);

public:
	void AddSkill(const UVremSkillDefinition* InSkillDefinition, int32 InSlotIndex);
	void RemoveSkill(int32 InSlotIndex);

protected:
	void ExecuteActivateSkill(int32 SlotIndex, const FVremSkillActivationContext& Context);
    void StartCooldownLocal(int32 SlotIndex);

    void BeginTargetingLocal(int32 SlotIndex);
    void ConfirmTargetingLocal();
    void CancelTargetingLocal();

    void NotifySkillListChanged() { OnSkillListChanged.Broadcast(); }
protected:
    UFUNCTION()
    void OnRep_SkillList();

private:
    FVremSkillActivationContext BuildActivationContext() const;
	AController* GetInstigatorController() const;

protected:
	UPROPERTY(EditDefaultsOnly)
	TArray<UVremSkillDefinition*> DefaultGrantedSkills; 

private:
	UPROPERTY(ReplicatedUsing = OnRep_SkillList)
    FSkillList SkillList;

    int32 CurrentTargetingSlotIndex = INDEX_NONE;
    FVremSkillActivationContext CurrentTargetingContext;
};

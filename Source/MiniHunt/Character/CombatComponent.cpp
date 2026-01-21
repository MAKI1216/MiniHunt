// Fill out your copyright notice in the Description page of Project Settings.


#include "MiniHunt/Character/CombatComponent.h"

#include "HunterCharacterBase.h"
#include "MiniHunt/Weapon/WeaponBase.h"

#pragma region Engine
class AHunterCharacterBase;
// Sets default values for this component's properties
UCombatComponent::UCombatComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
}


// Called every frame
void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                     FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}
#pragma endregion

#pragma region PickAndEquip
void UCombatComponent::PickupWeapon(AWeaponBase* WeaponToPick, USceneComponent* Parent1P, USceneComponent* Parent3P)
{
	if (WeaponToPick == nullptr) return;

	// 情况 1: 1号位是空的 -> 放入1号位并自动装备
	if (GunNO1 == nullptr)
	{
		GunNO1 = WeaponToPick;
		ServerPickupWeapon(GunNO1);
	}
	// 情况 2: 1号位有枪，2号位空的 -> 放入2号位 (但不一定马上切枪，或者你想直接切过去也行)
	else if (GunNO2 == nullptr)
	{
		GunNO2 = WeaponToPick;
		// 如果你想捡起来就装备，这里也调用 EquipWeapon
		// 这里假设只捡不换：我们需要把枪挂到背上或者隐藏起来
		// 简单起见，这里演示直接装备新捡的枪：
		ServerPickupWeapon(GunNO2);
	}
	// 情况 3: 两个位置都满了
	else
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red, TEXT("无法拾取更多的枪 (Inventory Full)"));
		}
		return; // 退出，不执行装备
	}
}

// 辅助函数：执行真正的装备逻辑
void UCombatComponent::EquipWeapon(AWeaponBase* WeaponToEquip, USceneComponent* Parent1P, USceneComponent* Parent3P)
{
	if (WeaponToEquip == nullptr) return;

	// 1. 设置当前持有的武器
	EquippedWeaponBase = WeaponToEquip;

	// 2. 设置武器的所有者 (Owner) 为角色，这就触发了网络同步
	EquippedWeaponBase->SetOwner(GetOwner());

	// 3. 调用武器自己的 Equip，让它挂载到模型上
	EquippedWeaponBase->Equip(Parent1P, Parent3P);

	// 4. (可选) 如果你之前手里有枪，需要把旧枪隐藏或者挂到背上
	// ...
}

void UCombatComponent::ServerPickupWeapon_Implementation(AWeaponBase* WeaponToPick)
{
	if (WeaponToPick == nullptr) return;

	// 1. 竞态条件检查 (防止两个人同时捡)
	if (WeaponToPick->GetWeaponState() == EWeaponState::EWS_Equipped || WeaponToPick->GetOwner() != nullptr)
	{
		return;
	}

	// 2. 放入背包逻辑 (确定是几号位)
	if (GunNO1 == nullptr)
	{
		GunNO1 = WeaponToPick;
	}
	else if (GunNO2 == nullptr)
	{
		GunNO2 = WeaponToPick;
	}
	else
	{
		return; // 背包满了
	}

	// 3. 确立所有权 (必须做)
	WeaponToPick->SetOwner(GetOwner());
   
	// 4. 修改状态 (通知客户端关闭物理模拟)
	// 注意：这会让所有客户端执行 OnRep，关闭碰撞和物理
	WeaponToPick->SetWeaponState(EWeaponState::EWS_Equipped);

	// === 【核心逻辑修正】 ===
   
	// 场景 A: 如果当前手里没有枪 -> 马上装备这把新枪
	if (EquippedWeaponBase == nullptr) // 必须是 == nullptr
		{
		EquippedWeaponBase = WeaponToPick;
      
		if (AHunterCharacterBase* Character = Cast<AHunterCharacterBase>(GetOwner()))
		{
			// 装备到手上 (WeaponSocket)
			WeaponToPick->Equip(Character->GetFPArmMesh(), Character->GetMesh());
		}
		}
	// 场景 B: 如果手里已经有枪了 -> 只捡不换
	else
	{
		// ⚠️ 极其重要：
		// 虽然不装备到手上，但必须把这把枪 Attach 到角色身上！
		// 否则角色走了，枪还留在原地（虽然物理关了，但位置没变）。
		// 我们可以把它挂载到身体上，但设为隐藏，或者挂载到背后的 Socket。
      
		if (AHunterCharacterBase* Character = Cast<AHunterCharacterBase>(GetOwner()))
		{
			// 这里的 Equip 实现需要稍微改一下支持备用武器，或者直接在这里手动 Attach
			// 简单做法：先挂到背上或者直接隐藏
			WeaponToPick->GetWeaponMesh3P()->AttachToComponent(
				Character->GetMesh(), 
				FAttachmentTransformRules::SnapToTargetNotIncludingScale, 
				FName("BackpackSocket") // 假设你以后想做背枪效果
			);
          
			// 如果不想做背枪，直接隐藏它
			 WeaponToPick->SetActorHiddenInGame(true); 
		}
	}
}

bool UCombatComponent::ServerPickupWeapon_Validate(AWeaponBase* WeaponToPick)
{
	return true;
}

#pragma endregion

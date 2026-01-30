// Fill out your copyright notice in the Description page of Project Settings.


#include "MiniHunt/Character/CombatComponent.h"

#include "HunterCharacterBase.h"
#include "Components/DecalComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "MiniHunt/Controller/HunterPlayerController.h"
#include "MiniHunt/Item/AmmoItem.h"
#include "MiniHunt/Item/ItemBase.h"
#include "MiniHunt/Item/ScoreItem.h"
#include "MiniHunt/Monster/MonsterBase.h"
#include "MiniHunt/Weapon/WeaponBase.h"
#include "Net/UnrealNetwork.h"
#include "PhysicalMaterials/PhysicalMaterial.h"

#pragma region Engine
class AHunterCharacterBase;
// Sets default values for this component's properties
UCombatComponent::UCombatComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	//自定义组件默认不开启复制，手动开启复制
	SetIsReplicatedByDefault(true);
}

// Called when the game starts
void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	// 初始化背包子弹数组
	// 0号表示步枪子弹，1号表示手枪子弹，2号表示狙击枪子弹
	// 只在服务器上初始化数据
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		BagBulletCounts.Empty();

		// Index 0: Rifle
		FBagBulletInfo RifleInfo;
		RifleInfo.BulletType = EBulletType::EBT_Rifle;
		RifleInfo.Count = 0;
		BagBulletCounts.Add(RifleInfo);

		// Index 1: Pistol
		FBagBulletInfo PistolInfo;
		PistolInfo.BulletType = EBulletType::EBT_Pistol;
		PistolInfo.Count = 0;
		BagBulletCounts.Add(PistolInfo);

		// Index 2: Sniper
		FBagBulletInfo SniperInfo;
		SniperInfo.BulletType = EBulletType::EBT_Sniper;
		SniperInfo.Count = 0;
		BagBulletCounts.Add(SniperInfo);
	}
	
	// =========================================================
	// DEBUG LOGIC (测试逻辑)
	// =========================================================
	// 只有服务器有权修改子弹数量
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		// 检查布尔变量是否打勾
		if (bDebug_GiveMaxAmmo)
		{
			// 遍历背包里所有的子弹类型，全部改成 999
			for (int32 i = 0; i < BagBulletCounts.Num(); i++)
			{
				BagBulletCounts[i].Count = 999;
			}

			// (可选) 打印一条红色的警告，提醒自己还没关掉它
			GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, TEXT("【警告】测试作弊模式已开启：无限子弹！"));
		}
	}
	// // 绑定到 Owner 的委托
	// if (AActor* Owner = GetOwner())
	// {
	// 	Owner->OnTakePointDamage.AddDynamic(this, &UCombatComponent::OnHit);
	// }
	
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		CurrentHealth = MaxHealth;
		// 监听宿主的受伤事件
		GetOwner()->OnTakeAnyDamage.AddDynamic(this, &UCombatComponent::OnTakeAnyDamage);
	}
}


// Called every frame
void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                     FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}


void UCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	// 修复变量名 BagBulletCountMap -> BagBulletCounts
	DOREPLIFETIME_CONDITION(UCombatComponent, BagBulletCounts, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UCombatComponent, CurrentHealth, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UCombatComponent, MaxHealth, COND_OwnerOnly);

	DOREPLIFETIME(UCombatComponent, EquippedWeaponBase);
}

#pragma endregion

#pragma region BasicComabt

// 重置血量的函数
void UCombatComponent::ResetHealth()
{
	CurrentHealth = MaxHealth;
	// 还需要通知 UI 更新
	ClientUpdateHealthUI(CurrentHealth, MaxHealth);
}

//收到伤害的函数，由服务器调用
void UCombatComponent::OnTakeAnyDamage(AActor* DamagedActor, float Damage, const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser)
{
	// ➤➤➤ 【修改】使用屏幕打印，确保证据确凿
	if (GetOwner()->HasAuthority())
	{
		FString DebugMsg = FString::Printf(TEXT("【服务器判定】收到伤害: %f | 当前血量: %f"), Damage, CurrentHealth - Damage);
		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red, DebugMsg);
	}
	
	if (CurrentHealth <= 0) return;

	CurrentHealth -= Damage;
    
	// 调用 Character 的 UI 更新
	ClientUpdateHealthUI(CurrentHealth, MaxHealth);

	if (CurrentHealth <= 0)
	{
		CurrentHealth = 0;
		// 死亡！
		if (AHunterCharacterBase* Char = Cast<AHunterCharacterBase>(GetOwner()))
		{
			Char->HandleDeath(DamageCauser);
		}
	}
}

//施加伤害的函数，由服务器调用
void UCombatComponent::TakeDamage(UPhysicalMaterial* PhysicalMaterial, AActor* DamagedActor, FVector HitFromDirection,
                                  FHitResult& HitInfo)
{
	// 1. 检查必要参数
	if (!DamagedActor || !EquippedWeaponBase)
	{
		return;
	}

	// 2. 转化当前武器为枪
	AGunBase* GunWeapon = Cast<AGunBase>(EquippedWeaponBase);
	if (GunWeapon)
	{
		// 3. 计算伤害倍率
		float DamageMultiplier = 1.0f;

		//todo 在ue编辑器中设置不同物理材质
		// 检查物理材质是否有效
		if (PhysicalMaterial)
		{
			switch (PhysicalMaterial->SurfaceType)
			{
			case EPhysicalSurface::SurfaceType1: // Head
				DamageMultiplier = 4.0f;
				break;
			case EPhysicalSurface::SurfaceType2: // Body
				DamageMultiplier = 1.0f;
				break;
			case EPhysicalSurface::SurfaceType3: // Arm
				DamageMultiplier = 0.8f;
				break;
			case EPhysicalSurface::SurfaceType4: // Leg
				DamageMultiplier = 0.7f;
				break;
			default:
				DamageMultiplier = 1.0f; // 默认材质
				break;
			}
		}
		else
		{
			// 如果没有物理材质，默认给 1 倍伤害
			DamageMultiplier = 1.0f;
		}

		// 4. 应用伤害
		UGameplayStatics::ApplyPointDamage(
			DamagedActor,
			GunWeapon->BaseDamage * DamageMultiplier,
			HitFromDirection,
			HitInfo,
			GetOwner()->GetInstigatorController(), // 使用 InstigatorController 更稳健
			GetOwner(),
			UDamageType::StaticClass()
		);
	}
	// TODO: 留出空位给以后做刀的伤害或者手榴弹的伤害
	/*
	else if (AKnifeBase* KnifeWeapon = Cast<AKnifeBase>(EquippedWeaponBase))
	{
		// 刀的伤害逻辑
	}
	else if (AGrenadeBase* GrenadeWeapon = Cast<AGrenadeBase>(EquippedWeaponBase))
	{
		// 手榴弹的伤害逻辑
	}
	*/
}

//ApplyPointDamage的回调函数，同样在server执行
// void UCombatComponent::OnHit(AActor* DamagedActor, float Damage, class AController* InstigatedBy,
//                              FVector HitLocation, class UPrimitiveComponent* FHitComponent, FName BoneName,
//                              FVector ShotFromDirection, const class UDamageType* DamageType, AActor* DamageCauser)
// {
// 	CurrentHealth -= Damage;
// 	//发起客户端rpc，修改ui
// 	ClientUpdateHealthUI(CurrentHealth, MaxHealth);
// 	if (CurrentHealth <= 0)
// 	{
// 		//TODO:死亡逻辑
// 		//DeathMatchDeath(DamageCauser);
// 	}
// }

//修改血量ui的客户端rpc
void UCombatComponent::ClientUpdateHealthUI_Implementation(float NewCurrentHealth, float NewMaxHealth)
{
	//获得controller并转化为hunterplayercontroller
	if (AHunterCharacterBase* Char = Cast<AHunterCharacterBase>(GetOwner()))
	{
		if (AHunterPlayerController* PC = Cast<AHunterPlayerController>(Char->GetController()))
		{
			// 这行代码现在会在客户端本地运行，所以能成功刷新 UI
			PC->UpdateHealthUI(NewCurrentHealth, NewMaxHealth);
		}
	}
}

#pragma endregion

#pragma region PickAndEquipAndDrop

void UCombatComponent::PickupWeapon(AWeaponBase* WeaponToPick, USceneComponent* Parent1P, USceneComponent* Parent3P)
{
	if (WeaponToPick == nullptr) return;

	// 情况 1: 1号位是空的 
	if (GunNO1 == nullptr)
	{
		// ✅ 修正：把真正要捡的枪传给服务器
		ServerPickupWeapon(WeaponToPick);
	}
	// 情况 2: 1号位有枪，2号位空的 
	else if (GunNO2 == nullptr)
	{
		// ✅ 修正：把真正要捡的枪传给服务器
		ServerPickupWeapon(WeaponToPick);
	}
	else
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red, TEXT("无法拾取更多的枪 (Inventory Full)"));
		}
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

	// 设置状态为 PickingUp
	AHunterCharacterBase* Character = Cast<AHunterCharacterBase>(GetOwner());
	if (Character)
    {
        // 如果正在丢枪、换弹、切枪，禁止捡枪
        if (Character->CurrentCharacterState != EHunterCharacterState::Idle) // 如果允许射击时捡枪可以保留 GunFiring，否则最好只允许 Idle
        {
             return; 
        }
    }
	if (Character)
	{
		Character->CurrentCharacterState = EHunterCharacterState::PickingUp;
	}

	AGunBase* GunToPick = Cast<AGunBase>(WeaponToPick); // 转化为 GunBase 以获取图标
	int32 TargetSlotIndex = -1; // 记录放到哪个槽位了

	// 2. 放入背包逻辑 (确定是几号位)
	if (GunNO1 == nullptr)
	{
		GunNO1 = WeaponToPick;
		TargetSlotIndex = 0; // 放入 1 号位
	}
	else if (GunNO2 == nullptr)
	{
		GunNO2 = WeaponToPick;
		TargetSlotIndex = 1; // 放入 2 号位
	}
	else
	{
		// 如果背包满了，记得把状态改回 Idle
		if (Character)
		{
			Character->CurrentCharacterState = EHunterCharacterState::Idle;
		}
		return; // 背包满了
	}

	// 3. 确立所有权 (必须做)
	WeaponToPick->SetOwner(GetOwner());

	// 4. 修改状态 (通知客户端关闭物理模拟)
	// 注意：这会让所有客户端执行 OnRep，关闭碰撞和物理


	// === 【核心逻辑修正】 ===

	// 场景 A: 如果当前手里没有枪 -> 马上装备这把新枪
	if (EquippedWeaponBase == nullptr) // 必须是 == nullptr
	{
		WeaponToPick->SetWeaponState(EWeaponState::EWS_Equipped);
		EquippedWeaponBase = WeaponToPick;

		if (Character)
		{
			// 装备到手上 (WeaponSocket)
			WeaponToPick->Equip(Character->GetFPArmMesh(), Character->GetMesh());
		}
		
		// 客户端rpc更新武器图标ui
		if (GunToPick)
		{
			// 客户端rpc更新武器图标ui
			ClientUpdateWeaponIconUI(TargetSlotIndex, GunToPick, 1);
		}
	}
	// 场景 B: 如果手里已经有枪了 -> 只捡不换
	else
	{
		WeaponToPick->SetWeaponState(EWeaponState::EWS_PossesButNotMoving);
		// ⚠️ 极其重要：
		// 虽然不装备到手上，但必须把这把枪 Attach 到角色身上！
		// 否则角色走了，枪还留在原地（虽然物理关了，但位置没变）。
		// 我们可以把它挂载到身体上，但设为隐藏，或者挂载到背后的 Socket。

		if (Character)
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
			
			// 客户端rpc更新武器图标ui
			if (GunToPick)
			{
				// 客户端rpc更新武器图标ui
				ClientUpdateWeaponIconUI(TargetSlotIndex, GunToPick, 0.5);
			}
		}
	}

	// 动作结束，改回 Idle
	if (Character)
	{
		Character->CurrentCharacterState = EHunterCharacterState::Idle;
	}
}

bool UCombatComponent::ServerPickupWeapon_Validate(AWeaponBase* WeaponToPick)
{
	return true;
}

//切枪
void UCombatComponent::ServerSwapWeapon_Implementation(int32 SlotIndex)
{
    // 1. 基本检查
    AHunterCharacterBase* Character = Cast<AHunterCharacterBase>(GetOwner());
	
	if (!Character) return;
    
	if (Character->CurrentCharacterState == EHunterCharacterState::WeaponSwitching ||
		Character->CurrentCharacterState == EHunterCharacterState::GunReloading ||  // <--- 禁止换弹时切枪
		Character->CurrentCharacterState == EHunterCharacterState::PickingUp)       // <--- 禁止捡东西时切枪
	{
		return;
	}
    // 2. 确定要切换的目标武器
    AWeaponBase* NewWeapon = nullptr;
    if (SlotIndex == 0) NewWeapon = GunNO1;
    else if (SlotIndex == 1) NewWeapon = GunNO2;

    // 3. 逻辑判断
    // 如果该槽位没枪，或者是当前手里的枪，直接返回
    if (NewWeapon == nullptr || NewWeapon == EquippedWeaponBase) 
    {
        return; 
    }

    // 4. 设置角色状态为 "正在切枪" (同步给客户端)
    Character->CurrentCharacterState = EHunterCharacterState::WeaponSwitching;
    // 停止当前的射击（如果按住左键切枪）
    StopFire(); 

    // 5. 【处理旧武器】 -> 放到背上
    if (EquippedWeaponBase)
    {
        // 设置状态为 "持有但未装备" -> 这会触发 OnRep，隐藏1P，挂载3P到背部
        EquippedWeaponBase->SetWeaponState(EWeaponState::EWS_PossesButNotMoving);
    }

	if (AGunBase* NewGun = Cast<AGunBase>(NewWeapon))
	{
		// 这一步非常重要：它会更新 Character 的动画状态机变量 (IsWithGun, GunType)
		// 从而让动画蓝图从 "空手" 切换到 "持枪姿势"
		Character->SetGunTypeAndIsWithGun(true, NewGun->GunType);
	}
	
    // 6. 【处理新武器】 -> 拿到手上
    // 更新引用
    EquippedWeaponBase = NewWeapon;
    // 设置状态为 "装备中" -> 这会触发 OnRep，显示1P/3P，挂载到手部
    EquippedWeaponBase->SetWeaponState(EWeaponState::EWS_Equipped);
    
    // 确保所有权正确 (防止丢包导致的 Owner 丢失)
    EquippedWeaponBase->SetOwner(Character);

    // 7. 【更新 UI】
    // 更新 Icon 高亮 (0号位)
    if (GunNO1) 
    {
        float Opacity = (SlotIndex == 0) ? 1.0f : 0.5f;
        ClientUpdateWeaponIconUI(0, Cast<AGunBase>(GunNO1), Opacity);
    }
    // 更新 Icon 高亮 (1号位)
    if (GunNO2)
    {
        float Opacity = (SlotIndex == 1) ? 1.0f : 0.5f;
        ClientUpdateWeaponIconUI(1, Cast<AGunBase>(GunNO2), Opacity);
    }

    // 更新弹药数 (如果是枪)
    if (AGunBase* NewGun = Cast<AGunBase>(NewWeapon))
    {
        int32 BagAmmo = GetBagBulletCount(NewGun->BulletType);
        NewGun->ClientUpdateAmmoUI(NewGun->ClipCurrentAmmo, NewGun->ClipMaxAmmo, BagAmmo);
    }

    // 8. 设置定时器，模拟切枪动画时间 (比如 0.5秒后切回 Idle)
    // 如果你有切枪蒙太奇，这里应该播放蒙太奇，并用蒙太奇时长或 Notify 来结束
    float SwapDuration = 0.5f; // 假设切枪需要 0.5秒
    GetWorld()->GetTimerManager().SetTimer(
        TimerHandle_SwapWeapon, 
        this, 
        &UCombatComponent::FinishSwapWeapon, 
        SwapDuration, 
        false
    );
}

bool UCombatComponent::ServerSwapWeapon_Validate(int32 SlotIndex)
{
    return true;
}

void UCombatComponent::FinishSwapWeapon()
{
	AHunterCharacterBase* Character = Cast<AHunterCharacterBase>(GetOwner());
	if (Character)
	{
		// 切枪结束，恢复 Idle
		Character->CurrentCharacterState = EHunterCharacterState::Idle;
	}
}

// 服务器 RPC：丢弃当前武器
void UCombatComponent::ServerDropWeapon_Implementation()
{
	AHunterCharacterBase* Character = Cast<AHunterCharacterBase>(GetOwner());
	if (!Character) return;

	if (Character->CurrentCharacterState != EHunterCharacterState::Idle || !EquippedWeaponBase) return;

	Character->CurrentCharacterState = EHunterCharacterState::WeaponDropping;
	StopFire(); 

	// 确定丢弃的是哪个槽位的枪
	int32 DroppedSlotIndex = -1;
	if (EquippedWeaponBase == GunNO1)
	{
		GunNO1 = nullptr;
		DroppedSlotIndex = 0;
	}
	else if (EquippedWeaponBase == GunNO2)
	{
		GunNO2 = nullptr;
		DroppedSlotIndex = 1;
	}

	// 准备丢弃的枪的引用
	AGunBase* DroppedGun = Cast<AGunBase>(EquippedWeaponBase);

	// ➤➤➤ 【关键修改：把 UI 更新提前到这里！】
	// 在 SetOwner(nullptr) 之前调用，确保 Gun 还能找到它的 Owner (Character -> Controller -> UI)
    
	// A. 弹药 UI 清零 (利用 Gun 自身的 RPC)
	if (DroppedGun)
	{
		DroppedGun->ClientUpdateAmmoUI(0, 0, 0); 
	}

	// B. 武器槽位 Icon 透明度变 0 (利用 Component 的 RPC)
	if (DroppedSlotIndex != -1)
	{
		ClientUpdateWeaponIconUI(DroppedSlotIndex, nullptr, 0.0f);
	}

	// ➤➤➤ 【现在才开始断绝关系和物理处理】
    
	// 执行物理丢弃
	EquippedWeaponBase->SetWeaponState(EWeaponState::EWS_Dropped);
	EquippedWeaponBase->OnRep_WeaponState(); // 强制物理开启
    
	// 这句话执行后，DroppedGun->GetOwner() 就会变空，所以上面的 UI 代码必须在这之前
	EquippedWeaponBase->SetOwner(nullptr); 
    
	// 施加冲力
	if (UPrimitiveComponent* RootComp = Cast<UPrimitiveComponent>(EquippedWeaponBase->GetRootComponent()))
	{
		if (RootComp->IsSimulatingPhysics())
		{
			FVector ThrowDir = Character->GetActorForwardVector() + FVector(0,0,0.5f);
			RootComp->AddImpulse(ThrowDir * 500.0f, NAME_None, true);
		}
	}

	// 清理当前装备引用
	EquippedWeaponBase = nullptr; 

	// 角色状态更新
	Character->SetGunTypeAndIsWithGun(false, EGunType::EGT_Rifle); 
	ClientUpdateGunState(false, EGunType::EGT_Rifle);

	// 恢复角色状态
	Character->CurrentCharacterState = EHunterCharacterState::Idle;
}

bool UCombatComponent::ServerDropWeapon_Validate() { return true; }

// 客户端 RPC：更新动画蓝图持枪状态
void UCombatComponent::ClientUpdateGunState_Implementation(bool bIsWithGun, EGunType GunType)
{
	if (AHunterCharacterBase* Character = Cast<AHunterCharacterBase>(GetOwner()))
	{
		// 这会在客户端本地执行，更新蓝图里的动画状态
		Character->SetGunTypeAndIsWithGun(bIsWithGun, GunType);
	}
}
#pragma endregion

#pragma region ItemRelated
// 服务器rpc拾取道具
void UCombatComponent::ServerPickupItem_Implementation(AItemBase* ItemToPick)
{
	if (!ItemToPick) return;

	// 根据道具类型分发逻辑
	switch (ItemToPick->ItemType)
	{
	case EItemType::EIT_Ammo:
		if (AAmmoItem* AmmoItem = Cast<AAmmoItem>(ItemToPick))
		{
			HandleAmmoPickup(AmmoItem);
		}
		break;
        
	case EItemType::EIT_Effect:
		// TODO: 效果道具逻辑
		break;
        
	case EItemType::EIT_Score:
		if (AScoreItem* SItem = Cast<AScoreItem>(ItemToPick))
		{
			HandleScorePickup(SItem);
		}
		break;
	}

	// 拾取完成后销毁道具
	ItemToPick->Destroy();
}

bool UCombatComponent::ServerPickupItem_Validate(AItemBase* ItemToPick)
{
	return true;
}

void UCombatComponent::HandleAmmoPickup(AAmmoItem* AmmoItem)
{
	if (!AmmoItem) return;

	// 1. 增加背包弹药数据
	int32 CurrentCount = GetBagBulletCount(AmmoItem->BulletType);
	int32 NewCount = CurrentCount + AmmoItem->AmmoCount;
	SetBagBulletCount(AmmoItem->BulletType, NewCount);

	// 2. 检查当前手持武器是否匹配
	// 如果手里正好拿着这个子弹类型的枪，需要立即刷新 UI
	if (AGunBase* EquippedGun = Cast<AGunBase>(EquippedWeaponBase))
	{
		if (EquippedGun->BulletType == AmmoItem->BulletType)
		{
			// 获取最新数据并刷新 UI
			// 注意：这里用的是增加后的 NewCount
			EquippedGun->ClientUpdateAmmoUI(
				EquippedGun->ClipCurrentAmmo, 
				EquippedGun->ClipMaxAmmo, 
				NewCount
			);
		}
	}
}

// 实现 HandleScorePickup拾取积分道具
void UCombatComponent::HandleScorePickup(AScoreItem* ScoreItem)
{
	if (!ScoreItem) return;

	AHunterCharacterBase* Character = Cast<AHunterCharacterBase>(GetOwner());
	if (Character)
	{
		// 服务器直接加分
		// 因为我们用了 ReplicatedUsing，加分后 Character 会自动通知客户端 Controller 更新 UI
		Character->AddCarryingPoints(ScoreItem->ScoreValue);
        
		// (可选) 播放捡到积分的音效
		// MultiPlaySound(...)
	}
}
#pragma endregion 

#pragma region Fire
//开始攻击
void UCombatComponent::StartFire()
{
	if (EquippedWeaponBase == nullptr) return;

	// ➤➤➤ 【新增】拦截逻辑：如果正在换弹或切枪，禁止开火
	AHunterCharacterBase* Character = Cast<AHunterCharacterBase>(GetOwner());
	if (Character)
	{
		// 【新增】封锁 WeaponDropping
		if (Character->CurrentCharacterState == EHunterCharacterState::GunReloading || 
			Character->CurrentCharacterState == EHunterCharacterState::WeaponSwitching ||
			Character->CurrentCharacterState == EHunterCharacterState::PickingUp ||
			Character->CurrentCharacterState == EHunterCharacterState::WeaponDropping) // 禁止丢枪时开火
		{
			return;
		}
	}
	
	// 1. 必响的第一枪
	Fire();

	if (AGunBase* GunWeapon = Cast<AGunBase>(EquippedWeaponBase))
	{
		// 🔴 打印出当前这把枪到底是不是自动的
		FString DebugMsg = FString::Printf(TEXT("【调试】枪名: %s, 是否自动: %s, 射速: %f"),
		                                   *GunWeapon->GetName(),
		                                   GunWeapon->IsAutomatic ? TEXT("是 (TRUE)") : TEXT("否 (FALSE)"),
		                                   GunWeapon->AutomaticRate);

		// 显示在屏幕上 (红色)
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, DebugMsg);

		// 如果是自动，开启定时器
		if (GunWeapon->IsAutomatic)
		{
			float FireRate = (GunWeapon->AutomaticRate > 0.f) ? GunWeapon->AutomaticRate : 0.1f;

			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("【调试】定时器已启动！"));

			GetWorld()->GetTimerManager().SetTimer(
				TimerHandle_AutoFire,
				this,
				&UCombatComponent::AutoFireLoop,
				FireRate,
				true
			);
		}
		else
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, TEXT("【调试】被判定为单发武器，忽略连发逻辑"));
		}
	}

	//
	// if (EquippedWeaponBase == nullptr) return;
	//
	// // A. 无论什么武器，按下瞬间先执行一次攻击
	// // 这样保证了：刀会挥一次，雷会扔出去，半自动步枪会射第一发
	// Fire();
	//
	// // B. 检查是否需要连发 (只有自动步枪需要)
	// if (AGunBase* GunWeapon = Cast<AGunBase>(EquippedWeaponBase))
	// {
	// 	// 只有当武器是全自动 (IsAutomatic) 时，才开启定时器
	// 	if (GunWeapon->IsAutomatic)
	// 	{
	// 		float FireRate = (GunWeapon->AutomaticRate > 0.f) ? GunWeapon->AutomaticRate : 0.1f;
	//            
	// 		// 开启循环定时器
	// 		GetWorld()->GetTimerManager().SetTimer(
	// 			TimerHandle_AutoFire, 
	// 			this, 
	// 			&UCombatComponent::AutoFireLoop, 
	// 			FireRate, 
	// 			true // 循环
	// 		);
	// 	}
	// }
	// // C. 如果是刀或手雷，这里什么都不做，因为它们不需要连发逻辑
}

// 停止攻击 (松开鼠标)
void UCombatComponent::StopFire()
{
	// 只有连发定时器需要被清除
	// 对于刀和雷，松开鼠标没有任何逻辑
	GetWorld()->GetTimerManager().ClearTimer(TimerHandle_AutoFire);

	// ✅ 【新增】重置后座力状态
	if (AGunBase* GunWeapon = Cast<AGunBase>(EquippedWeaponBase))
	{
		GunWeapon->ResetRecoil();
	}
}

// 自动循环开火
void UCombatComponent::AutoFireLoop()
{
	// 再次调用 Fire，形成连发
	Fire();
}

void UCombatComponent::Fire()
{
	//getowner并且转化为AHunterCharacterBase
	AHunterCharacterBase* Character = Cast<AHunterCharacterBase>(GetOwner());
	if (Character == nullptr) return;

	// ➤➤➤ 【新增】状态拦截
	if (Character->CurrentCharacterState == EHunterCharacterState::GunReloading || 
		Character->CurrentCharacterState == EHunterCharacterState::WeaponSwitching)
	{
		return;
	}
	
	// 1. 检查是否有武器装备以及角色状态是不是idle
	if (EquippedWeaponBase == nullptr) return;

	// 检查弹药 (仅针对枪)
	if (AGunBase* Gun = Cast<AGunBase>(EquippedWeaponBase))
	{
		if (Gun->ClipCurrentAmmo <= 0)
		{
			StopFire(); // 没子弹了强制松手
			return;
		}
	}

	//2，发起服务端rpc，服务器逻辑
	ServerFire();

	// 3. 调用武器的 Fire 函数,客户端表现
	EquippedWeaponBase->Fire();
}

// 服务器rpc开火逻辑
void UCombatComponent::ServerFire_Implementation()
{
	if (EquippedWeaponBase == nullptr) return;

	//按照武器类型执行不同的开火逻辑（修改成开火在里面执行）
	switch (EquippedWeaponBase->GetWeaponType())
	{
	case EWeaponType::EWT_Gun:
		GunFireProcess();
		break;
	default:
		break;
	}
}

// 枪开火逻辑，由服务器调用
void UCombatComponent::GunFireProcess()
{
	//getowner并且转化为AHunterCharacterBase
	AHunterCharacterBase* Character = Cast<AHunterCharacterBase>(GetOwner());
	if (Character == nullptr) return;

	//设置状态为正在开火
	//todo 在射击结束的时候改回idle
	Character->CurrentCharacterState = EHunterCharacterState::GunFiring;

	//WeaponBase转化为GunBase
	AGunBase* GunWeapon = Cast<AGunBase>(EquippedWeaponBase);
	if (GunWeapon != nullptr)
	{
		// 1. 检查是否有子弹
		if (GunWeapon->ClipCurrentAmmo <= 0)
		{
			return; // 没有子弹，不能开火
		}

		//2 减少枪体子弹并使用客户端rpc更新ui
		GunWeapon->ClipCurrentAmmo--;
		GunWeapon->ClientUpdateAmmoUI(GunWeapon->ClipCurrentAmmo, GunWeapon->ClipMaxAmmo,
		                              GetBagBulletCount(GunWeapon->BulletType));

		//todo 3,多播rpc身体动画，3p枪口火焰，3p枪口声音
		GunWeapon->MultiPlayShootAnimationAndEffect3P();

		//todo 3.5,客户端rpc武器射击动画,及手臂动画,1p枪口火焰，1p枪口声音播放，触发准星动画
		GunWeapon->ClientPlayShootAnimationAndEffect1P();

		// 服务器告诉客户端：你的枪该抖动了
		GunWeapon->ClientRecoil();

		// 4. 射线检测
		if (APlayerController* PC = Cast<APlayerController>(Character->GetController()))
		{
			FVector StartLocation;
			FRotator ViewRotation;
			bool isMoving = Character->IsMoving();

			// 获取玩家视角的最佳位置和旋转
			// 这在服务器和客户端都能工作，服务器会使用 ControlRotation
			PC->GetPlayerViewPoint(StartLocation, ViewRotation);

			// 使用 ViewRotation.Vector() 作为射击方向
			GunLineTrace(Character, GunWeapon, StartLocation, ViewRotation, isMoving);
		}

		//todo 自动射击，后续需要修改
		FTimerHandle TimerHandle_ResetState;
		// 如果枪有射速设置就用射速，否则默认 0.15秒
		float FireDelay = (GunWeapon->AutomaticRate > 0.f) ? GunWeapon->AutomaticRate : 0.15f;

		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(TimerHandle_ResetState, [Character]()
			{
				if (Character)
				{
					// 时间到，解锁状态，允许再次开火
					Character->CurrentCharacterState = EHunterCharacterState::Idle;
				}
			}, FireDelay, false);
		}
	}
}

bool UCombatComponent::ServerFire_Validate()
{
	return true;
}

//枪射线检测，由服务端调用
void UCombatComponent::GunLineTrace(AHunterCharacterBase* Character, AGunBase* GunWeapon, FVector CameraLocation,
                                    FRotator CameraRotation, bool IsMoving)
{
	FVector EndLocation;
	FVector CameraForwardVector = UKismetMathLibrary::GetForwardVector(CameraRotation);
	TArray<AActor*> IgnoreArray;
	IgnoreArray.Add(Character);
	FHitResult HitResult;
	if (GunWeapon)
	{
		//是否移动会导致不同的检测计算
		if (IsMoving)
		{
			//X,Y,Z加上随机的偏移量
			FVector Vector = CameraLocation + CameraForwardVector * GunWeapon->BulletDistance;
			float RandomX = UKismetMathLibrary::RandomFloatInRange(-GunWeapon->MovingFireRandomRange,
			                                                       GunWeapon->MovingFireRandomRange);
			float RandomY = UKismetMathLibrary::RandomFloatInRange(-GunWeapon->MovingFireRandomRange,
			                                                       GunWeapon->MovingFireRandomRange);
			float RandomZ = UKismetMathLibrary::RandomFloatInRange(-GunWeapon->MovingFireRandomRange,
			                                                       GunWeapon->MovingFireRandomRange);
			EndLocation = CameraLocation + FVector(Vector.X + RandomX, Vector.Y + RandomY, Vector.Z + RandomZ);
		}
		else
		{
			EndLocation = CameraLocation + CameraForwardVector * GunWeapon->BulletDistance;
		}
	}
	bool HitSuccess = UKismetSystemLibrary::LineTraceSingle(GetWorld(), CameraLocation, EndLocation,
	                                                        ETraceTypeQuery::TraceTypeQuery1, false, IgnoreArray,
	                                                        EDrawDebugTrace::None, HitResult, true, FLinearColor::Red,
	                                                        FLinearColor::Green, 3.f);
	if (HitSuccess)
	{
		//UKismetSystemLibrary::PrintString(GetWorld(),FString::Printf(TEXT("Hitactorname %s"),*HitResult.GetActor()->GetName()));

		// 判断是否击中玩家
		AHunterCharacterBase* HitHunterCharacter = Cast<AHunterCharacterBase>(HitResult.GetActor());

		// 2. ➤➤➤ 【新增】尝试识别为怪物 (只要是 AMonsterBase 的子类都能识别)
		AMonsterBase* HitMonster = Cast<AMonsterBase>(HitResult.GetActor());
		if (HitHunterCharacter)
		{
			//造成伤害函数
			TakeDamage(HitResult.PhysMaterial.Get(), HitResult.GetActor(), CameraLocation, HitResult); //打到谁，从哪打，结果信息
		}
		else if (HitMonster)
		{
			// 打中怪物 -> 同样调用 TakeDamage 造成伤害
			// TakeDamage 内部会调用 UGameplayStatics::ApplyPointDamage，
			// 进而触发 MonsterBase::TakeDamage -> 扣血 -> Die
			TakeDamage(HitResult.PhysMaterial.Get(), HitResult.GetActor(), CameraLocation, HitResult);
		}
		else if (false) //todo 判断打到的是不是ai对手
		{
		}
		else
		{
			FRotator XRotator = UKismetMathLibrary::MakeRotFromX(HitResult.Normal); //保存法线的前向向量
			//打到别的生成弹孔
			MultiSpawnBulletDecal(HitResult.Location, XRotator);
		}
	}
}

void UCombatComponent::ClientUpdateWeaponIconUI_Implementation(int32 SlotIndex, AGunBase* Weapon, float Opacity)
{
	if (AHunterCharacterBase* Character = Cast<AHunterCharacterBase>(GetOwner()))
	{
		if (Character->IsLocallyControlled())
		{
			if (AHunterPlayerController* PC = Cast<AHunterPlayerController>(Character->GetController()))
			{
				UTexture2D* Icon = (Weapon != nullptr) ? Weapon->WeaponIcon : nullptr;
				//持有武器透明度1，未持有已捡起武器透明度0.5，丢弃的武器活没有武器透明度0
				PC->UpdateWeaponSlotUI(SlotIndex, Icon, Opacity);
			}
		}
	}
}

//多播rpc，射击到物体生成弹孔贴花
void UCombatComponent::MultiSpawnBulletDecal_Implementation(FVector Location, FRotator Rotation)
{
	//讲当前武器转化为枪武器
	AGunBase* CurrentGuneaponActor = Cast<AGunBase>(EquippedWeaponBase);
	if (CurrentGuneaponActor)
	{
		UDecalComponent* Decal = UGameplayStatics::SpawnDecalAtLocation(
			GetWorld(), CurrentGuneaponActor->BulletDecalMaterial, FVector(8, 8, 8), Location, Rotation, 10);
		if (Decal)
		{
			Decal->SetFadeScreenSize(0.001); //设置距离屏幕暗淡距离
		}
	}
}

#pragma endregion

#pragma region Ammo
int32 UCombatComponent::GetBagBulletCount(EBulletType BulletType) const
{
	// 0号表示步枪子弹，1号表示手枪子弹，2号表示狙击枪子弹
	int32 Index = -1;
	switch (BulletType)
	{
	case EBulletType::EBT_Rifle:
		Index = 0;
		break;
	case EBulletType::EBT_Pistol:
		Index = 1;
		break;
	case EBulletType::EBT_Sniper:
		Index = 2;
		break;
	default:
		return 0;
	}

	if (BagBulletCounts.IsValidIndex(Index))
	{
		return BagBulletCounts[Index].Count;
	}
	return 0;
}

void UCombatComponent::SetBagBulletCount(EBulletType BulletType, int32 NewCount)
{
	// 0号表示步枪子弹，1号表示手枪子弹，2号表示狙击枪子弹
	int32 Index = -1;
	switch (BulletType)
	{
	case EBulletType::EBT_Rifle:
		Index = 0;
		break;
	case EBulletType::EBT_Pistol:
		Index = 1;
		break;
	case EBulletType::EBT_Sniper:
		Index = 2;
		break;
	default:
		return;
	}

	if (BagBulletCounts.IsValidIndex(Index))
	{
		BagBulletCounts[Index].Count = NewCount;
		// 如果需要，这里可以添加 OnRep 通知或者其他逻辑
	}
}

// 换弹本地预判
void UCombatComponent::Reload()
{
    // 如果手里没枪，或者已经满弹，或者已经在换弹，直接本地拦截，节省带宽
    AGunBase* Gun = Cast<AGunBase>(EquippedWeaponBase);
    if (!Gun) return;
    
    // 如果子弹是满的，不需要换
    if (Gun->ClipCurrentAmmo >= Gun->ClipMaxAmmo) return;
	
	//检测角色状态
	AHunterCharacterBase* Character = Cast<AHunterCharacterBase>(GetOwner());
	if (Character && Character->CurrentCharacterState != EHunterCharacterState::Idle &&
		Character->CurrentCharacterState != EHunterCharacterState::GunFiring)
	{
		return; // 丢枪、切枪、捡枪时不能换弹
	}

	// ➤➤➤ 【新增】本地立即停止开火！
	// 这会清除 TimerHandle_AutoFire，防止下一发子弹打断即将播放的换弹动画
	StopFire();
	
    // 发送请求给服务器
    ServerReload();
}

// 换弹服务器核心逻辑
void UCombatComponent::ServerReload_Implementation()
{
    AHunterCharacterBase* Character = Cast<AHunterCharacterBase>(GetOwner());
    if (!Character) return;
    
    AGunBase* Gun = Cast<AGunBase>(EquippedWeaponBase);
    if (!Gun) return;

    // --- 再次校验条件 (服务器权威检查) ---
    // A. 状态检查 (防止鬼畜)
	if (Character->CurrentCharacterState == EHunterCharacterState::GunReloading ||
		Character->CurrentCharacterState == EHunterCharacterState::WeaponDropping || // 【新增】
		Character->CurrentCharacterState == EHunterCharacterState::PickingUp) 
	{
		return;
	}

    // B. 子弹检查
    if (Gun->ClipCurrentAmmo >= Gun->ClipMaxAmmo) return;

    // C. 背包子弹检查
    int32 BagAmmo = GetBagBulletCount(Gun->BulletType);
    if (BagAmmo <= 0) return; // 没备弹了

    // --- 开始换弹 ---
    
    // 1. 修改状态 -> 正在换弹
    Character->CurrentCharacterState = EHunterCharacterState::GunReloading;
    StopFire(); // 停止开火

    // 2. 播放动画 (表现层)
    Gun->ClientPlayReloadAnim1P(); // 客户端 RPC：1P 手臂动画
    Gun->MultiPlayReloadAnim3P();  // 多播 RPC：3P 身体动画

    // 3. 计算换弹时间
    // 我们用 3P 蒙太奇的长度作为换弹时间 (或者你在 GunBase 里定义一个 float ReloadTime)
    float ReloadDuration = 1.5f; // 默认值
    if (Gun->ClientArmsReloadAnimMontage1P)
    {
        ReloadDuration = Gun->ClientArmsReloadAnimMontage1P->GetPlayLength();
    }
    
    // 4. 开启定时器 -> 真正加子弹
    GetWorld()->GetTimerManager().SetTimer(
        TimerHandle_Reload,
        this,
        &UCombatComponent::FinishReload,
        ReloadDuration,
        false
    );
}

bool UCombatComponent::ServerReload_Validate() { return true; }

// 换弹结束 (加子弹逻辑)
void UCombatComponent::FinishReload()
{
    AHunterCharacterBase* Character = Cast<AHunterCharacterBase>(GetOwner());
    if (!Character) return;
    
    // 确保换弹过程中没切枪 (仍然拿着这把枪)
    AGunBase* Gun = Cast<AGunBase>(EquippedWeaponBase);
    if (!Gun) 
    {
        Character->CurrentCharacterState = EHunterCharacterState::Idle;
        return;
    }

    // --- 弹药计算逻辑 ---
    int32 BagAmmo = GetBagBulletCount(Gun->BulletType);
    int32 AmmoNeeded = Gun->ClipMaxAmmo - Gun->ClipCurrentAmmo; // 缺多少发

    if (BagAmmo >= AmmoNeeded)
    {
        // 背包够用：直接补满
        Gun->ClipCurrentAmmo = Gun->ClipMaxAmmo;
        SetBagBulletCount(Gun->BulletType, BagAmmo - AmmoNeeded);
    }
    else
    {
        // 背包不够：全部填进去
        Gun->ClipCurrentAmmo += BagAmmo;
        SetBagBulletCount(Gun->BulletType, 0);
    }

    // --- 更新 UI (通知客户端) ---
    // 注意：这里需要重新获取剩余背包子弹
    Gun->ClientUpdateAmmoUI(Gun->ClipCurrentAmmo, Gun->ClipMaxAmmo, GetBagBulletCount(Gun->BulletType));

    // --- 恢复状态 ---
    Character->CurrentCharacterState = EHunterCharacterState::Idle;
}
#pragma endregion

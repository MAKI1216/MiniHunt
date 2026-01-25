// Fill out your copyright notice in the Description page of Project Settings.


#include "MiniHunt/Character/CombatComponent.h"

#include "HunterCharacterBase.h"
#include "Components/DecalComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
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

	// ...
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
	// 绑定到 Owner 的委托
	if (AActor* Owner = GetOwner())
	{
		Owner->OnTakePointDamage.AddDynamic(this, &UCombatComponent::OnHit);
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
}

#pragma endregion

#pragma region BasicComabt

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
void UCombatComponent::OnHit(AActor* DamagedActor, float Damage, class AController* InstigatedBy,
	FVector HitLocation, class UPrimitiveComponent* FHitComponent, FName BoneName,
	FVector ShotFromDirection, const class UDamageType* DamageType, AActor* DamageCauser)
{
	CurrentHealth-=Damage;
	//发起客户端rpc，修改ui
	ClientUpdateHealthUI(CurrentHealth,MaxHealth);
	if(CurrentHealth<=0)
	{
		//TODO:死亡逻辑
		//DeathMatchDeath(DamageCauser);
	}
}

//修改血量ui的客户端rpc
void UCombatComponent::ClientUpdateHealthUI_Implementation(float NewCurrentHealth,float NewMaxHealth)
{
	
}

#pragma endregion

#pragma region PickAndEquip

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
	

	// === 【核心逻辑修正】 ===
   
	// 场景 A: 如果当前手里没有枪 -> 马上装备这把新枪
	if (EquippedWeaponBase == nullptr) // 必须是 == nullptr
		{
		WeaponToPick->SetWeaponState(EWeaponState::EWS_Equipped);
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
		WeaponToPick->SetWeaponState(EWeaponState::EWS_PossesButNotMoving);
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

#pragma region Fire
void UCombatComponent::Fire()
{
	//getowner并且转化为AHunterCharacterBase
	AHunterCharacterBase* Character = Cast<AHunterCharacterBase>(GetOwner());
	if (Character == nullptr) return;
	
	// 1. 检查是否有武器装备以及角色状态是不是idle
	if (EquippedWeaponBase == nullptr || Character->CurrentCharacterState != EHunterCharacterState::Idle)
	{
		return; // 没有武器，不能开火
	}
	
	//2，发起服务端rpc，服务器逻辑
	ServerFire();
	
	// 3. 调用武器的 Fire 函数,客户端表现
	EquippedWeaponBase->Fire();
}

// 服务器rpc开火逻辑
void UCombatComponent::ServerFire_Implementation()
{
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
	if (GunWeapon == nullptr)
	{
		// 1. 检查是否有子弹
		if (GunWeapon->ClipCurrentAmmo <= 0)
		{
			return; // 没有子弹，不能开火
		}
		
		//todo 2,多播身体动画和射击动画
		
		// 3. 射线检
		if (APlayerController* PC = Cast<APlayerController>(Character->GetController()))
		{
			FVector StartLocation;
			FRotator ViewRotation;
			bool isMoving = Character->IsMoving();

			// 获取玩家视角的最佳位置和旋转
			// 这在服务器和客户端都能工作，服务器会使用 ControlRotation
			PC->GetPlayerViewPoint(StartLocation, ViewRotation);

			// 使用 ViewRotation.Vector() 作为射击方向
			GunLineTrace(Character,GunWeapon, StartLocation, ViewRotation, isMoving); 
		}
	}
}
bool UCombatComponent::ServerFire_Validate()
{
	return true;
}

//枪射线检测，由服务端调用
void UCombatComponent::GunLineTrace(AHunterCharacterBase* Character, AGunBase* GunWeapon, FVector CameraLocation, FRotator CameraRotation, bool IsMoving)
{
	FVector EndLocation;
	FVector CameraForwardVector = UKismetMathLibrary::GetForwardVector(CameraRotation);
	TArray<AActor*> IgnoreArray;
	IgnoreArray.Add(Character);
	FHitResult HitResult;
	if (GunWeapon)
	{
		//是否移动会导致不同的检测计算
		if(IsMoving)
		{
			//X,Y,Z加上随机的偏移量
			FVector Vector=CameraLocation+CameraForwardVector*GunWeapon->BulletDistance;
			float RandomX=UKismetMathLibrary::RandomFloatInRange(-GunWeapon->MovingFireRandomRange,GunWeapon->MovingFireRandomRange);
			float RandomY=UKismetMathLibrary::RandomFloatInRange(-GunWeapon->MovingFireRandomRange,GunWeapon->MovingFireRandomRange);
			float RandomZ=UKismetMathLibrary::RandomFloatInRange(-GunWeapon->MovingFireRandomRange,GunWeapon->MovingFireRandomRange);
			EndLocation=CameraLocation+FVector(Vector.X+RandomX,Vector.Y+RandomY,Vector.Z+RandomZ);
		}
		else
		{
			EndLocation=CameraLocation+CameraForwardVector*GunWeapon->BulletDistance;
		}
	}
	bool HitSuccess=UKismetSystemLibrary::LineTraceSingle(GetWorld(),CameraLocation,EndLocation,ETraceTypeQuery::TraceTypeQuery1,false,IgnoreArray,EDrawDebugTrace::None,HitResult,true,FLinearColor::Red,FLinearColor::Green,3.f);
	if(HitSuccess)
	{
		//UKismetSystemLibrary::PrintString(GetWorld(),FString::Printf(TEXT("Hitactorname %s"),*HitResult.GetActor()->GetName()));
			
		// 判断是否击中玩家
		AHunterCharacterBase* HitHunterCharacter=Cast<AHunterCharacterBase>(HitResult.GetActor());
		
		if(HitHunterCharacter)
		{
			//造成伤害函数
			TakeDamage(HitResult.PhysMaterial.Get(),HitResult.GetActor(),CameraLocation,HitResult);//打到谁，从哪打，结果信息
		}
		else if (false)//todo 判断打到的是不是怪物
		{
			
		}
		else if (false)//todo 判断打到的是不是ai对手
		{
			
		}
		else
		{
			FRotator XRotator = UKismetMathLibrary::MakeRotFromX(HitResult.Normal);//保存法线的前向向量
			//打到别的生成弹孔
			MultiSpawnBulletDecal(HitResult.Location,XRotator);
		}
		
	}
}

//多播rpc，射击到物体生成弹孔贴花
void UCombatComponent::MultiSpawnBulletDecal_Implementation(FVector Location, FRotator Rotation)
{
	//讲当前武器转化为枪武器
	AGunBase* CurrentGuneaponActor=Cast<AGunBase>(EquippedWeaponBase);
	if (CurrentGuneaponActor)
	{
		UDecalComponent* Decal=UGameplayStatics::SpawnDecalAtLocation(GetWorld(),CurrentGuneaponActor->BulletDecalMaterial,FVector(8,8,8),Location,Rotation,10);
		if(Decal)
		{
			Decal->SetFadeScreenSize(0.001);//设置距离屏幕暗淡距离
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
#pragma endregion
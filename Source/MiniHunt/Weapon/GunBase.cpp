// Fill out your copyright notice in the Description page of Project Settings.


#include "MiniHunt/Weapon/GunBase.h"

#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "MiniHunt/Character/HunterCharacterBase.h"
#include "MiniHunt/Character/CombatComponent.h" // 引入 CombatComponent
#include "MiniHunt/Controller/HunterPlayerController.h" // 引入 Controller
#include "Net/UnrealNetwork.h"

#pragma region Engine
AGunBase::AGunBase()
{
	PrimaryActorTick.bCanEverTick = true;
	
	WeaponType = EWeaponType::EWT_Gun;
	//3P武器碰撞体重叠事件
	SphereCollision3P->OnComponentBeginOverlap.AddDynamic(this,&AGunBase::OnOtherBeginOverlap);
	// 3P武器碰撞体重叠结束事件
	SphereCollision3P->OnComponentEndOverlap.AddDynamic(this, &AGunBase::OnOtherEndOverlap);
}

// Called every frame
void AGunBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// Called when the game starts or when spawned
void AGunBase::BeginPlay()
{
	Super::BeginPlay();
	
}

// 网络复制属性
void AGunBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(AGunBase, ClipCurrentAmmo, COND_None);
}
#pragma endregion

void AGunBase::Dropped()
{
	Super::Dropped();
}

#pragma region PickAndEquip
void AGunBase::Equip(USceneComponent* InParent1P, USceneComponent* InParent3P)
{
	if (InParent1P && InParent3P)
	{
		// 1. 设置状态，父类虚函数写好了
		SetActorEnableCollision(false); // 装备后关闭武器碰撞，防止挡路
		PickupWidget->SetVisibility(false); // 确保 UI 关闭
	
		//2. 挂载，子类虚函数自己去实现
		switch (GunType)
		{
		case EGunType::EGT_Rifle:
			// 挂载 1P 模型 -> 手臂的 WeaponSocket
			WeaponMesh1P->AttachToComponent(InParent1P, FAttachmentTransformRules::SnapToTargetNotIncludingScale, FName("WeaponSocket"));
        
			// 挂载 3P 模型 -> 身体的 WeaponSocket
			WeaponMesh3P->AttachToComponent(InParent3P, FAttachmentTransformRules::SnapToTargetNotIncludingScale, FName("WeaponSocket"));
			break;
		case EGunType::EGT_Pistol:
			//todo 手枪挂载
			break;
		case EGunType::EGT_Sniper:
			//todo 狙击枪挂载
			break;
		}
	
		//3. 调用character函数修改角色动画蓝图参数，子类虚函数自己去实现
		AHunterCharacterBase* Character = Cast<AHunterCharacterBase>(GetOwner());
		if (Character)
		{
			Character->SetGunTypeAndIsWithGun(true, GunType);
			
			// 获取 CombatComponent 以读取背包子弹数量
			if (UCombatComponent* CombatComp = Character->FindComponentByClass<UCombatComponent>())
			{
				int32 BagAmmo = CombatComp->GetBagBulletCount(BulletType);
				// 4. 修改子弹ui，客户端rpc
				ClientUpdateAmmoUI(ClipCurrentAmmo, BagAmmo);
			}
		}
	}
	
}
// 角色进入武器检测范围
void AGunBase::OnOtherBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	AHunterCharacterBase* Hunter = Cast<AHunterCharacterBase>(OtherActor);

	// 【修改点】增加 IsLocallyControlled() 判断
	// 只有当这个 Hunter 是当前客户端的主控角色时，才显示 UI
	if (Hunter && Hunter->IsLocallyControlled())
	{
		// 1. 显示 UI
		ShowPickupWidget(true);
		// 2. 告诉角色：你现在可以捡这把枪
		Hunter->SetOverlappingWeapon(this);
	}
}
// 角色离开武器检测范围
void AGunBase::OnOtherEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	AHunterCharacterBase* Hunter = Cast<AHunterCharacterBase>(OtherActor);

	// 【修改点】同样增加 IsLocallyControlled() 判断
	// 防止别人走开时，把你的 UI 给关掉了
	if (Hunter && Hunter->IsLocallyControlled())
	{
		// 1. 隐藏 UI
		ShowPickupWidget(false);
		// 2. 告诉角色：你离开了，不能捡了
		Hunter->SetOverlappingWeapon(nullptr);
	}
}
#pragma endregion

#pragma region Fire
void AGunBase::Fire()
{
	Super::Fire();
	
}
#pragma endregion

void AGunBase::MultiShottingEffect_Implementation()
{
			
}

bool AGunBase::MultiShottingEffect_Validate()
{
	return true;
}

#pragma region GunUI
void AGunBase::ClientUpdateAmmoUI_Implementation(int32 CurrentAmmo, int32 MaxAmmo)
{
	// 获取 Owner (Character)
	if (AHunterCharacterBase* Character = Cast<AHunterCharacterBase>(GetOwner()))
	{
		// 获取 Controller
		if (AHunterPlayerController* PC = Cast<AHunterPlayerController>(Character->GetController()))
		{
			// 调用 UI 更新函数
			PC->UpdateAmmoUI(CurrentAmmo, MaxAmmo);
		}
	}
}
#pragma endregion

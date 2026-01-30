// Fill out your copyright notice in the Description page of Project Settings.


#include "MiniHunt/Weapon/GunBase.h"

#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "Kismet/GameplayStatics.h"
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
	SphereCollision3P->OnComponentBeginOverlap.AddDynamic(this, &AGunBase::OnOtherBeginOverlap);
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

#pragma region  Ammo
// 1P 换弹：只在控制该角色的客户端执行
void AGunBase::ClientPlayReloadAnim1P_Implementation()
{
	AHunterCharacterBase* Character = Cast<AHunterCharacterBase>(GetOwner());
	if (!Character) return;
	
	// 1. 播放手臂蒙太奇
	if (UAnimInstance* AnimBP = Character->GetClientArmsAnimBP())
	{
		if (ClientArmsReloadAnimMontage1P)
		{
			AnimBP->Montage_Play(ClientArmsReloadAnimMontage1P);
		}
	}
	
	// 2. 播放枪械本身的动画 (比如弹夹掉落)
	if (WeaponMesh1P && ClientWeaponReloadAnimSequence1P)
	{
		WeaponMesh1P->PlayAnimation(ClientWeaponReloadAnimSequence1P, false);
	}
    
	// 3. (可选) 播放换弹声音 
	// UGameplayStatics::PlaySound2D(...)
	
}

// 3P 换弹：所有人都能看到
void AGunBase::MultiPlayReloadAnim3P_Implementation()
{
	AHunterCharacterBase* Character = Cast<AHunterCharacterBase>(GetOwner());
	if (!Character) return;

	// 1. 播放身体蒙太奇
	if (UAnimInstance* BodyAnimBP = Character->GetServerBodysAnimBP())
	{
		if (ServerTpBoysReloadAnimMontage3P)
		{
			BodyAnimBP->Montage_Play(ServerTpBoysReloadAnimMontage3P);
		}
	}
    
	// 2. (可选) 播放3P空间音效
}
#pragma endregion
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
			WeaponMesh1P->AttachToComponent(InParent1P, FAttachmentTransformRules::SnapToTargetNotIncludingScale,
			                                FName("WeaponSocket"));

			// 挂载 3P 模型 -> 身体的 WeaponSocket
			WeaponMesh3P->AttachToComponent(InParent3P, FAttachmentTransformRules::SnapToTargetNotIncludingScale,
			                                FName("WeaponSocket"));
			break;
		case EGunType::EGT_Pistol:
			//todo 手枪挂载
			// 挂载 1P 模型 -> 手臂的 WeaponSocket
			WeaponMesh1P->AttachToComponent(InParent1P, FAttachmentTransformRules::SnapToTargetNotIncludingScale,
											FName("Pistol_Socket"));

			// 挂载 3P 模型 -> 身体的 WeaponSocket
			WeaponMesh3P->AttachToComponent(InParent3P, FAttachmentTransformRules::SnapToTargetNotIncludingScale,
											FName("WeaponSocket"));
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
				ClientUpdateAmmoUI(ClipCurrentAmmo, ClipMaxAmmo, BagAmmo);
			}
		}
	}
}

// 角色进入武器检测范围
void AGunBase::OnOtherBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                   UPrimitiveComponent* OtherComp, int OtherBodyIndex, bool bFromSweep,
                                   const FHitResult& SweepResult)
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
void AGunBase::OnOtherEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                 UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
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


//1p客户端rpc武器射击动画,及手臂动画,1p枪口火焰，1p枪口声音播放，触发准星动画
void AGunBase::ClientPlayShootAnimationAndEffect1P_Implementation()
{
	// 获取角色并转化为 AHunterCharacterBase
	AHunterCharacterBase* Character = Cast<AHunterCharacterBase>(GetOwner());
	// 获取角色的 Controller 并转化为 AHunterPlayerController
	AHunterPlayerController* HunterController = nullptr; // 初始化为 nullptr
	
	// 检查 Character 是否有效
	if (Character)
	{
		// 通过 Character 获取 Controller
		HunterController = Cast<AHunterPlayerController>(Character->GetController());
	}
		
	//播放1p武器手臂射击动画
	if (Character && ClientArmsFireAnimMontage1P)
	{
		//获取角色1p动画蓝图
		UAnimInstance* ClientWeaponsAnimBP = Character->GetClientArmsAnimBP();
		if (ClientWeaponsAnimBP)
		{
			// 播放1p武器手臂射击动画
			ClientWeaponsAnimBP->Montage_Play(ClientArmsFireAnimMontage1P);
		}
	}
	
	//播放1p武器射击动画序列
	if (WeaponMesh1P && ClientWeaponFireAnimSequence1P)
	{
		//播放1p武器射击动画序列
		WeaponMesh1P->PlayAnimation(ClientWeaponFireAnimSequence1P, false);
	}
	
	//播放1p枪口声音
	UGameplayStatics::SpawnEmitterAttached(MuzzleFlash1P, WeaponMesh1P, TEXT("Fire_FX_Slot"), FVector::Zero(),
		FRotator::ZeroRotator,FVector::OneVector,
		EAttachLocation::KeepRelativeOffset,true,
		EPSCPoolMethod::None,true
		);
	//客户端射击声音用2d因为和距离无关
	UGameplayStatics::PlaySound2D(GetWorld(), FireSound1P);
	
	//运用镜头抖动和准星扩散动画
	if (HunterController && CameraShakeClass1P)
	{
		// 使用新的 API ClientStartCameraShake
		HunterController->ClientStartCameraShake(CameraShakeClass1P);
		//准星扩散动画
		HunterController->DoCrosshairRecoil();
	}
}


//3p多播rpc，实现多播rpc身体动画，3p枪口火焰，3p枪口声音
void AGunBase::MultiPlayShootAnimationAndEffect3P_Implementation()
{
	//获得角色并转化为AHunterCharacterBase
	AHunterCharacterBase* Character = Cast<AHunterCharacterBase>(GetOwner());

	//播放3p身体射击动画
	if (Character && ServerTpBoysShootAnimMontage3P)
	{
		//获取角色3p动画蓝图
		UAnimInstance* ServerBodysAnimBP = Character->GetServerBodysAnimBP();
		if (ServerBodysAnimBP)
		{
			// 播放3p武器射击动画
			ServerBodysAnimBP->Montage_Play(ServerTpBoysShootAnimMontage3P);
		}
	}

	//播放枪口火焰和声音
	if (GetOwner() != UGameplayStatics::GetPlayerPawn(GetWorld(), 0))
	{
		//如果调用多播的客户端等于当前客户端就不执行闪光效果和播放声音
		FName MuzzleFlashSocketName = TEXT("Fire_FX_Slot");
		if (GunType == EGunType::EGT_Rifle)
		{
			MuzzleFlashSocketName = TEXT("MuzzleSocket");
		}
		if (MuzzleFlash3P && WeaponMesh3P)
		{
			UGameplayStatics::SpawnEmitterAttached(MuzzleFlash3P, WeaponMesh3P, MuzzleFlashSocketName, FVector::Zero(),
			FRotator::ZeroRotator, FVector::OneVector,
			EAttachLocation::KeepRelativeOffset, true,
			EPSCPoolMethod::None, true
			);
		}
		//播放枪口声音
		if (FireSound3P)
		{
			UGameplayStatics::PlaySoundAtLocation(GetWorld(), FireSound3P, GetActorLocation());
		}
	}
}

bool AGunBase::MultiPlayShootAnimationAndEffect3P_Validate()
{
	return true;
}

// 1. 实现后座力计算逻辑
void AGunBase::ClientRecoil_Implementation()
{
	// 获取持有者的 Controller
	AHunterCharacterBase* Character = Cast<AHunterCharacterBase>(GetOwner());
	if (!Character) return;
    
	// 如果不是本地控制的角色，不需要计算后座力（别人不需要看你的屏幕抖动）
	if (!Character->IsLocallyControlled()) return;

	APlayerController* PC = Cast<APlayerController>(Character->GetController());
	if (!PC) return;

	// 1. 累加时间/次数 (每次调用代表射了一枪)
	// 如果是连发，这里可以加 AutomaticRate；如果是单点，可以加固定值 0.1
	// 这里我们简单处理，每次射击 X轴 + 0.1
	RecoilTimer += (IsAutomatic && AutomaticRate > 0) ? AutomaticRate : 0.1f;

	float NewVerticalRecoilAmount = 0.0f;
	float NewHorizontalRecoilAmount = 0.0f;

	// 2. 从曲线获取当前的累计后座力值 (Y轴)
	if (VerticalRecoilCurve)
	{
		NewVerticalRecoilAmount = VerticalRecoilCurve->GetFloatValue(RecoilTimer);
	}
	if (HorizontalRecoilCurve)
	{
		NewHorizontalRecoilAmount = HorizontalRecoilCurve->GetFloatValue(RecoilTimer);
	}

	// 3. 计算增量 (这一帧需要移动多少) = 当前总值 - 上一帧总值
	// 比如：第1发是2度，第2发是5度。那么第2发这一瞬间应该抬高 (5-2)=3度。
	float DeltaVertical = NewVerticalRecoilAmount - OldVerticalRecoilAmount;
	float DeltaHorizontal = NewHorizontalRecoilAmount - OldHorizontalRecoilAmount;

	// 4. 应用到控制器
	// 注意：在 UE 中，Pitch 往上通常是负值(或正值取决于设置)，AddInput 会处理灵敏度
	// 通常后座力是枪口上抬，所以 Pitch 增加。
	if (DeltaVertical != 0.f || DeltaHorizontal != 0.f)
	{
		// AddControllerPitchInput: 负值通常是向下看，正值向上看? 
		// 实际上取决于你的 Input设置。通常 Pitch -= Value 是抬头。
		// 我们假设曲线配置的是正数代表上抬力度。
		Character->AddControllerPitchInput(-DeltaVertical); // 负值让镜头上抬
		Character->AddControllerYawInput(DeltaHorizontal);
	}

	// 5. 更新旧值，为下一枪做准备
	OldVerticalRecoilAmount = NewVerticalRecoilAmount;
	OldHorizontalRecoilAmount = NewHorizontalRecoilAmount;
}

// 2. 实现重置逻辑
void AGunBase::ResetRecoil()
{
	RecoilTimer = 0.0f;
	OldVerticalRecoilAmount = 0.0f;
	OldHorizontalRecoilAmount = 0.0f;
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
void AGunBase::ClientUpdateAmmoUI_Implementation(int32 CurrentAmmo, int32 MaxAmmo, int32 BagAmmo)
{
	// 获取 Owner (Character)
	if (AHunterCharacterBase* Character = Cast<AHunterCharacterBase>(GetOwner()))
	{
		// 获取 Controller
		if (AHunterPlayerController* PC = Cast<AHunterPlayerController>(Character->GetController()))
		{
			// 调用 UI 更新函数
			PC->UpdateAmmoUI(CurrentAmmo, MaxAmmo, BagAmmo);
		}
	}
}

// 客户端 RPC：更新武器槽位 UI 图标
void AGunBase::ClientUpdateWeaponIconUI_Implementation(int32 SlotIndex, AGunBase* Weapon, bool bIsActive)
{
	AHunterCharacterBase* Character = Cast<AHunterCharacterBase>(GetOwner());
	if (!Character || !Character->IsLocallyControlled()) return;

	if (AHunterPlayerController* PC = Cast<AHunterPlayerController>(Character->GetController()))
	{
		// 2. 取出枪的图标 (如果 Weapon 是空，传 nullptr 进去，表示清空槽位)
		UTexture2D* IconTexture = (Weapon != nullptr) ? Weapon->WeaponIcon : nullptr;

		// 3. 调用 PC 的蓝图函数去更新 UMG
		PC->UpdateWeaponSlotUI(SlotIndex, IconTexture, bIsActive);
	}
}
#pragma endregion

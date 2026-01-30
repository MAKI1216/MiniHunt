// Fill out your copyright notice in the Description page of Project Settings.


#include "MiniHunt/Weapon/WeaponBase.h"

#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "MiniHunt/Character/HunterCharacterBase.h"
#include "Net/UnrealNetwork.h"
#include "PhysicsEngine/PhysicsAsset.h"

// WeaponBase.cpp

AWeaponBase::AWeaponBase()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true; // 开启网络复制

	// ➤➤➤ 【新增】确保物理移动会同步给客户端
	SetReplicateMovement(true);

	// =========================================================
	// 1. 【核心修改】先创建 3P Mesh，并把它设为 RootComponent
	// =========================================================
	WeaponMesh3P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh3P"));

	// 设置为根组件！这样物理掉落时，Actor 坐标会跟随模型移动
	SetRootComponent(WeaponMesh3P);

	// 3P Mesh 设置
	WeaponMesh3P->SetOwnerNoSee(true); // 自己看不见（看的是1P）
	WeaponMesh3P->SetEnableGravity(true);
	WeaponMesh3P->SetSimulatePhysics(false); // 默认拿在手里，不模拟物理

	// 碰撞设置：作为根组件，它负责物理碰撞
	WeaponMesh3P->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	WeaponMesh3P->SetCollisionObjectType(ECC_PhysicsBody);
	WeaponMesh3P->SetCollisionResponseToAllChannels(ECR_Block); // 阻挡地面
	WeaponMesh3P->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore); // 忽略角色（防绊倒）
	WeaponMesh3P->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore); // 忽略摄像机

	// =========================================================
	// 2. 创建 1P Mesh，挂载到 3P Mesh (也就是 Root) 上
	// =========================================================
	WeaponMesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh1P"));
	WeaponMesh1P->SetupAttachment(RootComponent); // 挂在根上
	WeaponMesh1P->SetOnlyOwnerSee(true); // 只有自己看得见
	WeaponMesh1P->SetCastShadow(false); // 1P 手臂通常不需要投射阴影（优化）
	WeaponMesh1P->SetCollisionEnabled(ECollisionEnabled::NoCollision); // 1P 模型纯视觉，无碰撞

	// =========================================================
	// 3. 创建检测球，挂载到 3P Mesh 上
	// =========================================================
	SphereCollision3P = CreateDefaultSubobject<USphereComponent>(TEXT("SphereCollision3P"));
	SphereCollision3P->SetupAttachment(RootComponent); // 挂在根上，这样枪滚远了，检测圈也跟着走
	SphereCollision3P->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SphereCollision3P->SetCollisionObjectType(ECC_WorldDynamic);
	SphereCollision3P->SetCollisionResponseToAllChannels(ECR_Ignore);
	SphereCollision3P->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap); // 只检测角色

	// =========================================================
	// 4. 创建 UI Widget
	// =========================================================
	PickupWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("PickupWidget"));
	PickupWidget->SetupAttachment(RootComponent);
	PickupWidget->SetVisibility(false);
	PickupWidget->SetWidgetSpace(EWidgetSpace::Screen);
	// 稍微抬高一点位置，别陷在枪里
	PickupWidget->SetRelativeLocation(FVector(0.f, 0.f, 50.f));
}

#pragma region Engine
// Called when the game starts or when spawned
void AWeaponBase::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void AWeaponBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// 获取网络复制的属性
void AWeaponBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	// 注册 WeaponState，当服务器改变它时，客户端会收到通知
	DOREPLIFETIME(AWeaponBase, WeaponState);
}
#pragma endregion

void AWeaponBase::Fire()
{
}

void AWeaponBase::Dropped()
{
}


#pragma region Weapon State PickAndEquip
void AWeaponBase::Equip(USceneComponent* InParent1P, USceneComponent* InParent3P)
{
	if (InParent1P && InParent3P)
	{
		// 1. 设置状态
		SetActorEnableCollision(false); // 装备后关闭武器碰撞，防止挡路
		PickupWidget->SetVisibility(false); // 确保 UI 关闭

		//2. 挂载，子类虚函数自己去实现


		//3. 调用character函数修改角色动画蓝图参数，子类虚函数自己去实现
	}
}

// 2. 服务器设置状态
void AWeaponBase::SetWeaponState(EWeaponState State)
{
	WeaponState = State;
	// 服务器自己也需要手动调用一次 OnRep，因为 OnRep 默认只在客户端触发
	OnRep_WeaponState();
}

void AWeaponBase::OnRep_WeaponState()
{
	// 定义一个临时字符串变量用于格式化日志
	FString LogMsg;
	
	switch (WeaponState)
	{
	// ----------------------------------------------------------------
	// 状态 A: 装备中 (拿在手上)
	// ----------------------------------------------------------------
	case EWeaponState::EWS_Equipped:
		// 1. 【必须】客户端必须自己关物理
		SphereCollision3P->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		WeaponMesh3P->SetSimulatePhysics(false);
		WeaponMesh3P->SetEnableGravity(false);
		WeaponMesh3P->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		// ➤➤➤ 【核心修复代码 Start】 ➤➤➤

		// 1. 解除 Actor 级别的隐藏 
		// (修复：捡起第二把枪时 CombatComponent 调用的 SetActorHiddenInGame(true))
		SetActorHiddenInGame(false);

		// 2. 解除 3P Mesh 组件级别的隐藏
		// (修复：背枪状态下调用的 WeaponMesh3P->SetHiddenInGame(true))
		WeaponMesh3P->SetHiddenInGame(false);

		// 3. (保险起见) 解除 1P Mesh 隐藏
		WeaponMesh1P->SetHiddenInGame(false);

		// ➤➤➤ 【核心修复代码 End】 ➤➤➤

		// 2. 恢复可见性
		WeaponMesh1P->SetVisibility(true);
		WeaponMesh3P->SetVisibility(true);

		// 3. 挂载逻辑 (保持不变)
		if (AHunterCharacterBase* Character = Cast<AHunterCharacterBase>(GetOwner()))
		{
			Equip(Character->GetFPArmMesh(), Character->GetMesh());
		}
		break;

	// ----------------------------------------------------------------
	// 状态 B: 在背包里 (持有但不装备)
	// ----------------------------------------------------------------
	case EWeaponState::EWS_PossesButNotMoving:
		// 1. 【必须】关物理
		SphereCollision3P->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		WeaponMesh3P->SetSimulatePhysics(false);
		WeaponMesh3P->SetEnableGravity(false);
		WeaponMesh3P->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		// 2. 【核心】隐藏 1P 模型 (枪在背上，所以眼睛里不能看到枪)
		WeaponMesh1P->SetVisibility(false);

		// 3. 处理 3P 模型 (挂在背上)
		// 虽然根组件的 Attach 会自动同步，但为了保证新加入玩家(JIP)也能看到枪在背上，
		// 这里建议写上，或者用来处理"从手里放回背上"的动画过渡逻辑
		if (AHunterCharacterBase* Character = Cast<AHunterCharacterBase>(GetOwner()))
		{
			WeaponMesh3P->SetVisibility(true);
			WeaponMesh3P->AttachToComponent(
				Character->GetMesh(),
				FAttachmentTransformRules::SnapToTargetNotIncludingScale,
				FName("BackpackSocket")
			);
			// 如果不想做背枪，直接隐藏它
			WeaponMesh3P->SetHiddenInGame(true);
		}
		break;

		// ----------------------------------------------------------------
		// 状态 C: 掉在地上
		// ----------------------------------------------------------------
	case EWeaponState::EWS_Dropped:
		// 1. 基础解禁
		SetActorHiddenInGame(false);
		WeaponMesh3P->SetHiddenInGame(false);
		WeaponMesh3P->SetVisibility(true);
		SetActorEnableCollision(true); 
		DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

		// 2. ➤➤➤ 【修改核心】不再使用 ProfileName，手动精细控制每一项！
		// 这样可以确保 100% 忽略角色，不会被 Profile 覆盖
		WeaponMesh3P->SetCollisionObjectType(ECC_PhysicsBody); // 设为物理物体
		WeaponMesh3P->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics); // 开启查询和物理
        
		// A. 先让它阻挡所有东西 (比如地面、墙壁)
		WeaponMesh3P->SetCollisionResponseToAllChannels(ECR_Block);
        
		// B. 然后单独把“Pawn (角色)”和“Camera (相机)”设为忽略
		// 这样角色走过去会直接穿过枪，不会踢飞它
		WeaponMesh3P->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore); 
		WeaponMesh3P->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);

		// 3. 开启物理 (带 CCD)
		WeaponMesh3P->SetSimulatePhysics(true);
		WeaponMesh3P->SetEnableGravity(true);
		WeaponMesh3P->SetUseCCD(true);
		WeaponMesh3P->WakeAllRigidBodies();

		// 4. ➤➤➤ 【新增】代码级阻尼设置 (双重保险)
		// 即使你忘记改物理资产，这里也能强制让枪不乱转
		WeaponMesh3P->SetLinearDamping(2.0f);  // 线性阻尼
		WeaponMesh3P->SetAngularDamping(5.0f); // 角阻尼 (防转圈神器)

		// 5. 辅助组件设置 (确保 Sphere 只是检测，不产生推力)
		SphereCollision3P->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		SphereCollision3P->SetCollisionResponseToAllChannels(ECR_Ignore);
		SphereCollision3P->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap); // 只重叠，不阻挡

		WeaponMesh1P->SetVisibility(false);
		break;
	}
}

void AWeaponBase::ShowPickupWidget(bool bShow)
{
	if (PickupWidget)
	{
		PickupWidget->SetVisibility(bShow);
	}
}
#pragma endregion

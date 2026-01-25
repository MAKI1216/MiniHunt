// Fill out your copyright notice in the Description page of Project Settings.


#include "MiniHunt/Weapon/WeaponBase.h"

#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "MiniHunt/Character/HunterCharacterBase.h"
#include "Net/UnrealNetwork.h"

// WeaponBase.cpp

AWeaponBase::AWeaponBase()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true; // 开启网络复制

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
       
       // 2. 恢复可见性 (防止从背包拿出来时看不见)
       WeaponMesh1P->SetVisibility(true); 
       WeaponMesh3P->SetVisibility(true);

       // 3. 【必须】挂载 1P 模型 (子组件挂载不同步，必须客户端自己做)
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
       // 1. 【必须】开物理
       WeaponMesh3P->SetSimulatePhysics(true);
       WeaponMesh3P->SetEnableGravity(true);
       WeaponMesh3P->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
       SphereCollision3P->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

       // 2. 隐藏 1P，显示 3P
       WeaponMesh1P->SetVisibility(false);
       WeaponMesh3P->SetVisibility(true);
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

// Fill out your copyright notice in the Description page of Project Settings.


#include "MiniHunt/Character/HunterCharacterBase.h"

#include "CombatComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "Components/SkeletalMeshComponent.h" // 解决 ResetAllBodiesBelowSimulatePhysics 爆红
#include "Components/SkinnedMeshComponent.h"  // 解决 EVisibilityBasedAnimTickOption 爆红
#include "MiniHunt/Controller/HunterPlayerController.h"
#include "MiniHunt/Item/ItemBase.h"
#include "Net/UnrealNetwork.h"

#pragma region Engine
// Sets default values
AHunterCharacterBase::AHunterCharacterBase()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	//创建摄像机组件
	PlayerCamera=CreateDefaultSubobject<UCameraComponent>(TEXT("PlayerCamera"));
	if(PlayerCamera)
	{
		PlayerCamera->SetupAttachment(RootComponent);
		PlayerCamera->bUsePawnControlRotation = true;
	}
	//创建第一人称1P手臂模型
	FPArmMesh=CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FPArmMesh"));
	if(FPArmMesh)
	{
		FPArmMesh->SetupAttachment(PlayerCamera);
		//设置自由自己可以看见手臂模型
		FPArmMesh->SetOnlyOwnerSee(true);
	}
	//设置身体3P模型自己看不见,只进行查询碰撞,碰撞类型为Pawn
	GetMesh()->SetOwnerNoSee(true);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	GetMesh()->SetCollisionObjectType(ECC_Pawn);
	// 创建战斗组件
	CombatComponent = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));

	// 初始化角色状态为 Idle
	CurrentCharacterState = EHunterCharacterState::Idle;
	
	// ➤➤➤ 【新增】初始化动画蓝图实例
	if (GetMesh())
	{
		ServerBodysAnimBP = GetMesh()->GetAnimInstance();
	}
	if (FPArmMesh)
	{
		ClientArmsAnimBP = FPArmMesh->GetAnimInstance();
	}
	
	// 初始化大范围探测球 (比如 8米半径)
	DiscoverySphere = CreateDefaultSubobject<USphereComponent>(TEXT("DiscoverySphere"));
	DiscoverySphere->SetupAttachment(RootComponent);
	DiscoverySphere->SetSphereRadius(800.0f);
	DiscoverySphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	// 假设道具 Mesh 的 ObjectType 是 WorldDynamic，这里只检测它
	DiscoverySphere->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
}

// Called when the game starts or when spawned
void AHunterCharacterBase::BeginPlay()
{
	Super::BeginPlay();

	// 记录游戏开始时的位置作为复活点
	BirthLocation = GetActorLocation();
	BirthRotation = GetActorRotation();
	
	// ➤➤➤ 【新增】游戏一开始就强制应用正确的碰撞设置
    InitMeshCollision();
	
	// 获取玩家控制器
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		// 获取增强输入子系统
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			// 激活映射上下文！(优先级设为0)
			// 注意：这里必须确保 DefaultMappingContext 在蓝图中已经赋值，否则为 nullptr
			if (DefaultMappingContext)
			{
				Subsystem->AddMappingContext(DefaultMappingContext, 0);
			}
		}
	}
	// --- 添加以下代码来限制视角 ---
	if (APlayerController* PC = Cast<APlayerController>(Controller))
	{
		if (PC->PlayerCameraManager)
		{
			// 设置抬头限制 (正数，通常设为 80.0f 到 89.0f)
			PC->PlayerCameraManager->ViewPitchMax = 80.0f;

			// 设置低头限制 (负数，通常设为 -80.0f 到 -89.0f)
			PC->PlayerCameraManager->ViewPitchMin = -80.0f;
            
			// 顺便提一下，如果你想限制左右转头（比如做炮塔），可以设置 ViewYawMin/Max
			// 但 FPS 角色通常左右是 360 度无限制，所以不需要设 Yaw
		}
	}
	
	// 绑定大范围检测事件
	if (IsLocallyControlled()) // 只有本地客户端需要描边
	{
		DiscoverySphere->OnComponentBeginOverlap.AddDynamic(this, &AHunterCharacterBase::OnDiscoverySphereBeginOverlap);
		DiscoverySphere->OnComponentEndOverlap.AddDynamic(this, &AHunterCharacterBase::OnDiscoverySphereEndOverlap);
	}
	
	// ➤➤➤ 【极速修复】 强制刷新一次网络休眠状态
	// 告诉引擎：我不休眠，一直同步我！
	SetNetDormancy(DORM_Never);
}

// Called every frame
void AHunterCharacterBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AHunterCharacterBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	//同步当前角色状态
	DOREPLIFETIME_CONDITION(AHunterCharacterBase, CurrentCharacterState, COND_None);
	
	// 同步携带和提交的分数
	DOREPLIFETIME(AHunterCharacterBase, CarryingPoints);
	DOREPLIFETIME(AHunterCharacterBase, SubmittedPoints);
}
#pragma endregion

#pragma region Input
// 绑定输入功能
void AHunterCharacterBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))//检查是否成功转换为增强输入组件
	{
		// 跳跃
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Triggered, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// 移动
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AHunterCharacterBase::Move);

		// 视角
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AHunterCharacterBase::Look);

		// 绑定交互键 (F)
		EnhancedInputComponent->BindAction(FInteractAction, ETriggerEvent::Started, this, &AHunterCharacterBase::InteractButtonPressed);
		
		// 绑定开火键 (鼠标左键)
		// 连发系统使用 Triggered 是正确的：它会在按住时持续触发（每帧调用）。
		// 注意：需要在 FireButtonPressed 或 CombatComponent 中处理射击间隔。
		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Started, this, &AHunterCharacterBase::FireButtonPressed);
		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Completed, this, &AHunterCharacterBase::StopFireButtonPressed);
		
		//绑定切换武器按键
		EnhancedInputComponent->BindAction(EquipSlot1Action, ETriggerEvent::Started, this, &AHunterCharacterBase::EquipSlot1ButtonPressed);
		EnhancedInputComponent->BindAction(EquipSlot2Action, ETriggerEvent::Started, this, &AHunterCharacterBase::EquipSlot2ButtonPressed);
		
		// 绑定 R 键换弹 (Started 触发一次即可)
		EnhancedInputComponent->BindAction(ReloadAction, ETriggerEvent::Started, this, &AHunterCharacterBase::ReloadButtonPressed);
		
		// 绑定丢弃键 (P)
		// Started: 按下瞬间触发; Completed: 松开瞬间触发
		EnhancedInputComponent->BindAction(DropAction, ETriggerEvent::Started, this, &AHunterCharacterBase::DropButtonPressed);
		EnhancedInputComponent->BindAction(DropAction, ETriggerEvent::Completed, this, &AHunterCharacterBase::DropButtonReleased);
		// 为了防止玩家按着按着走开了或者被取消了，Canceled 也绑定一下松开逻辑
		EnhancedInputComponent->BindAction(DropAction, ETriggerEvent::Canceled, this, &AHunterCharacterBase::DropButtonReleased);
	}
}
// 移动
void AHunterCharacterBase::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add movement 
		AddMovementInput(GetActorForwardVector(), MovementVector.Y);
		AddMovementInput(GetActorRightVector(), MovementVector.X);
	}
}

// 视角
void AHunterCharacterBase::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void AHunterCharacterBase::FireButtonPressed()
{
	if (CombatComponent)
	{
		CombatComponent->StartFire(); 
	} 
}

void AHunterCharacterBase::StopFireButtonPressed()
{
	if (CombatComponent)
	{
		CombatComponent->StopFire();
	}
}

void AHunterCharacterBase::EquipSlot1ButtonPressed()
{
	// 如果正在切枪、正在换弹、正在射击，禁止切枪 (根据需求调整)
	if (CurrentCharacterState != EHunterCharacterState::Idle && CurrentCharacterState != EHunterCharacterState::GunFiring) return;
    
	if (CombatComponent)
	{
		CombatComponent->ServerSwapWeapon(0); // 请求切换到 0号位 (GunNO1)
	}
}

void AHunterCharacterBase::EquipSlot2ButtonPressed()
{
	if (CurrentCharacterState != EHunterCharacterState::Idle && CurrentCharacterState != EHunterCharacterState::GunFiring) return;

	if (CombatComponent)
	{
		CombatComponent->ServerSwapWeapon(1); // 请求切换到 1号位 (GunNO2)
	}
}

// 实现回调函数
void AHunterCharacterBase::ReloadButtonPressed()
{
	// 1. 判断角色状态：只有 Idle 或 GunFiring (射击中允许换弹) 可以换弹
	if (CurrentCharacterState != EHunterCharacterState::Idle && 
		CurrentCharacterState != EHunterCharacterState::GunFiring)
	{
		return;
	}

	if (CombatComponent)
	{
		CombatComponent->Reload(); // 调用组件的换弹接口
	}
}

// 丢弃武器按下 P 键：开始倒计时
void AHunterCharacterBase::DropButtonPressed()
{
	// 只有 Idle 状态且 战斗组件存在 且 手里有枪 才能开始长按
	if (CurrentCharacterState != EHunterCharacterState::Idle) return;
	if (!CombatComponent || !CombatComponent->EquippedWeaponBase) return;

	// 3秒后执行丢弃
	GetWorldTimerManager().SetTimer(TimerHandle_DropWeapon, this, &AHunterCharacterBase::ExecuteDropWeapon, 3.0f, false);
    
	//todo (可选) 这里可以做个 UI 进度条显示“正在丢弃...”
	GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Yellow, TEXT("开始长按丢弃..."));
}

// 丢弃松开 P 键：取消倒计时
void AHunterCharacterBase::DropButtonReleased()
{
	GetWorldTimerManager().ClearTimer(TimerHandle_DropWeapon);
	//todo (可选) 隐藏 UI 进度条
}

// 3秒时间到：执行丢弃
void AHunterCharacterBase::ExecuteDropWeapon()
{
	// 再次检查状态 (防止这3秒内你被眩晕了或者死了)
	if (CurrentCharacterState != EHunterCharacterState::Idle) return;

	if (CombatComponent)
	{
		CombatComponent->ServerDropWeapon(); // 调用组件的丢弃逻辑
	}
}
#pragma endregion

#pragma region PickAndEquipWeaponAndItem
void AHunterCharacterBase::InteractButtonPressed()
{
	// ➤➤➤ 【优先级 1】 提交点 (最高优先级)
	if (OverlappingSubmissionPoint)
	{
		// 直接调用 Server RPC
		ServerSubmitPoints();
        
		// (可选) 播放个提交成功的音效或特效
		return; 
	}

	// ➤➤➤ 【优先级 2】 捡道具
	if (OverlappingItem)
	{
		if (CombatComponent)
		{
			CombatComponent->ServerPickupItem(OverlappingItem);
		}
		return; 
	}

	// ➤➤➤ 【优先级 3】 捡枪
	if (OverlappingWeapon)
	{
		if (CombatComponent)
		{
			CombatComponent->PickupWeapon(OverlappingWeapon, FPArmMesh, GetMesh());
		}
	}
}

//检测道具加高光描边
void AHunterCharacterBase::OnDiscoverySphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (AItemBase* Item = Cast<AItemBase>(OtherActor))
	{
		Item->EnableOutline(true); // 开启黄色描边
	}
}

//检测道具加高光描边
void AHunterCharacterBase::OnDiscoverySphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (AItemBase* Item = Cast<AItemBase>(OtherActor))
	{
		Item->EnableOutline(false); // 关闭描边
	}
}

// 设置暂存脚下的道具
void AHunterCharacterBase::SetOverlappingItem(AItemBase* Item)
{
	OverlappingItem = Item;
}

//设置暂存脚下的武器
void AHunterCharacterBase::SetOverlappingWeapon(AWeaponBase* Weapon)
{
	OverlappingWeapon = Weapon;
}
#pragma endregion

#pragma region PointSystem
// 2. 增加积分 (只在服务器运行)
void AHunterCharacterBase::AddCarryingPoints(int32 Amount)
{
	if (HasAuthority())
	{
		CarryingPoints += Amount;
		// 服务器自己也需要刷新一下 UI (OnRep 默认只在客户端触发)
		OnRep_CarryingPoints();
	}
}

// 3. 提交积分 (只在服务器运行 - 预留给提交点功能)
void AHunterCharacterBase::SubmitPoints()
{
	if (HasAuthority())
	{
		SubmittedPoints += CarryingPoints;
		CarryingPoints = 0;
        
		OnRep_SubmittedPoints();
		OnRep_CarryingPoints();
	}
}

// 4. 当携带积分变化时 (客户端自动执行)
void AHunterCharacterBase::OnRep_CarryingPoints()
{
	if (IsLocallyControlled())
	{
		if (AHunterPlayerController* PC = Cast<AHunterPlayerController>(Controller))
		{
			PC->UpdateScoreUI(CarryingPoints, SubmittedPoints);
		}
	}
}

// 5. 当提交积分变化时
void AHunterCharacterBase::OnRep_SubmittedPoints()
{
	if (IsLocallyControlled())
	{
		if (AHunterPlayerController* PC = Cast<AHunterPlayerController>(Controller))
		{
			PC->UpdateScoreUI(CarryingPoints, SubmittedPoints);
		}
	}
}

// 1. 实现 Server RPC
void AHunterCharacterBase::ServerSubmitPoints_Implementation()
{
	// 调用之前做好的 SubmitPoints 函数 (这个函数已经包含了 carrying -> submitted 的逻辑)
	SubmitPoints();
}

bool AHunterCharacterBase::ServerSubmitPoints_Validate() { return true; }
#pragma endregion 

#pragma region GameRulesRelated
void AHunterCharacterBase::HandleDeath(AActor* Killer)
{
	if (!HasAuthority()) return; // 只有服务器能决定生杀大权

	// ➤➤➤ 【新增】 防止重复死亡 (非常重要！)
	// 如果已经死了，直接返回，不再重置定时器
	if (bIsDead) return; 
	bIsDead = true;
	
    // 1. 计算损失积分
    int32 LostPoints = CarryingPoints / 2;
    CarryingPoints -= LostPoints;
    OnRep_CarryingPoints(); // 刷新剩余积分

    // 2. 积分转移 (如果凶手是另一个玩家)
    if (Killer && Killer != this)
    {
        AHunterCharacterBase* KillerPlayer = Cast<AHunterCharacterBase>(Killer);
        // 如果凶手是枪/子弹，尝试找主人
        if (!KillerPlayer) 
        {
             KillerPlayer = Cast<AHunterCharacterBase>(Killer->GetOwner());
        }

        if (KillerPlayer)
        {
            KillerPlayer->AddCarryingPoints(LostPoints);
        }
    }

	// A. 逻辑层：禁止输入 (只需要服务器做)
	if (AController* PC = GetController())
	{
		if (APlayerController* PlayerPC = Cast<APlayerController>(PC))
		{
			PlayerPC->SetIgnoreMoveInput(true);
			PlayerPC->SetIgnoreLookInput(true);
		}
	}
    
	// B. 表现层：变布娃娃 ( ➤➤➤ 改为调用多播!)
	MultiDeathEffects();

	// C. 定时重生
	GetWorldTimerManager().SetTimer(TimerHandle_Respawn, this, &AHunterCharacterBase::Respawn, 5.0f, false);
}

// 【核心】重生逻辑
void AHunterCharacterBase::Respawn()
{
	// ➤➤➤ 【新增】 复活了，重置标记
	bIsDead = false;
	
	// 1. 位置重置 (服务器做就行，Movement组件会自动同步位置)
	TeleportTo(BirthLocation, BirthRotation);

	// 2. ➤➤➤ 【关键修改】 调用多播，让所有客户端(包括自己)都执行物理重置
	MultiRespawn();

	// 3. 满血复活 (逻辑数据)
	if (CombatComponent)
	{
		CombatComponent->ResetHealth(); 
	}

	// 4. 恢复控制 (逻辑数据)
	if (AController* PC = GetController())
	{
		if (APlayerController* PlayerPC = Cast<APlayerController>(PC))
		{
			PlayerPC->SetIgnoreMoveInput(false);
			PlayerPC->SetIgnoreLookInput(false);
		}
	}
    
	// 5. 状态重置
	CurrentCharacterState = EHunterCharacterState::Idle;
}

// 1. 实现多播函数 (所有端都会执行)
void AHunterCharacterBase::MultiDeathEffects_Implementation()
{
	// 开启布娃娃
	GetMesh()->SetCollisionProfileName(TEXT("Ragdoll"));
	GetMesh()->SetSimulatePhysics(true);
    
	// 关闭胶囊体碰撞 (防止挡路)
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

// 1. 实现 MultiRespawn (处理物理、碰撞、模型位置)
void AHunterCharacterBase::MultiRespawn_Implementation()
{
	// 1. 获取 Mesh，防止空指针
	USkeletalMeshComponent* MyMesh = GetMesh();
	if (!MyMesh) return;

	// -------------------------------------------------------
	// 物理重置阶段
	// -------------------------------------------------------
    
	// A. 关闭物理模拟
	MyMesh->SetSimulatePhysics(false);
    
	// B. 【替代方案】让所有刚体强制休眠
	// 这比 ResetAllBodies... 更通用，不需要额外头文件，属于 PrimitiveComponent 的基础功能
	MyMesh->PutAllRigidBodiesToSleep(); 

	// C. 挂载归位 (这一步至关重要)
	MyMesh->AttachToComponent(GetCapsuleComponent(), FAttachmentTransformRules::SnapToTargetIncludingScale);
    
	// D. 手动修正位置 (防止布娃娃导致的偏移)
	MyMesh->SetRelativeLocation(FVector(0, 0, -90)); 
	MyMesh->SetRelativeRotation(FRotator(0, -90, 0));

	// -------------------------------------------------------
	// 碰撞重置阶段
	// -------------------------------------------------------
    
	// 调用之前写好的辅助函数 (确保这一步你保留了)
	InitMeshCollision();

	// -------------------------------------------------------
	// 渲染刷新阶段 (替代之前报错的代码)
	// -------------------------------------------------------

	// A. 强制更新组件的世界坐标 (告诉引擎：我换地方了！)
	MyMesh->UpdateComponentToWorld();

	// B. 强制刷新渲染包围盒
	MyMesh->UpdateBounds();

	// C. 确保可见性开启
	MyMesh->SetVisibility(true);
	MyMesh->SetHiddenInGame(false);

	// D. 混合权重归零
	MyMesh->SetPhysicsBlendWeight(0.0f);

	// E. 停止所有蒙太奇
	if (UAnimInstance* AnimInst = MyMesh->GetAnimInstance())
	{
		AnimInst->StopAllMontages(0.0f);
	}
	
	// ➤➤➤ 【极速修复】 强制刷新网络复制，神奇bug：不移动的化客户端打监听服务器死两次就看不见模型了
	ForceNetUpdate();
	AddActorWorldOffset(FVector(0,0,0.1f));
}

// 1. 实现辅助函数 (把之前的碰撞逻辑搬到这里)
void AHunterCharacterBase::InitMeshCollision()
{
	if (!GetMesh()) return;

	// --- 强制设置 Mesh 碰撞 (双保险) ---
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryOnly); // 只查询
	GetMesh()->SetCollisionObjectType(ECC_Pawn);
    
	GetMesh()->SetCollisionResponseToAllChannels(ECR_Ignore); // 先清空
	GetMesh()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block); // 【核心】让子弹能打中
	GetMesh()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);    // 让相机穿透
	GetMesh()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);      // 忽略胶囊体干扰

	// --- 顺便确保胶囊体也是对的 ---
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
}
#pragma endregion 

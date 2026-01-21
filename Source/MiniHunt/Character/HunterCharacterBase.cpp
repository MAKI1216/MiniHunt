// Fill out your copyright notice in the Description page of Project Settings.


#include "MiniHunt/Character/HunterCharacterBase.h"

#include "CombatComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Camera/CameraComponent.h"

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
}

// Called when the game starts or when spawned
void AHunterCharacterBase::BeginPlay()
{
	Super::BeginPlay();

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
}

// Called every frame
void AHunterCharacterBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

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
#pragma endregion

#pragma region PickAndEquipWeapon
void AHunterCharacterBase::InteractButtonPressed()
{
	// 如果脚下有枪，并且有战斗组件
	if (OverlappingWeapon && CombatComponent)
	{
		// 调用组件的拾取逻辑，传入两个 Mesh 以便挂载
		CombatComponent->PickupWeapon(OverlappingWeapon, FPArmMesh, GetMesh());
	}
}

//设置暂存脚下的武器
void AHunterCharacterBase::SetOverlappingWeapon(AWeaponBase* Weapon)
{
	OverlappingWeapon = Weapon;
}
#pragma endregion

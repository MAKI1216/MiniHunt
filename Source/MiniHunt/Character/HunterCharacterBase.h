// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Kismet/KismetMathLibrary.h"
#include "HunterCharacterBase.generated.h"

class AWeaponBase;
class UCombatComponent;
struct FInputActionValue;
class UCameraComponent;

//枚举类型，表示角色当前状态，有空闲，正在开火，正在换弹，后续添加
UENUM(BlueprintType)
enum class EHunterCharacterState : uint8
{
	Idle UMETA(DisplayName="Idle"),//空闲(或移动)
	GunFiring UMETA(DisplayName="GunFiring"),//正在开火
	GunReloading UMETA(DisplayName="GunReloading"),//正在换弹
	WeaponSwitching UMETA(DisplayName="WeaponSwitching"),//正在更换武器
	UsingSkill UMETA(DisplayName="UsingSkill"),//正在使用技能
	UsingItem UMETA(DisplayName="UsingItem"),//正在使用道具
	PickingUp UMETA(DisplayName="PickingUp"),//正在拾取道具或武器
};

UCLASS()
#pragma region Engine
class MINIHUNT_API AHunterCharacterBase : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AHunterCharacterBase();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	USceneComponent* GetFPArmMesh() const { return FPArmMesh; };
#pragma endregion

#pragma region component
private:
	//相机
	UPROPERTY(Category=Character, VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess="true"))
	UCameraComponent* PlayerCamera;

	//第一人手臂模型
	UPROPERTY(Category=Character, VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess="true"))
	USkeletalMeshComponent* FPArmMesh;

	//手臂动画蓝图
	UPROPERTY(Category=Character, BlueprintReadOnly, meta=(AllowPrivateAccess="true"))
	UAnimInstance* ClientArmsAnimBP;

	//身体动画蓝图
	UPROPERTY(Category=Character, BlueprintReadOnly, meta=(AllowPrivateAccess="true"))
	UAnimInstance* ServerBodysAnimBP;
	
	// 战斗组件
	UPROPERTY(BlueprintReadOnly, Category = Component, meta=(AllowPrivateAccess="true"))
	UCombatComponent* CombatComponent;
public:
	//获取相机组件
	UCameraComponent* GetPlayerCamera() const { return PlayerCamera; };
#pragma endregion

#pragma region CharacterProperties
public:
	// 当前角色状态
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Character, meta=(AllowPrivateAccess="true"),Replicated)
	EHunterCharacterState CurrentCharacterState;
	
	// 判断是否移动的函数
	UFUNCTION()
	bool IsMoving(){return UKismetMathLibrary::VSize(GetVelocity())>0.1f;};

#pragma endregion

#pragma region Weapon
public:
	// 设置当前重叠武器的接口
	void SetOverlappingWeapon(AWeaponBase* Weapon);
	
	// 暂存脚下的武器
	UPROPERTY()
	AWeaponBase* OverlappingWeapon;
#pragma endregion 

#pragma region Input
public:
	
	/** 输入映射上下文，定义输入按键与动作的关联 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	class UInputMappingContext* DefaultMappingContext;

	/** 跳跃动作输入，绑定跳跃按键 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	class UInputAction* JumpAction;

	/** 移动动作输入，绑定方向键或WASD */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	class UInputAction* MoveAction;

	/** 视角旋转动作输入，绑定鼠标移动 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	class UInputAction* LookAction;

	/** 开火键输入，绑定鼠标左键 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta=(AllowPrivateAccess = "true"))
	class UInputAction* FireAction;
	
	/** 交互键输入，绑定 F 键 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	class UInputAction* FInteractAction;

	/** 处理移动输入的方法 */
	void Move(const FInputActionValue& Value);

	/** 处理视角旋转输入的方法 */
	void Look(const FInputActionValue& Value);
	
	// 按下 F 键触发的函数
	void InteractButtonPressed();
	
	// 按下鼠标左键触发的函数
	void FireButtonPressed();
#pragma endregion
};

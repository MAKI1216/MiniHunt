// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Kismet/KismetMathLibrary.h"
#include "MiniHunt/Weapon/GunBase.h"
#include "HunterCharacterBase.generated.h"

class AWeaponBase;
class UCombatComponent;
struct FInputActionValue;
class UCameraComponent;
class AItemBase;
class ASubmissionPoint;

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
	WeaponDropping UMETA(DisplayName="WeaponDropping"), //正在丢弃武器
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
	
	// 组件：大范围探测球,用于察觉道具使其高亮描边
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta=(AllowPrivateAccess="true"))
	USphereComponent* DiscoverySphere;
public:
	//获取相机组件
	UCameraComponent* GetPlayerCamera() const { return PlayerCamera; };
	
	//获取手臂动画蓝图
	UAnimInstance* GetClientArmsAnimBP() const 
	{ 
		return FPArmMesh ? FPArmMesh->GetAnimInstance() : nullptr; 
	};

	//获取身体动画蓝图
	UAnimInstance* GetServerBodysAnimBP() const 
	{ 
		return GetMesh() ? GetMesh()->GetAnimInstance() : nullptr; 
	};
#pragma endregion

#pragma region CharacterProperties
public:
	// 当前角色状态
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Character, meta=(AllowPrivateAccess="true"),Replicated)
	EHunterCharacterState CurrentCharacterState;
	
	// 判断是否移动的函数
	UFUNCTION()
	bool IsMoving() const {return UKismetMathLibrary::VSize(GetVelocity())>0.1f;};

	// 设置是否持枪以及枪的类型的函数（蓝图实现）
	UFUNCTION(BlueprintImplementableEvent, Category = "Character")
	void SetGunTypeAndIsWithGun(bool bIsWithGun, EGunType GunType);
	
	//设置角色当前状态
	UFUNCTION(BlueprintCallable, Category = "Character")
	void SetCharacterState(EHunterCharacterState State){ CurrentCharacterState=State; }
	
	//获取当前角色状态
	UFUNCTION(BlueprintCallable, Category = "Character")
	EHunterCharacterState GetCharacterState() const { return CurrentCharacterState; }

	
#pragma endregion

#pragma region PointSystem
public:
	// --- 积分系统 ---

	// 身上携带的积分 (高风险，死后掉落)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_CarryingPoints, Category = "Score")
	int32 CarryingPoints = 0;

	// 已提交的积分 (安全，计入总分)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_SubmittedPoints, Category = "Score")
	int32 SubmittedPoints = 0;

	// 增加携带积分 (服务器调用)
	UFUNCTION(BlueprintCallable, Category = "Score")
	void AddCarryingPoints(int32 Amount);

	// 提交积分 (服务器调用，后续做提交点时用)
	UFUNCTION(BlueprintCallable, Category = "Score")
	void SubmitPoints();

protected:
	// 变量同步回调：当服务器修改了分数，客户端会自动调用这个函数，我们在这里刷新 UI
	UFUNCTION()
	void OnRep_CarryingPoints();

	UFUNCTION()
	void OnRep_SubmittedPoints();
#pragma endregion 
	
#pragma region Weapon
public:
	// 设置当前重叠武器的接口
	void SetOverlappingWeapon(AWeaponBase* Weapon);
	
	// 暂存脚下的武器
	UPROPERTY()
	AWeaponBase* OverlappingWeapon;

#pragma endregion 
	
#pragma region Item
public:
	// 变量：当前脚下的道具
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction")
	AItemBase* OverlappingItem;

	// 接口：设置当前道具
	void SetOverlappingItem(AItemBase* Item);
	AItemBase* GetOverlappingItem() const { return OverlappingItem; }
	
	// 事件：大范围重叠察觉道具
	UFUNCTION()
	void OnDiscoverySphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	// 事件：大范围重叠结束察觉道具
	UFUNCTION()
	void OnDiscoverySphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
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

	/** 切换武器槽位1输入 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	class UInputAction* EquipSlot1Action;

	/** 切换武器槽位2输入 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	class UInputAction* EquipSlot2Action;
	
	/** 换弹按键输入 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	class UInputAction* ReloadAction;
	
	/** 丢弃武器按键输入 (P键) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	class UInputAction* DropAction;
	
	/** 处理移动输入的方法 */
	void Move(const FInputActionValue& Value);

	/** 处理视角旋转输入的方法 */
	void Look(const FInputActionValue& Value);
	
	// 按下 F 键触发的函数
	void InteractButtonPressed();
	
	// 按下鼠标左键触发的函数
	void FireButtonPressed();
	
	// 松开鼠标左键触发 (停止开火)
	void StopFireButtonPressed();
	
	// 按下切换武器槽位1键触发的函数
	void EquipSlot1ButtonPressed();
	
	// 按下切换武器槽位2键触发的函数
	void EquipSlot2ButtonPressed();
	
	// 按下 R 键触发换弹
	void ReloadButtonPressed();
	
	// 按下 P 键三秒丢弃
	void DropButtonPressed();
	// 松开 P 键
	void DropButtonReleased();
	// 真正的丢弃执行函数 (定时器回调)
	void ExecuteDropWeapon();
	// 丢弃计时器，计时3s
	FTimerHandle TimerHandle_DropWeapon;
#pragma endregion
	
#pragma region GameRulesRelated
public:
	// 当前重叠的提交点
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction")
	ASubmissionPoint* OverlappingSubmissionPoint;

	// 接口
	void SetOverlappingSubmissionPoint(ASubmissionPoint* Point) { OverlappingSubmissionPoint = Point; }
	ASubmissionPoint* GetOverlappingSubmissionPoint() const { return OverlappingSubmissionPoint; }

	// 服务器 RPC：请求提交积分
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerSubmitPoints();
	
	// --- 重生 ---
	FVector BirthLocation; // 出生点
	FRotator BirthRotation;

	// 处理死亡 (服务器调用)
	void HandleDeath(AActor* Killer);

	// 执行重生
	void Respawn();
    
	// 重生定时器
	FTimerHandle TimerHandle_Respawn;
	
	// 【新增】多播 RPC：播放死亡效果 (布娃娃)
	UFUNCTION(NetMulticast, Reliable)
	void MultiDeathEffects();
	
	// 【新增】多播 RPC：负责处理复活时的视觉和物理重置 (所有端都要执行)
	UFUNCTION(NetMulticast, Reliable)
	void MultiRespawn();
	
	// 【新增】辅助函数：初始化网格体碰撞 (防止蓝图设置错误)
	void InitMeshCollision();
	
	// 标记是否死亡
	bool bIsDead = false;
#pragma endregion
};

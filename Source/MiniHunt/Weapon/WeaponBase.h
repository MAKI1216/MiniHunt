// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WeaponBase.generated.h"

class UWidgetComponent;
class USphereComponent;
// 定义武器类型枚举（可选，方便动画状态机切换）
UENUM(BlueprintType)
enum class EWeaponType : uint8
{
	EWT_Gun UMETA(DisplayName = "Gun"),
	EWT_Knife UMETA(DisplayName = "Knife"),
	EWT_Grenade UMETA(DisplayName = "Grenade")
};
// 定义武器状态枚举
UENUM(BlueprintType)
enum class EWeaponState : uint8
{
	EWS_Initial UMETA(DisplayName = "Initial State"),
	EWS_Equipped UMETA(DisplayName = "Equipped"),
	EWS_Dropped UMETA(DisplayName = "Dropped")
};
UCLASS()
#pragma region Engine
class MINIHUNT_API AWeaponBase : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AWeaponBase();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

#pragma endregion

#pragma region Component
protected:
	//武器模型
	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	EWeaponType WeaponType;
public:
	//第三人称武器模型
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category = "Weapon Properties")
	USkeletalMeshComponent* WeaponMesh3P;
	//获取第三人称武器模型
	UFUNCTION(BlueprintCallable, Category = "Weapon Properties")
	USkeletalMeshComponent* GetWeaponMesh3P() const { return WeaponMesh3P; }
	//第一人称武器模型
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category = "Weapon Properties")
	USkeletalMeshComponent* WeaponMesh1P;

	//3P武器碰撞体
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category = "Weapon Properties")
	USphereComponent* SphereCollision3P;

	// 添加一个 UI 组件，用于显示 "按F拾取"
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Properties")
	UWidgetComponent* PickupWidget;

	//重点！！！
	//武器状态 (需要同步),OnRep_WeaponState是同步的回调函数，当武器状态改变时，会调用这个函数
	UPROPERTY(ReplicatedUsing = OnRep_WeaponState, VisibleAnywhere, Category = "Weapon Properties")
	EWeaponState WeaponState;

	//获取武器类型
	UFUNCTION(BlueprintCallable, Category = "Weapon Properties")
	EWeaponType GetWeaponType() const { return WeaponType; }
	
	//同步武器状态的回调函数
	UFUNCTION()
	void OnRep_WeaponState();

	// 【新增】设置武器状态 (服务器调用)
	void SetWeaponState(EWeaponState State);

	// 获取当前状态
	FORCEINLINE EWeaponState GetWeaponState() const { return WeaponState; }
	
	// 装备函数需要知道要把模型挂到谁身上
	// 修改 Equip 函数签名，传入角色的 Mesh
	UFUNCTION(BlueprintCallable)
	virtual void Equip(USceneComponent* InParent1P, USceneComponent* InParent3P);
	
	// --- 核心接口 (虚函数) ---
	// 这是一个多态接口，刀挥舞和枪开火都叫 Fire
	UFUNCTION()
	virtual void Fire();
	
	// 刀不需要换弹，所以这里可以留空，或者甚至不写在基类里（取决于架构）
	// 但通常为了方便，我们在基类留个空实现
	UFUNCTION()
	virtual void Dropped();

	// 控制 UI 显示隐藏的辅助函数
	void ShowPickupWidget(bool bShow);
#pragma endregion
};

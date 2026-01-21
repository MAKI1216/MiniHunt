// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CombatComponent.generated.h"


enum class EBulletType : uint8;
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class MINIHUNT_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()
#pragma region Engine
public:	
	// Sets default values for this component's properties
	UCombatComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
#pragma endregion

#pragma region WeaponMessage
public:
	// 当前装备的武器
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Properties")
	class AWeaponBase* EquippedWeaponBase;

	
#pragma endregion

#pragma region WeaponFunction
public:
	// 拾取武器的核心逻辑
	// Parent1P: 角色手臂 Mesh, Parent3P: 角色身体 Mesh
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void PickupWeapon(AWeaponBase* WeaponToPick, USceneComponent* Parent1P, USceneComponent* Parent3P);
	// 装备武器的核心逻辑
	void EquipWeapon(AWeaponBase* WeaponToEquip, USceneComponent* Parent1P, USceneComponent* Parent3P);
protected:
	// 【新增】服务器 RPC (运行在服务器)
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerPickupWeapon(AWeaponBase* WeaponToPick);

#pragma endregion

#pragma region GunMessage
public:
	//1号位枪
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gun Properties")
	class AWeaponBase*	GunNO1;
	//2号位枪
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gun Properties")
	class AWeaponBase*	GunNO2;
	// 记录背包中每种子弹类型的数量
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gun Properties")
	TMap<EBulletType, int32> BagBulletCountMap;
#pragma endregion 
};

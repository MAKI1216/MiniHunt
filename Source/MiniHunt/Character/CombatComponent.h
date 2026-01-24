// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MiniHunt/Weapon/GunBase.h" // 引入 GunBase 以获取 EBulletType 定义
#include "CombatComponent.generated.h"

// 背包子弹数量结构体用于网络复制 (TMap 不支持复制)
USTRUCT(BlueprintType)
struct FBagBulletInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	EBulletType BulletType = EBulletType::EBT_Rifle; // 默认值

	UPROPERTY(BlueprintReadWrite)
	int32 Count = 0;
};

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

#pragma region HunterComabtPropertise
private:
	//当前生命值，需要同步
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hunter Properties", Replicated, meta = (AllowPrivateAccess = "true"))
	float CurrentHealth;
	
	//最大生命值，需要同步
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hunter Properties", Replicated, meta = (AllowPrivateAccess = "true"))
	float MaxHealth;
#pragma endregion
	
#pragma region HunterCombatFunction
public:
	//给别人造成伤害
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void TakeDamage(UPhysicalMaterial* PhysicalMaterial,AActor* DamagedActor,FVector HitFromDirection,FHitResult& HitInfo);
	
	//ApplyPointDamage的回调函数
	UFUNCTION()
	void OnHit(AActor* DamagedActor, float Damage, class AController* InstigatedBy,
		FVector HitLocation, class UPrimitiveComponent* FHitComponent, FName BoneName,
		FVector ShotFromDirection, const class UDamageType* DamageType, AActor* DamageCauser);
	
	//修改血量ui的客户端rpc
	UFUNCTION(Client, Reliable)
	void ClientUpdateHealthUI(float NewCurrentHealth,float NewMaxHealth);
	
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
	
	// 开火
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void Fire();
	
	// 枪开火
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void GunFireProcess();

protected:
	// 【新增】服务器 RPC (运行在服务器)
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerPickupWeapon(AWeaponBase* WeaponToPick);

	//服务器rpc开火逻辑
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerFire();
#pragma endregion

#pragma region GunMessage
public:
	//	1号位枪
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gun Properties")
	class AWeaponBase*	GunNO1;
	//	2号位枪
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gun Properties")
	class AWeaponBase*	GunNO2;
	
	// 记录背包中每种子弹类型的数量,开启网络复制
	// 注意：TMap 不支持网络复制，改为 TArray<Struct>
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gun Properties", Replicated, meta = (AllowPrivateAccess = "true"))
	TArray<FBagBulletInfo> BagBulletCounts;

	
	
#pragma endregion 
	
#pragma region GunFunction
public:
	// 辅助函数：获取背包中各类子弹数量
	UFUNCTION(BlueprintCallable, Category = "Combat")
	int32 GetBagBulletCount(EBulletType BulletType) const;

	// 辅助函数：设置背包中各类子弹数量
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void SetBagBulletCount(EBulletType BulletType, int32 NewCount);
	
	// 枪射线检测，服务端调用
	UFUNCTION()
	void GunLineTrace(AHunterCharacterBase* Character,AGunBase* GunWeapon,FVector CameraLocation, FRotator CameraRotation, bool IsMoving);

	//多播rpc，射击到物体生成弹孔贴花，不要求可靠，节省带宽
	UFUNCTION(NetMulticast,Unreliable)
	void MultiSpawnBulletDecal(FVector Location,FRotator Rotation);
#pragma endregion
	
};

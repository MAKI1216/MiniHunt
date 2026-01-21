// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MiniHunt/Weapon/WeaponBase.h"
#include "GunBase.generated.h"

class USphereComponent;
// 定义武器类型枚举（可选，方便动画状态机切换）
UENUM(BlueprintType)
enum class EGunType : uint8
{
	EGT_Rifle UMETA(DisplayName = "Rifle"),
	EGT_Pistol UMETA(DisplayName = "Pistol"),
	EGT_Sniper UMETA(DisplayName = "Sniper")
};

// 定义子弹类型枚举（可选，方便动画状态机切换）
UENUM(BlueprintType)
enum class EBulletType : uint8
{
	EBT_Rifle UMETA(DisplayName = "Rifle"),
	EBT_Pistol UMETA(DisplayName = "Pistol"),
	EBT_Sniper UMETA(DisplayName = "Sniper")
};

UCLASS()
class MINIHUNT_API AGunBase : public AWeaponBase
{
	GENERATED_BODY()
public:
	AGunBase();

	// Called every frame
	virtual void Tick(float DeltaTime) override;
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

#pragma region GunProperties
public:
	//枪类型
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gun Properties")
	EGunType GunType;

	//子弹类型
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gun Properties")
	EBulletType BulletType;
	
	//弹夹当前子弹数量
	UPROPERTY(EditAnywhere,Replicated)//服务器改变，服务器页改变
	int32 ClipCurrentAmmo;

	//弹夹最大子弹数量
	UPROPERTY(EditAnywhere)
	int32 ClipMaxAmmo;

	//子弹射击距离
	UPROPERTY(EditAnywhere)
	float BulletDistance;

	//弹孔贴花
	UPROPERTY(EditAnywhere)
	UMaterialInterface* BulletDecalMaterial;

	//基础伤害
	UPROPERTY(EditAnywhere)
	float BaseDamage;

	//是否能自动射击
	UPROPERTY(EditAnywhere)
	bool IsAutomatic;

	//自动射击频率
	UPROPERTY(EditAnywhere)
	float AutomaticRate;

	//跑步射击时候用于计算射线检测的偏移值范围
	UPROPERTY(EditAnywhere)
	float MovingFireRandomRange;
#pragma endregion

#pragma region GunProperties-1P
public:
	//1P开枪手臂动画
	UPROPERTY(EditAnywhere)
	UAnimMontage* ClientArmsFireAnimMontage1P;

	//1P换弹手臂动画
	UPROPERTY(EditAnywhere)
	UAnimMontage* ClientArmsReloadAnimMontage1P;

	//1P开枪声音
	UPROPERTY(EditAnywhere)
	USoundBase* FireSound1P;

	//1P枪口闪光
	UPROPERTY(EditAnywhere)
	UParticleSystem* MuzzleFlash1P;

	//1P镜头摇晃
	UPROPERTY(EditAnywhere)
	TSubclassOf<UCameraShakeBase> CameraShakeClass1P;

	//垂直后座力曲线
	UPROPERTY(EditAnywhere)
	UCurveFloat* VerticalRecoilCurve;

	//水平后座力曲线
	UPROPERTY(EditAnywhere)
	UCurveFloat* HorizontalRecoilCurve;
#pragma endregion

#pragma region  GunFunctions-1P
public:
	//1p射击枪的动画
	UFUNCTION(BlueprintImplementableEvent, Category="FPGunAnimation")
	void PlayShootAnimation(); 

	//1p换弹枪的动画
	UFUNCTION(BlueprintImplementableEvent, Category="FPGunAnimation")
	void PlayReloadAnimation(); 
#pragma endregion


#pragma  region  GunProperties-3P
public:
	//3P枪口闪光
	UPROPERTY(EditAnywhere)
	UParticleSystem* MuzzleFlash3P;
	
	//3P开枪声音
	UPROPERTY(EditAnywhere)
	USoundBase* FireSound3P;

	//第三人称射击动画蒙太奇
	UPROPERTY(EditAnywhere)
	UAnimMontage* ServerTpBoysShootAnimMontage3P;

	//第三人称换弹动画蒙太奇
	UPROPERTY(EditAnywhere)
	UAnimMontage* ServerTpBoysReloadAnimMontage3P;

#pragma endregion

#pragma region GunFunctions-3P
public:
	virtual void Fire() override;
	virtual void Dropped() override;
	virtual void Equip(USceneComponent* InParent1P, USceneComponent* InParent3P) override;

	//3P武器碰撞体重叠事件
	UFUNCTION()
	void OnOtherBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	// 角色离开范围
	UFUNCTION()
	void OnOtherEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	UFUNCTION(NetMulticast,Reliable,WithValidation)
	void MultiShottingEffect();//枪口闪光效果多播
	void MultiShottingEffect_Implementation();
	bool MultiShottingEffect_Validate();
#pragma endregion
};

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
	
	//UI显示的枪械图标
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	UTexture2D* WeaponIcon;
	
	// 【新增】换弹逻辑时长 (秒)
	// 请确保这个时间 >= 你的 1P 换弹蒙太奇长度，否则动画会被切断！
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gun Properties")
	float ReloadTime = 3.3f;
#pragma endregion

#pragma region GunProperties-1P
public:
	//1P开枪手臂动画
	UPROPERTY(EditAnywhere)
	UAnimMontage* ClientArmsFireAnimMontage1P;

	//1P换弹手臂动画
	UPROPERTY(EditAnywhere)
	UAnimMontage* ClientArmsReloadAnimMontage1P;
	
	//1P开枪武器动画序列
	UPROPERTY(EditAnywhere)
	UAnimationAsset* ClientWeaponFireAnimSequence1P;

	//1P换弹武器动画序列
	UPROPERTY(EditAnywhere)
	UAnimationAsset* ClientWeaponReloadAnimSequence1P;

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

	// 记录连发进行了多久 (对应曲线的 X 轴)
	float RecoilTimer = 0.0f;

	// 记录上一次的垂直后座力总值 (用于计算增量)
	float OldVerticalRecoilAmount = 0.0f;

	// 记录上一次的水平后座力总值
	float OldHorizontalRecoilAmount = 0.0f;
#pragma endregion

#pragma region  GunFunctions-1P
public:
	//1p射击枪的动画
	UFUNCTION(BlueprintImplementableEvent, Category="FPGunAnimation")
	void PlayShootAnimation(); 

	//1p换弹枪的动画
	UFUNCTION(BlueprintImplementableEvent, Category="FPGunAnimation")
	void PlayReloadAnimation(); 
	
	//1p客户端rpc武器射击动画,及手臂动画,1p枪口火焰，1p枪口声音播放，触发准星动画
	UFUNCTION(Client, Reliable)
	void ClientPlayShootAnimationAndEffect1P();
	
	// 客户端 RPC：执行后座力计算
	UFUNCTION(Client, Unreliable) // Unreliable 即可，射击频率高，丢包一帧不影响手感
	void ClientRecoil();

	// 重置后座力 (松开鼠标时调用)
	void ResetRecoil();
	
	//客户端rpc，更新武器槽位ui图标
	UFUNCTION(Client, Reliable)
	void ClientUpdateWeaponIconUI(int32 SlotIndex, AGunBase* Weapon, bool bIsActive);
	
	//客户端rpc，播放1p换弹动画
	UFUNCTION(Client, Reliable)
	void ClientPlayReloadAnim1P();
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
	
	//3p多播rpc，实现多播rpc身体动画，3p枪口火焰，3p枪口声音
	UFUNCTION(NetMulticast, Reliable, WithValidation)
	void MultiPlayShootAnimationAndEffect3P();	
	void MultiPlayShootAnimationAndEffect3P_Implementation();
	bool MultiPlayShootAnimationAndEffect3P_Validate();
	
	// 3P 换弹 (多播 RPC)
	UFUNCTION(NetMulticast, Reliable)
	void MultiPlayReloadAnim3P();
#pragma endregion
	
#pragma region GunUI
public:
	//客户端rpc，更新持枪的客户端ui
	UFUNCTION(Client, Reliable)
	void ClientUpdateAmmoUI(int32 CurrentAmmo, int32 MaxAmmo, int32 BagAmmo);
#pragma endregion 
	
};

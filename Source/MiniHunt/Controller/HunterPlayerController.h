// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "HunterPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class MINIHUNT_API AHunterPlayerController : public APlayerController
{
	GENERATED_BODY()
public:
	// 玩家相机震动
	void PlayerCameraShake(TSubclassOf<UCameraShakeBase> CameraShake);

	// 创建玩家UI
	UFUNCTION(BlueprintImplementableEvent,Category="PlayerUI")
	void CreatePlayerUI();

	// 玩家UI执行跨准线震动
	UFUNCTION(BlueprintImplementableEvent,Category="PlayerUI")
	void DoCrosshairRecoil();

	// 更新玩家UI弹药量
	UFUNCTION(BlueprintImplementableEvent,Category="PlayerUI")
	void UpdateAmmoUI(int32 ClipCurrentAmmo,int32 BagCurrentAmmo);

	// 更新玩家UI生命值
	UFUNCTION(BlueprintImplementableEvent,Category="PlayerUI")
	void UpdateHealthUI(float NewHealth,float NewMaxHealth);
};

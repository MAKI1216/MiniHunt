// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MiniHunt/Game/MiniHuntGameState.h"
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
	void UpdateAmmoUI(int32 ClipCurrentAmmo,int32 ClipMaxAmmo,int32 BagCurrentAmmo);

	// 更新玩家UI生命值
	UFUNCTION(BlueprintImplementableEvent,Category="PlayerUI")
	void UpdateHealthUI(float NewHealth,float NewMaxHealth);
	
	// 蓝图实现的事件：更新武器槽位 UI
	// SlotIndex: 0 (1号位), 1 (2号位)
	// Icon: 枪的图片
	// bIsActive: 是否当前正拿着这把枪 (用来高亮)
	UFUNCTION(BlueprintImplementableEvent, Category = "UI")
	void UpdateWeaponSlotUI(int32 SlotIndex, UTexture2D* Icon, float Opacity);
	
	//丢弃更新icon
	UFUNCTION(BlueprintImplementableEvent, Category = "UI")
	void DropUpdateWeaponSlotUI(int32 DropSlotIndex);
	
	//装备更新icon
	UFUNCTION(BlueprintImplementableEvent, Category = "UI")
	void EquipUpdateWeaponSlotUI(int32 EquipSlotIndex);
	
	// 蓝图实现：更新积分 UI
	// Carrying: 携带的, Submitted: 已提交的
	UFUNCTION(BlueprintImplementableEvent, Category = "PlayerUI")
	void UpdateScoreUI(int32 Carrying, int32 Submitted);
	
	// 【新增】客户端 RPC：游戏结束通知
	UFUNCTION(Client, Reliable)
	void ClientNotifyGameOver(const TArray<FPlayerScoreInfo>& FinalScores);

	// 【新增】蓝图实现的事件：显示结算 UI
	UFUNCTION(BlueprintImplementableEvent, Category = "GameRule")
	void ShowGameOverUI(const TArray<FPlayerScoreInfo>& SortedScores);
};

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "MiniHuntGameMode.generated.h"

/**
 * 负责处理出生点、倒计时、游戏结束逻辑
 */
UCLASS()
class MINIHUNT_API AMiniHuntGameMode : public AGameMode
{
	GENERATED_BODY()
	
public:
	AMiniHuntGameMode();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	
	// 重写：处理玩家登录后的出生点选择
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

protected:
	// 游戏总时长 (秒) -> 10分钟 = 600秒
	UPROPERTY(EditDefaultsOnly, Category = "GameRule")
	int32 TotalGameTime = 600;

	// 倒计时定时器
	FTimerHandle TimerHandle_GameCountdown;

	// 每秒执行一次的倒计时函数
	void UpdateTimer();

	// 游戏结束处理
	void GameOver();
};
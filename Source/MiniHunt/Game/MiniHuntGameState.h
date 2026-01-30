// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "MiniHuntGameState.generated.h"

// 定义一个结构体，用来存储排行榜上的单条数据
USTRUCT(BlueprintType)
struct FPlayerScoreInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FString PlayerName;

	UPROPERTY(BlueprintReadOnly)
	int32 Score;

	// 用于高亮判断：是否是当前客户端玩家
	UPROPERTY(BlueprintReadOnly)
	bool bIsSelf; 
};

/**
 * 负责同步全局游戏数据（时间、分数）给所有客户端
 */
UCLASS()
class MINIHUNT_API AMiniHuntGameState : public AGameState
{
	GENERATED_BODY()
	
public:
	// 剩余时间（秒），Replicated 表示服务器变了，客户端自动变
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "GameRule")
	int32 RemainingTime;

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
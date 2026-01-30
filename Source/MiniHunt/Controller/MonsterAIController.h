// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h" // 必须包含这个，否则 FActorPerceptionUpdateInfo 会报错
#include "MonsterAIController.generated.h"

class UBehaviorTreeComponent;
class UBlackboardComponent;
class UAISenseConfig_Sight;

UCLASS()
class MINIHUNT_API AMonsterAIController : public AAIController
{
	GENERATED_BODY()

public:
	AMonsterAIController();

protected:
	virtual void BeginPlay() override;
	
	// 当控制器控制怪物时调用 (相当于 Character 的 PossessedBy)
	virtual void OnPossess(APawn* InPawn) override;

public:
	// --- AI 组件 ---
	// 行为树组件 (执行逻辑)
	// 注意：新版 UE5 AAIController 自带 BrainComponent，但显式声明组件有时候更方便调试
	// 这里我们直接复用基类的 BrainComponent，或者使用 RunBehaviorTree 函数

	// 视觉配置 (眼睛的参数)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	UAISenseConfig_Sight* SightConfig;

	// --- 核心感知回调 ---
	// 当感知系统更新目标时触发 (看到了谁，或者丢失了谁)
	UFUNCTION()
	void OnTargetDetected(AActor* Actor, FAIStimulus Stimulus);
	void SetTargetEnemy(AActor* Actor);

public:
	// --- 配置接口 ---
	// 在编辑器里指定的行为树资产
	UPROPERTY(EditDefaultsOnly, Category = "AI")
	UBehaviorTree* BehaviorTreeAsset;

	// 黑板上的 Key 名称 (防止手写字符串出错)
	static const FName Key_TargetActor;
	static const FName Key_PatrolLocation;
	static const FName Key_IsDead;
	static const FName Key_SpawnLocation;
	static const FName Key_HasRoared;
	static const FName Key_DistanceToTarget;
};
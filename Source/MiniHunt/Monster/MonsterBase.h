// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MonsterBase.generated.h"

UCLASS()
class MINIHUNT_API AMonsterBase : public ACharacter
{
	GENERATED_BODY()
#pragma region engine
public:
	// Sets default values for this character's properties
	AMonsterBase();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
#pragma endregion
	
public:    
	// 1. 血量系统
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float MaxHealth = 100.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
	float CurrentHealth = 100.0f;

	// 2. 攻击配置
	UPROPERTY(EditAnywhere, Category = "Combat")
	UAnimMontage* AttackMontage;

	UPROPERTY(EditAnywhere, Category = "Combat")
	float AttackRange = 150.0f;

	UPROPERTY(EditAnywhere, Category = "Combat")
	float AttackDamage = 10.0f;
	
	// 击杀此怪物获得的积分
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster Stats")
	int32 ScoreValue = 50;
	
	// 3. 核心函数
	// 虚幻引擎标准的受击回调
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;
	
	// 修改移动速度
	UFUNCTION(BlueprintCallable, Category = "Movement")
	void SetMovementSpeed(float NewSpeed);
	
	// 【新增】多播函数：告诉所有客户端播放蒙太奇
	// NetMulticast: 表示服务器调用，所有端执行
	// Reliable: 保证消息一定送到（防止丢包导致没动作）
	UFUNCTION(NetMulticast, Reliable, BlueprintCallable, Category = "Combat")
	void Multi_PlayMontage(UAnimMontage* MontageToPlay);
	
	// 执行攻击（播放动画）
	void Attack();

	// 真正的伤害判定（由动画通知 AnimNotify 调用）
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void AttackHitCheck();

	// 死亡处理
	virtual void Die(AActor* Killer);

	// 行为树需要的接口：获取攻击范围
	float GetAttackRange() const { return AttackRange; }

	// 标记是否死亡
	bool bIsDead = false;
	
	// 【新增】多播函数：处理死亡表现 (布娃娃、碰撞关闭)
	UFUNCTION(NetMulticast, Reliable)
	void MultiDeathEffects();
	
	// 【新增】死亡动画蒙太奇
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	UAnimMontage* DeathMontage;
};

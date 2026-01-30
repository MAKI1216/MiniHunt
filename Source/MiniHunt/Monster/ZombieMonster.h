#pragma once

#include "CoreMinimal.h"
#include "MiniHunt/Monster/MonsterBase.h"
#include "ZombieMonster.generated.h"

UCLASS()
class MINIHUNT_API AZombieMonster : public AMonsterBase
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	virtual void Die(AActor* Killer) override; // 重写死亡，增加给分逻辑

public:
	// 咆哮蒙太奇
	UPROPERTY(EditAnywhere, Category = "Zombie AI")
	UAnimMontage* RoarMontage;

	// 巡逻半径 (以出生点为中心)
	UPROPERTY(EditAnywhere, Category = "Zombie AI")
	float PatrolRadius = 800.0f;

	// 奔跑速度
	UPROPERTY(EditAnywhere, Category = "Zombie AI")
	float RunSpeed = 600.0f;

	// 行走速度 (攻击/闲置时)
	UPROPERTY(EditAnywhere, Category = "Zombie AI")
	float WalkSpeed = 200.0f;
	
	// 记录出生点
	FVector SpawnLocation;
};
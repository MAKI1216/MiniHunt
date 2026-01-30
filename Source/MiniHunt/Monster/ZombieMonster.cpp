#include "MiniHunt/Monster/ZombieMonster.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "MiniHunt/Character/HunterCharacterBase.h"
#include "MiniHunt/Controller/MonsterAIController.h"
#include "MiniHunt/Controller/HunterPlayerController.h" // 假设积分在PC里

void AZombieMonster::BeginPlay()
{
	Super::BeginPlay();

	// 1. 获取 AI 控制器
	if (AMonsterAIController* AIC = Cast<AMonsterAIController>(GetController()))
	{
		// 2. 获取黑板组件
		if (UBlackboardComponent* BB = AIC->GetBlackboardComponent())
		{
			// 3. 【关键】设置出生点为当前位置
			BB->SetValueAsVector(AMonsterAIController::Key_SpawnLocation, GetActorLocation());
            
			// 4. (可选) 确保目标一开始是空的
			BB->SetValueAsObject(AMonsterAIController::Key_TargetActor, nullptr);
			BB->SetValueAsBool(AMonsterAIController::Key_HasRoared, false);
		}
	}
}

void AZombieMonster::Die(AActor* Killer)
{
	// 1. 防止重复死亡
	if (bIsDead) return;
	bIsDead = true;

	// 2. 加分逻辑 (保持不变)
	if (Killer)
	{
		AHunterCharacterBase* PlayerHunter = Cast<AHunterCharacterBase>(Killer);
		if (!PlayerHunter && Killer->GetOwner())
		{
			PlayerHunter = Cast<AHunterCharacterBase>(Killer->GetOwner());
		}

		if (PlayerHunter)
		{
			PlayerHunter->AddCarryingPoints(ScoreValue);
		}
	}

	// 3. 停止 AI (保持不变)
	if (AController* AICon = GetController())
	{
		if (AMonsterAIController* MonsterAIC = Cast<AMonsterAIController>(AICon))
		{
			MonsterAIC->StopMovement();
		}
		AICon->UnPossess(); 
	}

	// 4. 执行多播表现 (播放动画)
	MultiDeathEffects();

	// 5. 设置销毁倒计时
	// 建议设置为：动画长度 + 2秒缓冲，或者直接给个 5.0f
	// 如果你有蒙太奇，可以动态获取长度：
	float LifeTime = 5.0f;
	if (DeathMontage)
	{
		LifeTime = DeathMontage->GetPlayLength()-0.05f;
	}
	SetLifeSpan(LifeTime);
}
// Fill out your copyright notice in the Description page of Project Settings.


#include "MiniHunt/Controller/MonsterAIController.h"

#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "MiniHunt/Character/HunterCharacterBase.h" // 引入玩家类，用于识别目标

// 初始化静态 Key 名称
const FName AMonsterAIController::Key_TargetActor(TEXT("TargetActor"));
const FName AMonsterAIController::Key_PatrolLocation(TEXT("PatrolLocation"));
const FName AMonsterAIController::Key_IsDead(TEXT("IsDead"));
const FName AMonsterAIController::Key_SpawnLocation(TEXT("SpawnLocation"));
const FName AMonsterAIController::Key_HasRoared(TEXT("HasRoared"));
const FName AMonsterAIController::Key_DistanceToTarget(TEXT("DistanceToTarget"));

AMonsterAIController::AMonsterAIController()
{
	// 1. 创建感知组件
	PerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("PerceptionComponent"));

	// 2. 创建并配置视觉
	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	
	if (SightConfig)
	{
		SightConfig->SightRadius = 1000.0f; // 看得见的距离 (10米)
		SightConfig->LoseSightRadius = 1200.0f; // 丢失视野的距离 (稍微大一点，防止在边缘鬼畜切换)
		SightConfig->PeripheralVisionAngleDegrees = 60.0f; // 视野角度 (60度表示前方120度扇形)
		
		// 设置检测规则：检测所有阵营 (敌对、中立、友军)
		// 这一点很重要，否则 AI 可能全都看不见
		SightConfig->DetectionByAffiliation.bDetectEnemies = true;
		SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
		SightConfig->DetectionByAffiliation.bDetectFriendlies = true;

		// 3. 将配置应用到感知组件
		PerceptionComponent->ConfigureSense(*SightConfig);
		PerceptionComponent->SetDominantSense(SightConfig->GetSenseImplementation());
	}
}

void AMonsterAIController::BeginPlay()
{
	Super::BeginPlay();

	// 绑定感知更新委托
	if (PerceptionComponent)
	{
		PerceptionComponent->OnTargetPerceptionUpdated.AddDynamic(this, &AMonsterAIController::OnTargetDetected);
	}
	
	if (BehaviorTreeAsset) 
	{
		RunBehaviorTree(BehaviorTreeAsset);
	}
}

void AMonsterAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// 运行行为树
	if (BehaviorTreeAsset)
	{
		// 这里的 RunBehaviorTree 会自动帮你初始化 Blackboard
		RunBehaviorTree(BehaviorTreeAsset);
	}
}

void AMonsterAIController::OnTargetDetected(AActor* Actor, FAIStimulus Stimulus)
{
	// 1. 确认这是不是我们关心的目标 (比如是不是玩家)
	AHunterCharacterBase* Player = Cast<AHunterCharacterBase>(Actor);
	if (Player == nullptr)
	{
		return; // 看到的不是玩家 (可能是其他怪或者路障)，忽略
	}

	// 2. 获取黑板组件
	UBlackboardComponent* BB = GetBlackboardComponent();
	if (!BB) return;

	// 3. 处理视觉逻辑
	if (Stimulus.WasSuccessfullySensed())
	{
		// === 看到了玩家 ===
		// 把玩家写入黑板的 TargetActor Key
		BB->SetValueAsObject(Key_TargetActor, Player);
		
		// (可选) 可以在这里设置一个状态，比如 "Chasing"
		// GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red, FString::Printf(TEXT("看到了玩家: %s"), *Player->GetName()));
	}
	else
	{
		// === 丢失了玩家视野 (玩家跑远了或躲墙后了) ===
		// 这里有两种处理方式：
		// A. 立刻清除目标 (丧尸变傻，不追了) -> 适合简单 AI
		// B. 保留目标几秒 (在行为树里做 Wait)，或者记录 "LastKnownLocation"
		
		// 这里我们先简单处理：清空目标
		BB->SetValueAsObject(Key_TargetActor, nullptr);
		
		// GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green, TEXT("玩家消失了"));
	}
}

void AMonsterAIController::SetTargetEnemy(AActor* Actor)
{
	if (Actor == nullptr) return;

	UBlackboardComponent* BB = GetBlackboardComponent();
	if (BB)
	{
		AActor* CurrentTarget = Cast<AActor>(BB->GetValueAsObject(Key_TargetActor));
        
		// 如果当前没目标，或者目标变了，更新目标
		if (CurrentTarget != Actor)
		{
			BB->SetValueAsObject(Key_TargetActor, Actor);
			// 注意：不要在这里重置 "HasRoared"，因为如果换了目标可能不需要重新吼叫
			// 但如果脱战后重新发现目标，可能需要重置。这取决于具体设计。
		}
	}
}

#include "MiniHunt/Game/MiniHuntGameMode.h"
#include "MiniHunt/Game/MiniHuntGameState.h"
#include "MiniHunt/Controller/HunterPlayerController.h"
#include "MiniHunt/Character/HunterCharacterBase.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/PlayerState.h"

AMiniHuntGameMode::AMiniHuntGameMode()
{
	// 设置默认的 GameState 类
	GameStateClass = AMiniHuntGameState::StaticClass();
	// 设置默认的 Controller 类 (确保你的蓝图里也设置了)
	PlayerControllerClass = AHunterPlayerController::StaticClass();
	
	// ➤➤➤ 【关键修改】开启无缝漫游，这样 PlayerState 里的数据才能带进游戏！
	bUseSeamlessTravel = true;
}


void AMiniHuntGameMode::BeginPlay()
{
	Super::BeginPlay();

	// 1. 初始化倒计时
	if (AMiniHuntGameState* MyGameState = GetGameState<AMiniHuntGameState>())
	{
		MyGameState->RemainingTime = TotalGameTime;
	}

	// 2. 开启定时器，每秒执行一次 UpdateTimer
	GetWorldTimerManager().SetTimer(TimerHandle_GameCountdown, this, &AMiniHuntGameMode::UpdateTimer, 1.0f, true);
}

void AMiniHuntGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
}

void AMiniHuntGameMode::UpdateTimer()
{
	AMiniHuntGameState* MyGameState = GetGameState<AMiniHuntGameState>();
	if (!MyGameState) return;

	// 减少时间
	MyGameState->RemainingTime--;

	// 检查是否结束
	if (MyGameState->RemainingTime <= 0)
	{
		MyGameState->RemainingTime = 0;
		// 停止定时器
		GetWorldTimerManager().ClearTimer(TimerHandle_GameCountdown);
		// 触发游戏结束
		GameOver();
	}
}

void AMiniHuntGameMode::GameOver()
{
	// 1. 收集所有玩家的分数
	// GameState->PlayerArray 包含了所有连入玩家的信息
	AMiniHuntGameState* MyGameState = GetGameState<AMiniHuntGameState>();
	if (!MyGameState) return;

	// 创建一个临时的结构体数组传给客户端
	TArray<FPlayerScoreInfo> FinalScores;

	for (APlayerState* PS : MyGameState->PlayerArray)
	{
		if (!PS) continue;
		
		FPlayerScoreInfo Info;
		Info.PlayerName = PS->GetPlayerName();
		
		// 这里我们需要获取 HunterCharacter 身上的 SubmittedPoints
		// 注意：PlayerState 默认有 Score 变量，但我们之前把分写在 Character 里了
		// 为了简单，我们遍历 Controller 获取 Character 的分
		// 但更标准的做法是在 Character SubmitPoints 时，把分同步更新到 PlayerState->SetScore()
		
		// 临时方案：遍历所有 Controller
		// (更稳健的方案是把分数存在 PlayerState 里，这里先按你现有的架构写)
		if (AHunterPlayerController* PC = Cast<AHunterPlayerController>(PS->GetOwner())) // PlayerState 的 Owner 通常是 Controller
		{
			// 实际上 PlayerState 存分最方便，假设我们去获取 Pawn 里的分
			if (AHunterCharacterBase* Hunter = Cast<AHunterCharacterBase>(PC->GetPawn()))
			{
				Info.Score = Hunter->SubmittedPoints; //
			}
			else
			{
				// 玩家可能死了没复活，分还在吗？
				// 如果你想要玩家死后分不丢，SubmittedPoints 应该是安全的，
				// 但最好还是把 SubmittedPoints 移到 PlayerState 里。
				// 目前先假设玩家活着或 Controller 还在。
				Info.Score = 0; // 暂时给0，需要优化架构
			}
		}
		
		FinalScores.Add(Info);
	}

	// 2. 排序 (分数从高到低)
	FinalScores.Sort([](const FPlayerScoreInfo& A, const FPlayerScoreInfo& B) {
		return A.Score > B.Score;
	});

	// 3. 通知所有玩家：游戏结束
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (AHunterPlayerController* PC = Cast<AHunterPlayerController>(It->Get()))
		{
			// 调用 PC 的客户端函数，显示 UI 并禁用输入
			PC->ClientNotifyGameOver(FinalScores);
		}
	}
}

// 随机不重合出生点逻辑
AActor* AMiniHuntGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	// 获取场景中所有 PlayerStart
	TArray<AActor*> AllStarts;
	UGameplayStatics::GetAllActorsOfClass(this, APlayerStart::StaticClass(), AllStarts);

	if (AllStarts.Num() == 0) return Super::ChoosePlayerStart_Implementation(Player);

	// 简单的随机策略：随机找一个，如果没有人用就用它
	// 但更简单的方式是：利用 PlayerStart 的 "PlayerStartTag" 或者简单的随机乱序
	
	// 这里写一个简单的随机逻辑
	int32 RandomIndex = FMath::RandRange(0, AllStarts.Num() - 1);
	return AllStarts[RandomIndex];
	
	// 注意：UE 自带的 ChoosePlayerStart 已经有一定的防重叠机制。
	// 如果你想严格不重叠，需要维护一个 "已使用出生点列表"
}
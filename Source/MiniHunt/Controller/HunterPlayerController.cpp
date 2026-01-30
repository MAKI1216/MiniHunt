// Fill out your copyright notice in the Description page of Project Settings.


#include "MiniHunt/Controller/HunterPlayerController.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerState.h"
#include "MiniHunt/Game/MiniHuntGameState.h"


void AHunterPlayerController::PlayerCameraShake(TSubclassOf<UCameraShakeBase> CameraShake)
{
	// 使用新的 API ClientStartCameraShake 替代已弃用的 ClientPlayCameraShake
	ClientStartCameraShake(CameraShake, 1.f, ECameraShakePlaySpace::CameraLocal, FRotator::ZeroRotator);
}

void AHunterPlayerController::ClientNotifyGameOver_Implementation(const TArray<FPlayerScoreInfo>& FinalScores)
{
	// 1. 禁用输入
	SetIgnoreMoveInput(true);
	SetIgnoreLookInput(true);
	
	// 停止开火等动作
	if (APawn* MyPawn = GetPawn())
	{
		MyPawn->DisableInput(this);
	}

	// 2. 显示鼠标（为了点击 UI）
	bShowMouseCursor = true;
	SetInputMode(FInputModeUIOnly());

	// 3. 标记哪个是自己 (为了 UI 高亮)
	TArray<FPlayerScoreInfo> ProcessedScores = FinalScores;
	FString MyName = "";
	if (PlayerState)
	{
		MyName = PlayerState->GetPlayerName();
	}

	for (FPlayerScoreInfo& Info : ProcessedScores)
	{
		if (Info.PlayerName == MyName)
		{
			Info.bIsSelf = true;
		}
		else
		{
			Info.bIsSelf = false;
		}
	}

	// 4. 调用蓝图显示 UI
	ShowGameOverUI(ProcessedScores);
}
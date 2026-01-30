#include "LobbyPlayerController.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/GameStateBase.h"
#include "MiniHunt/Game/LobbyPlayerState.h"

void ALobbyPlayerController::ServerToggleReady_Implementation()
{
	if (ALobbyPlayerState* PS = GetPlayerState<ALobbyPlayerState>())
	{
		PS->bIsReady = !PS->bIsReady;
	}
}

void ALobbyPlayerController::ServerSelectCharacter_Implementation(int32 CharacterIndex)
{
	if (ALobbyPlayerState* PS = GetPlayerState<ALobbyPlayerState>())
	{
		// 如果还没准备，允许换人
		if (!PS->bIsReady)
		{
			PS->SelectedCharacterIndex = CharacterIndex;
		}
	}
}

void ALobbyPlayerController::ServerStartGame_Implementation()
{
	// 1. 只有房主(Authority)能开始
	if (!HasAuthority()) return;

	UWorld* World = GetWorld();
	if (!World) return;

	// 2. (可选) 检查是否所有人都准备好了
	// 这里为了测试方便，你可以先注释掉这个检查
	/*
	for (APlayerState* PS : World->GetGameState()->PlayerArray)
	{
		if (ALobbyPlayerState* LobbyPS = Cast<ALobbyPlayerState>(PS))
		{
			if (!LobbyPS->bIsReady) return; // 有人没准备，不许开始
		}
	}
	*/

	//todo
	// 3. 全员跳转到游戏地图 (GameMap)
	// 这里的路径要根据你的实际地图路径填！
	World->ServerTravel("/Game/Maps/GameMap?listen");
}
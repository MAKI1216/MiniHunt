#pragma once
#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "LobbyPlayerController.generated.h"

UCLASS()
class MINIHUNT_API ALobbyPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	// 切换准备状态
	UFUNCTION(BlueprintCallable, Server, Reliable)
	void ServerToggleReady();

	// 选择角色
	UFUNCTION(BlueprintCallable, Server, Reliable)
	void ServerSelectCharacter(int32 CharacterIndex);

	// 开始游戏 (仅房主)
	UFUNCTION(BlueprintCallable, Server, Reliable)
	void ServerStartGame();
};
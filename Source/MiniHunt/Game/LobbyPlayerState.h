#pragma once
#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "LobbyPlayerState.generated.h"

UCLASS()
class MINIHUNT_API ALobbyPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	// 1. 是否准备好 (同步给所有人)
	UPROPERTY(Replicated, BlueprintReadWrite, Category = "Lobby")
	bool bIsReady = false;

	// 2. 选了第几个角色 (对应 GameInstance 数组的下标)
	UPROPERTY(Replicated, BlueprintReadWrite, Category = "Lobby")
	int32 SelectedCharacterIndex = 0;

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
protected:
	// ... 原有的 GetLifetimeReplicatedProps ...

	// ➤➤➤ 【新增】重写复制属性函数，用于无缝漫游时传递数据
	virtual void CopyProperties(APlayerState* PlayerState) override;
};
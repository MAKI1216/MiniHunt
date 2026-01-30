#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "MiniHuntGameInstance.generated.h"

UCLASS()
class MINIHUNT_API UMiniHuntGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	UMiniHuntGameInstance();

	// 全局保存玩家输入的名字
	UPROPERTY(BlueprintReadWrite, Category = "Player Info")
	FString PlayerName;

	// --- 会话接口函数 (给蓝图调用) ---
	
	// 创建房间 (Host)
	UFUNCTION(BlueprintCallable)
	void CreateSession(FString RoomName, int32 MaxPlayers);

	// 寻找房间 (Client)
	UFUNCTION(BlueprintCallable)
	void FindSessions();

	// 加入房间 (Client)
	UFUNCTION(BlueprintCallable)
	void JoinFoundSession(int32 SessionIndex);

protected:
	// 内部会话接口
	IOnlineSessionPtr SessionInterface;
	TSharedPtr<FOnlineSessionSearch> SessionSearch;

	// 回调函数
	void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	void OnFindSessionsComplete(bool bWasSuccessful);
	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);

	// 蓝图事件：通知 UI 刷新
	UFUNCTION(BlueprintImplementableEvent)
	void OnSessionCreated(bool bSuccess);

	// 这里的 Array 是为了传给蓝图显示列表
	UFUNCTION(BlueprintImplementableEvent)
	void OnSessionsFound(const TArray<FString>& RoomNames); 

	UFUNCTION(BlueprintImplementableEvent)
	void OnSessionJoined(bool bSuccess);
};
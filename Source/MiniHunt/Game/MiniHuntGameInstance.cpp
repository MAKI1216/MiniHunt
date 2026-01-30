#include "MiniHunt/Game/MiniHuntGameInstance.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include "Kismet/GameplayStatics.h"

UMiniHuntGameInstance::UMiniHuntGameInstance()
{
}

void UMiniHuntGameInstance::CreateSession(FString RoomName, int32 MaxPlayers)
{
	if (IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get())
	{
		SessionInterface = Subsystem->GetSessionInterface();
		if (SessionInterface.IsValid())
		{
			// 绑定回调
			SessionInterface->OnCreateSessionCompleteDelegates.AddUObject(this, &UMiniHuntGameInstance::OnCreateSessionComplete);

			FOnlineSessionSettings SessionSettings;
			SessionSettings.bIsLANMatch = true; // 局域网
			SessionSettings.NumPublicConnections = MaxPlayers; // 4人
			SessionSettings.bShouldAdvertise = true; // 允许被搜索
			SessionSettings.bUsesPresence = true;    // 使用在线状态
			SessionSettings.bAllowJoinInProgress = true;
			
			// 把房间名存入设置，方便搜索时显示
			SessionSettings.Set(TEXT("RoomName"), RoomName, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

			const FName SessionName = FName("MySession");
			SessionInterface->CreateSession(0, SessionName, SessionSettings);
		}
	}
}

void UMiniHuntGameInstance::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	// ➤➤➤ 【新增调试日志】
	if(GEngine)
	{
		FString Msg = bWasSuccessful ? TEXT("成功！准备跳转地图...") : TEXT("失败！bWasSuccessful 为 False");
		GEngine->AddOnScreenDebugMessage(-1, 10.f, bWasSuccessful ? FColor::Green : FColor::Red, Msg);
	}

	if (bWasSuccessful)
	{
		OnSessionCreated(true);
		UGameplayStatics::OpenLevel(GetWorld(), "LobbyMap", true, "listen");
	}
	else
	{
		OnSessionCreated(false);
	}
}

void UMiniHuntGameInstance::FindSessions()
{
	if (IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get())
	{
		SessionInterface = Subsystem->GetSessionInterface();
		if (SessionInterface.IsValid())
		{
			SessionInterface->OnFindSessionsCompleteDelegates.AddUObject(this, &UMiniHuntGameInstance::OnFindSessionsComplete);

			SessionSearch = MakeShareable(new FOnlineSessionSearch());
			SessionSearch->bIsLanQuery = true; // 搜局域网
			SessionSearch->MaxSearchResults = 100;
			SessionSearch->QuerySettings.Set(FName("PRESENCE"), true, EOnlineComparisonOp::Equals);

			SessionInterface->FindSessions(0, SessionSearch.ToSharedRef());
		}
	}
}

void UMiniHuntGameInstance::OnFindSessionsComplete(bool bWasSuccessful)
{
	// 打印一下到底搜到了几个结果
	int32 Count = (bWasSuccessful && SessionSearch.IsValid()) ? SessionSearch->SearchResults.Num() : 0;
    
	if(GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Yellow, 
			FString::Printf(TEXT("搜索结束！成功状态: %d, 找到房间数: %d"), bWasSuccessful, Count));
	}
	
	TArray<FString> RoomNames;
	if (bWasSuccessful && SessionSearch.IsValid())
	{
		for (const FOnlineSessionSearchResult& Result : SessionSearch->SearchResults)
		{
			FString RoomName;
			Result.Session.SessionSettings.Get(TEXT("RoomName"), RoomName);
			if (RoomName.IsEmpty()) RoomName = "Unknown Room";
			
			// 简单拼装：房间名 + (Ping)
			RoomNames.Add(RoomName); 
		}
	}
	// 通知蓝图显示列表
	OnSessionsFound(RoomNames);
}

void UMiniHuntGameInstance::JoinFoundSession(int32 SessionIndex)
{
	if (SessionInterface.IsValid() && SessionSearch.IsValid() && SessionSearch->SearchResults.IsValidIndex(SessionIndex))
	{
		SessionInterface->OnJoinSessionCompleteDelegates.AddUObject(this, &UMiniHuntGameInstance::OnJoinSessionComplete);
		SessionInterface->JoinSession(0, FName("MySession"), SessionSearch->SearchResults[SessionIndex]);
	}
}

void UMiniHuntGameInstance::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	if (Result == EOnJoinSessionCompleteResult::Success)
	{
		FString ConnectInfo;
		bool bGotAddress = SessionInterface->GetResolvedConnectString(SessionName, ConnectInfo);

		// ➤➤➤ 【关键修改开始】编辑器模式下，强制连本机 127.0.0.1
#if WITH_EDITOR
		// 打印原始获取到的地址（用于调试）
		UE_LOG(LogTemp, Warning, TEXT("原始连接地址: %s"), *ConnectInfo);
        
		// 无论获取到什么，只要是在编辑器里双开测试，强行改回环地址
		// 这能 100% 解决防火墙、路由器不支持回环、虚拟网卡IP错误等所有连接问题
		ConnectInfo = TEXT("127.0.0.1"); 
        
		if(GEngine) 
		{
			GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Magenta, 
				TEXT("【编辑器模式】强制重定向连接到: 127.0.0.1"));
		}
#endif
		// ➤➤➤ 【关键修改结束】

		if (!ConnectInfo.IsEmpty())
		{
			OnSessionJoined(true);
			if (APlayerController* PC = GetFirstLocalPlayerController())
			{
				PC->ClientTravel(ConnectInfo, TRAVEL_Absolute);
			}
		}
		else
		{
			if(GEngine) GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, TEXT("获取连接信息失败！"));
		}
	}
}
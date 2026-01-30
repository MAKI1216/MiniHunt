#include "LobbyPlayerState.h"
#include "Net/UnrealNetwork.h"

void ALobbyPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	// 注册同步变量，任何一方修改，所有人都会收到更新
	DOREPLIFETIME(ALobbyPlayerState, bIsReady);
	DOREPLIFETIME(ALobbyPlayerState, SelectedCharacterIndex);
}
void ALobbyPlayerState::CopyProperties(APlayerState* PlayerState)
{
	Super::CopyProperties(PlayerState);

	// 把数据从“旧我”身上，拷贝到“新我”身上
	if (ALobbyPlayerState* NewLobbyPS = Cast<ALobbyPlayerState>(PlayerState))
	{
		NewLobbyPS->SelectedCharacterIndex = this->SelectedCharacterIndex;
		NewLobbyPS->bIsReady = this->bIsReady;
        
		// 打印一条日志证明数据传过去了
		UE_LOG(LogTemp, Warning, TEXT("CopyProperties: 搬运选角数据 Index = %d"), this->SelectedCharacterIndex);
	}
}
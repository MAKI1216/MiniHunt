#include "MiniHunt/Game/MiniHuntGameState.h"
#include "Net/UnrealNetwork.h"

void AMiniHuntGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// 注册要同步的变量
	DOREPLIFETIME(AMiniHuntGameState, RemainingTime);
}
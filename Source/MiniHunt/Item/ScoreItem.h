#pragma once

#include "CoreMinimal.h"
#include "ItemBase.h"
#include "ScoreItem.generated.h"

UCLASS()
class MINIHUNT_API AScoreItem : public AItemBase
{
	GENERATED_BODY()
    
public:
	AScoreItem();

	// 这个道具值多少分？
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Score")
	int32 ScoreValue;
};
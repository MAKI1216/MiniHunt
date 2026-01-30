#pragma once

#include "CoreMinimal.h"
#include "ItemBase.h"
#include "MiniHunt/Weapon/GunBase.h" // 获取 EBulletType
#include "AmmoItem.generated.h"

UCLASS()
class MINIHUNT_API AAmmoItem : public AItemBase
{
	GENERATED_BODY()
    
public:
	AAmmoItem();

	// 弹药类型
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ammo")
	EBulletType BulletType;

	// 增加的数量
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ammo")
	int32 AmmoCount;
};
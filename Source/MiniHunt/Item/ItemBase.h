// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ItemBase.generated.h"

class USphereComponent;
class UWidgetComponent;
class AHunterCharacterBase;

// 道具类型枚举
UENUM(BlueprintType)
enum class EItemType : uint8
{
	EIT_Ammo    UMETA(DisplayName = "Ammo"),
	EIT_Effect  UMETA(DisplayName = "Effect"), // 留空
	EIT_Score   UMETA(DisplayName = "Score")   // 留空
};

UCLASS()
class MINIHUNT_API AItemBase : public AActor
{
	GENERATED_BODY()
    
public:    
	AItemBase();

protected:
	virtual void BeginPlay() override;

public:
	// --- 组件 ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item Components")
	UStaticMeshComponent* ItemMesh;

	// 小范围检测球：用于显示 UI 文字
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item Components")
	USphereComponent* PickupAreaSphere;

	// UI 组件：显示 "按F拾取..."
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item Components")
	UWidgetComponent* PickupWidget;

	// --- 属性 ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Properties")
	FString ItemName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Properties")
	EItemType ItemType;

	// --- 接口 ---
    
	// 开启/关闭描边 (由角色的探测球调用)
	void EnableOutline(bool bEnable);

protected:
	// ➤➤➤ 【核心修改】定义一个蓝图实现的事件
	// C++ 只管调用，具体怎么修改 UI 文字，由蓝图决定
	UFUNCTION(BlueprintImplementableEvent, Category = "UI")
	void BP_SetItemName(const FString& Name);
	
	// UI 重叠事件
	UFUNCTION()
	virtual void OnPickupAreaBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	virtual void OnPickupAreaEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
};
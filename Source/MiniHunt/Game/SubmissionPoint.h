#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SubmissionPoint.generated.h"

class UWidgetComponent;
class USphereComponent;

UCLASS()
class MINIHUNT_API ASubmissionPoint : public AActor
{
	GENERATED_BODY()
    
public:    
	ASubmissionPoint();

protected:
	virtual void BeginPlay() override;

public:
	// --- 组件 ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USceneComponent* RootScene; // 根节点

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* CartMesh; // 车（如果是骨骼动画就换 USkeletalMeshComponent）

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* HorseMesh; // 马

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USphereComponent* InteractSphere; // 检测球

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UWidgetComponent* InteractWidget; // 提示 UI ("按F提交")

protected:
	// 重叠事件
	UFUNCTION()
	void OnBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
};
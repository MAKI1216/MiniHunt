// Fill out your copyright notice in the Description page of Project Settings.


#include "MiniHunt/Item/ItemBase.h"

#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "MiniHunt/Character/HunterCharacterBase.h"

AItemBase::AItemBase()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true; // 道具必须同步

    // 1. Mesh
    ItemMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ItemMesh"));
    SetRootComponent(ItemMesh);
    ItemMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    ItemMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore); // 防止挡路
    // 开启 CustomDepth 以支持描边，默认 Stencil Value 为 0 (不显示)
    ItemMesh->SetRenderCustomDepth(true);
    ItemMesh->SetCustomDepthStencilValue(0); 

    // 2. Pickup Sphere (UI 触发范围)
    PickupAreaSphere = CreateDefaultSubobject<USphereComponent>(TEXT("PickupAreaSphere"));
    PickupAreaSphere->SetupAttachment(RootComponent);
    PickupAreaSphere->SetSphereRadius(150.f);

    // 3. Widget
    PickupWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("PickupWidget"));
    PickupWidget->SetupAttachment(RootComponent);
    PickupWidget->SetWidgetSpace(EWidgetSpace::Screen); // 屏幕空间，始终朝向玩家
    PickupWidget->SetVisibility(false); // 默认隐藏
    PickupWidget->SetRelativeLocation(FVector(0, 0, 50));
    
    // 默认关闭 CustomDepth，省一点点渲染开销
    ItemMesh->SetRenderCustomDepth(false); 
    ItemMesh->SetCustomDepthStencilValue(0);
}

void AItemBase::BeginPlay()
{
    Super::BeginPlay();
    
    // 绑定重叠事件
    PickupAreaSphere->OnComponentBeginOverlap.AddDynamic(this, &AItemBase::OnPickupAreaBeginOverlap);
    PickupAreaSphere->OnComponentEndOverlap.AddDynamic(this, &AItemBase::OnPickupAreaEndOverlap);
    
    // 更新ui名字，蓝图调用
    BP_SetItemName(ItemName);
}

// 描边控制：设置 Stencil Value
// 假设后处理材质中：0=无，250=黄色，251=红色等。这里假设 250 是黄色。
void AItemBase::EnableOutline(bool bEnable)
{
    if (ItemMesh)
    {
        // 开启/关闭 深度渲染
        ItemMesh->SetRenderCustomDepth(bEnable);
        
        // 设置颜色值 (假设材质里 252 是黄色)
        ItemMesh->SetCustomDepthStencilValue(bEnable ? 252 : 0); 
    }
}

void AItemBase::OnPickupAreaBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    AHunterCharacterBase* Hunter = Cast<AHunterCharacterBase>(OtherActor);
    if (Hunter && Hunter->IsLocallyControlled())
    {
        // ➤➤➤ 【新增修复】 排除法
        // 如果碰到我的组件不是角色的根组件（胶囊体），直接忽略！
        // 这样 DiscoverySphere（大圈）碰到我时，这行代码会让逻辑直接返回，不显示 UI
        if (OtherComp != Hunter->GetRootComponent()) 
        {
            return;
        }

        PickupWidget->SetVisibility(true);
        Hunter->SetOverlappingItem(this); 
    }
}

void AItemBase::OnPickupAreaEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    AHunterCharacterBase* Hunter = Cast<AHunterCharacterBase>(OtherActor);
    if (Hunter && Hunter->IsLocallyControlled())
    {
        // ➤➤➤ 【新增修复】 同样的排除法
        // 只有当身体离开时，才隐藏 UI
        if (OtherComp != Hunter->GetRootComponent()) 
        {
            return;
        }

        PickupWidget->SetVisibility(false);
        if (Hunter->GetOverlappingItem() == this)
        {
            Hunter->SetOverlappingItem(nullptr);
        }
    }
}
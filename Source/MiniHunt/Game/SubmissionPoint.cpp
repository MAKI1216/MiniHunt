#include "MiniHunt/Game/SubmissionPoint.h" // 路径根据你实际文件夹调整
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "MiniHunt/Character/HunterCharacterBase.h"

ASubmissionPoint::ASubmissionPoint()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true; // 虽然是静态的，但为了保险起见开启同步

    // 1. Root
    RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
    SetRootComponent(RootScene);

    // 2. Meshes
    CartMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CartMesh"));
    CartMesh->SetupAttachment(RootScene);
    
    HorseMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HorseMesh"));
    HorseMesh->SetupAttachment(RootScene);
    // 调整马的位置，比如放在车前面
    HorseMesh->SetRelativeLocation(FVector(200.f, 0.f, 0.f)); 

    // 3. Detection Sphere
    InteractSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractSphere"));
    InteractSphere->SetupAttachment(RootScene);
    InteractSphere->SetSphereRadius(300.f); // 范围大一点，方便停车
    InteractSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
    InteractSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

    // 4. Widget
    InteractWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("InteractWidget"));
    InteractWidget->SetupAttachment(RootScene);
    InteractWidget->SetVisibility(false);
    InteractWidget->SetWidgetSpace(EWidgetSpace::Screen);
    InteractWidget->SetRelativeLocation(FVector(0, 0, 200)); // 飘在头顶
}

void ASubmissionPoint::BeginPlay()
{
    Super::BeginPlay();
    
    // ✅ 修复版：先检查是否已经绑定，防止重复绑定导致的报错断开
    if (!InteractSphere->OnComponentBeginOverlap.IsAlreadyBound(this, &ASubmissionPoint::OnBeginOverlap))
    {
        InteractSphere->OnComponentBeginOverlap.AddDynamic(this, &ASubmissionPoint::OnBeginOverlap);
    }

    if (!InteractSphere->OnComponentEndOverlap.IsAlreadyBound(this, &ASubmissionPoint::OnEndOverlap))
    {
        InteractSphere->OnComponentEndOverlap.AddDynamic(this, &ASubmissionPoint::OnEndOverlap);
    }
}

void ASubmissionPoint::OnBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    AHunterCharacterBase* Hunter = Cast<AHunterCharacterBase>(OtherActor);
    // 必须是本地控制的角色才显示 UI
    if (Hunter && Hunter->IsLocallyControlled()) 
    {
        InteractWidget->SetVisibility(true);
        Hunter->SetOverlappingSubmissionPoint(this); // 告诉角色：你现在在提交点范围内
    }
}

void ASubmissionPoint::OnEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    AHunterCharacterBase* Hunter = Cast<AHunterCharacterBase>(OtherActor);
    if (Hunter && Hunter->IsLocallyControlled())
    {
        InteractWidget->SetVisibility(false);
        // 如果离开的是当前记录的提交点，清空
        if (Hunter->GetOverlappingSubmissionPoint() == this)
        {
            Hunter->SetOverlappingSubmissionPoint(nullptr);
        }
    }
}
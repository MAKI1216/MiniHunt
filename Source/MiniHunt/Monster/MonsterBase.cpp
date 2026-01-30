// Fill out your copyright notice in the Description page of Project Settings.


#include "MiniHunt/Monster/MonsterBase.h"

#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "MiniHunt/Character/HunterCharacterBase.h"
#include "MiniHunt/Controller/MonsterAIController.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SkinnedMeshComponent.h"

AMonsterBase::AMonsterBase()
{
    PrimaryActorTick.bCanEverTick = true;
    
    // 1. 配置胶囊体 (Capsule)
    GetCapsuleComponent()->SetCollisionProfileName(TEXT("Pawn"));
    // ➤➤➤ 【关键修改】让胶囊体忽略射线检测 (Visibility)
    // 这样子弹不会打在胶囊体上，而是穿过去打在里面的 Mesh 上 (为了能检测到 Physical Material 做爆头)
    GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
    // 同时忽略相机，防止相机贴近时被推开
    GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);

    // 2. 配置网格体 (Mesh)
    // ➤➤➤ 【关键修改】让网格体阻挡射线！
    GetMesh()->SetCollisionObjectType(ECC_Pawn);
    GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryOnly); // 开启查询碰撞 (不需要物理模拟)
    
    // 默认先忽略所有，然后单独开启需要的
    GetMesh()->SetCollisionResponseToAllChannels(ECR_Ignore);
    
    // A. 必须阻挡 Visibility (子弹是这个通道)
    GetMesh()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    
    // B. 忽略相机 (防止穿模时相机被卡住)
    GetMesh()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
    
    // C. 忽略 Pawn (防止怪物和玩家的胶囊体发生物理挤压)
    GetMesh()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);

    // AI 设置 (保持不变)
    bUseControllerRotationYaw = false;
    GetCharacterMovement()->bOrientRotationToMovement = true;
}

void AMonsterBase::BeginPlay()
{
    Super::BeginPlay();
    CurrentHealth = MaxHealth;
}

// Called every frame
void AMonsterBase::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AMonsterBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

}


float AMonsterBase::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
    if (bIsDead) return 0.0f;

    float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
    
    // 扣血
    CurrentHealth -= DamageAmount;
    UE_LOG(LogTemp, Warning, TEXT("Monster took damage: %f, Remaining: %f"), DamageAmount, CurrentHealth);

    // AI 感知更新：如果你被打，应该立刻感知到攻击者（仇恨系统）
    if (AMonsterAIController* AIC = Cast<AMonsterAIController>(GetController()))
    {
        AIC->SetTargetEnemy(DamageCauser);
    }

    // 死亡判定
    if (CurrentHealth <= 0.0f)
    {
        Die(DamageCauser);
    }

    return DamageAmount;
}

void AMonsterBase::Die(AActor* Killer)
{
    // 1. 防止重复死亡
    if (bIsDead) return;
    bIsDead = true;

    // 2. 加分逻辑 (保持不变)
    if (Killer)
    {
        AHunterCharacterBase* PlayerHunter = Cast<AHunterCharacterBase>(Killer);
        if (!PlayerHunter && Killer->GetOwner())
        {
            PlayerHunter = Cast<AHunterCharacterBase>(Killer->GetOwner());
        }

        if (PlayerHunter)
        {
            PlayerHunter->AddCarryingPoints(ScoreValue);
        }
    }

    // 3. 停止 AI (保持不变)
    if (AController* AICon = GetController())
    {
        if (AMonsterAIController* MonsterAIC = Cast<AMonsterAIController>(AICon))
        {
            MonsterAIC->StopMovement();
        }
        AICon->UnPossess(); 
    }

    // 4. 执行多播表现 (播放动画)
    MultiDeathEffects();

    // 5. 设置销毁倒计时
    // 建议设置为：动画长度 + 2秒缓冲，或者直接给个 5.0f
    // 如果你有蒙太奇，可以动态获取长度：
    float LifeTime = 5.0f;
    if (DeathMontage)
    {
        LifeTime = DeathMontage->GetPlayLength() + 2.0f;
    }
    SetLifeSpan(LifeTime);
}

void AMonsterBase::Attack()
{
    if (bIsDead) return;
    if (AttackMontage)
    {
        PlayAnimMontage(AttackMontage);
    }
}

// 这个函数必须在动画蓝图中通过 AnimNotify 来调用！
// 在挥手动作最有力的一瞬间调用它
void AMonsterBase::AttackHitCheck()
{
    FVector Start = GetActorLocation();
    FVector End = Start + GetActorForwardVector() * AttackRange;
    
    TArray<FHitResult> HitResults;
    FCollisionShape Sphere = FCollisionShape::MakeSphere(50.0f); // 攻击判定球半径

    // 只检测 Pawn (玩家)
    bool bHit = GetWorld()->SweepMultiByChannel(
        HitResults, Start, End, FQuat::Identity, 
        ECC_Pawn, Sphere
    );

    if (bHit)
    {
        for (auto& Hit : HitResults)
        {
            if (AActor* HitActor = Hit.GetActor())
            {
                // 防止打到自己
                if (HitActor == this) continue;

                // 造成伤害
                UGameplayStatics::ApplyDamage(
                    HitActor, AttackDamage, GetController(), this, UDamageType::StaticClass()
                );
            }
        }
    }
}

void AMonsterBase::SetMovementSpeed(float NewSpeed)
{
    GetCharacterMovement()->MaxWalkSpeed = NewSpeed;
}


//多播动画
void AMonsterBase::Multi_PlayMontage_Implementation(UAnimMontage* MontageToPlay)
{
    // 逻辑：如果是“模拟代理（Simulated Proxy）”，也就是客户端里的那个丧尸影子，就强制播放
    // 服务器自己（Authority）已经在行为树里播了，所以这里不用播，防止重叠鬼畜
    if (GetLocalRole() != ROLE_Authority)
    {
        if (MontageToPlay)
        {
            PlayAnimMontage(MontageToPlay);
        }
    }
}

// 实现多播函数
void AMonsterBase::MultiDeathEffects_Implementation()
{
    // 1. 停止当前动画 (Idle/Run)
    if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
    {
        AnimInstance->StopAllMontages(0.0f);
    }

    // 2. 播放死亡蒙太奇
    // ⚠️ 必须确保动画蓝图 AnimGraph 里加了 Slot 节点，否则这句代码无效！
    if (DeathMontage)
    {
        PlayAnimMontage(DeathMontage);
    }
    
    // 3. 关闭胶囊体碰撞 (不再阻挡玩家)
    GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    // 4. 关闭网格体碰撞 (让它变成纯视觉物体)
    // 这一步是正确的，因为播动画不需要物理碰撞
    GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    // 5. 彻底冻结移动
    if (GetCharacterMovement())
    {
        GetCharacterMovement()->StopMovementImmediately();
        GetCharacterMovement()->DisableMovement();
    }
}

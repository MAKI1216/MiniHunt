// Fill out your copyright notice in the Description page of Project Settings.


#include "MiniHunt/Weapon/ServerGunBase.h"

// Sets default values
AServerGunBase::AServerGunBase()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AServerGunBase::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AServerGunBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}


// Fill out your copyright notice in the Description page of Project Settings.


#include "MiniHunt/Controller/HunterPlayerController.h"


void AHunterPlayerController::PlayerCameraShake(TSubclassOf<UCameraShakeBase> CameraShake)
{
	// 使用新的 API ClientStartCameraShake 替代已弃用的 ClientPlayCameraShake
	ClientStartCameraShake(CameraShake, 1.f, ECameraShakePlaySpace::CameraLocal, FRotator::ZeroRotator);
}

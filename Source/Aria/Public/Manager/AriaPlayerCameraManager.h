// Copyright (c) SPC Gaming. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Camera/PlayerCameraManager.h"
#include "AriaPlayerCameraManager.generated.h"

class AAriaCharacter;

DECLARE_LOG_CATEGORY_EXTERN(LogAriaCameraManager, Log, All);

USTRUCT()
struct FDeadFollowZone
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly) bool bEnabled = true;
	UPROPERTY(EditDefaultsOnly) float LeftOffset = 100.f;
	UPROPERTY(EditDefaultsOnly) float RightOffset = 100.f;
	UPROPERTY(EditDefaultsOnly) float TopOffset = 100.f;
};

UCLASS()
class ARIA_API AAriaPlayerCameraManager : public APlayerCameraManager
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;
	virtual void UpdateViewTarget(FTViewTarget& OutVT, float DeltaTime) override;

private:
	UPROPERTY(Transient) TObjectPtr<AAriaCharacter> AriaCharacterOwner;

	// Camera Lag
	UPROPERTY(EditDefaultsOnly, Category="Camera|Lag") bool bEnableFollowLag = true;
	UPROPERTY(EditDefaultsOnly, Category="Camera|Lag") float FollowLagSpeed = 6.f;

	// Dead Zone
	UPROPERTY(EditDefaultsOnly, Category="Camera") FDeadFollowZone DeadFollowZone;
	float GetFollowTargetX(const FVector& TargetLocation, const FVector& CameraLocation) const;
	float GetFollowTargetZ(const FVector& TargetLocation, const FVector& CameraLocation, const FVector& ViewTargetLocation) const;
	
	void FollowCharacter(FTViewTarget& OutVT, float DeltaTime);
};

// Copyright (c) SPC Gaming. All rights reserved.

#include "Manager/AriaPlayerCameraManager.h"
#include "DrawDebugHelpers.h"
#include "Character/AriaCharacter.h"
#include "Utils/AriaMath.h"
#include "GameFramework/PlayerController.h"

DEFINE_LOG_CATEGORY(LogAriaCameraManager);

void AAriaPlayerCameraManager::BeginPlay()
{
	Super::BeginPlay();
	AriaCharacterOwner = Cast<AAriaCharacter>(GetOwningPlayerController()->GetPawn());
	check(AriaCharacterOwner);
}

void AAriaPlayerCameraManager::UpdateViewTarget(FTViewTarget& OutVT, float DeltaTime)
{
	Super::UpdateViewTarget(OutVT, DeltaTime);
	FollowCharacter(OutVT, DeltaTime);
}

void AAriaPlayerCameraManager::FollowCharacter(FTViewTarget& OutVT, const float DeltaTime)
{
	const FVector CameraLocation = GetCameraLocation();
	const FVector TargetLocation = AriaCharacterOwner->GetActorLocation();
	const FVector ViewTargetLocation = ViewTarget.POV.Location;
	const float FollowTargetX = GetFollowTargetX(TargetLocation, CameraLocation);
	const float FollowTargetZ = GetFollowTargetZ(TargetLocation, CameraLocation, ViewTargetLocation);
	const FVector NewCameraLocation = FVector(FollowTargetX, ViewTargetLocation.Y, FollowTargetZ);

	if (bEnableFollowLag)
	{
		const FVector CurrentCameraLocation = CameraLocation.IsNearlyZero() ? ViewTargetLocation : CameraLocation;
		ViewTarget.POV.Location = FMath::VInterpTo(CurrentCameraLocation, NewCameraLocation, DeltaTime, FollowLagSpeed);
	}
	else
	{
		ViewTarget.POV.Location = CameraLocation.IsNearlyZero() ? ViewTargetLocation : NewCameraLocation;
	}
}

float AAriaPlayerCameraManager::GetFollowTargetX(const FVector& TargetLocation, const FVector& CameraLocation) const
{
	// Follow the hero if Dead Zone doesn't enabled or horizontal limits are zero 
	if (!DeadFollowZone.bEnabled || (FMath::IsNearlyZero(DeadFollowZone.LeftOffset) && FMath::IsNearlyZero(DeadFollowZone.RightOffset)))
	{
		return TargetLocation.X;
	}

	// Move the camera Left after the Target reached the left limit
	const float LeftDeadZone = TargetLocation.X - DeadFollowZone.LeftOffset;
	if (FAriaMath::IsFirstGreater(LeftDeadZone, CameraLocation.X))
	{
		return LeftDeadZone;
	}

	// Move the camera Right after the Target reached the right limit
	const float RightDeadZone = TargetLocation.X + DeadFollowZone.RightOffset;
	if (FAriaMath::IsFirstLess(RightDeadZone, CameraLocation.X))
	{
		return RightDeadZone;
	}

	// Camera stays in one place
	return CameraLocation.X;
}

float AAriaPlayerCameraManager::GetFollowTargetZ(const FVector& TargetLocation, const FVector& CameraLocation, const FVector& ViewTargetLocation) const
{
	// Follow the hero if Dead Zone doesn't enabled or top limit are zero 
	if (!DeadFollowZone.bEnabled || FMath::IsNearlyZero(DeadFollowZone.TopOffset))
	{
		return ViewTargetLocation.Z;
	}

	// Move the camera Up after the Target has reached the top limit
	const float TopDeadZone = TargetLocation.Z - DeadFollowZone.TopOffset;
	if (FAriaMath::IsFirstGreater(TopDeadZone, CameraLocation.Z, 5))
	{
		return TopDeadZone;
	}

	// Move the camera Down after the Target has reached the bottom limit
	const float BottomDeadZone = ViewTargetLocation.Z;
	if (FAriaMath::IsFirstLess(BottomDeadZone, CameraLocation.Z))
	{
		return BottomDeadZone;
	}

	// If character is grounded set camera to the bottom zone
	FFindFloorResult FloorResult;
	AriaCharacterOwner->GetCharacterMovement()->FindFloor(TargetLocation, FloorResult, false);
	if (FloorResult.IsWalkableFloor())
	{
		return BottomDeadZone;
	}

	// Camera stays in one place
	return CameraLocation.Z;
}

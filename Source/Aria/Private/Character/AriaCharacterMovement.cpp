// Copyright (c) SPC Gaming. All rights reserved.

#include "Character/AriaCharacterMovement.h"
#include "DrawDebugHelpers.h"
#include "StaticMeshAttributes.h"
#include "Animation/CrawlingAnimNotify.h"
#include "Animation/DashAnimNotify.h"
#include "Animation/FallingToRollAnimNotify.h"
#include "Animation/HardLandingAnimNotify.h"
#include "Animation/MantleAnimNotify.h"
#include "Character/AriaCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Curves/CurveFloat.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStaticsTypes.h"
#include "Utils/VariableStorage.h"

DEFINE_LOG_CATEGORY(LogAriaCharacterMovement);

UAriaCharacterMovement::UAriaCharacterMovement()
{
	RotationRate = FRotator(0.f, 3072.f, 0.f);
	AirControl = 1.f;
	Mass = 500.f;
	JumpZVelocity = 1450.f;
	MaxWalkSpeed = 700.f;
	GravityScale = 3.f;
	MaxAcceleration = 4096.f;
	bConstrainToPlane = true;
	bOrientRotationToMovement = true;
	UMovementComponent::SetPlaneConstraintNormal(FVector::UnitY());

	// Crouch
	MaxWalkSpeedCrouched = 200.f;
	GetNavAgentPropertiesRef().bCanCrouch = true;
}

void UAriaCharacterMovement::InitializeComponent()
{
	Super::InitializeComponent();

	AriaCharacterOwner = Cast<AAriaCharacter>(GetOwner());
	check(AriaCharacterOwner);

	AriaCharacterOwner->LandedDelegate.AddDynamic(this, &UAriaCharacterMovement::OnLanded);
	InitAnimations();
}

void UAriaCharacterMovement::UpdateCharacterStateBeforeMovement(float DeltaSeconds)
{
	// Wall Sliding
	TryWallSlide();

	// Sliding
	if (FFindFloorResult FloorHit; IsMovingOnGround() && bWantsToSlide && CanSlide(FloorHit))
	{
		EnterSlide();
	}

	if (IsSliding() && !bWantsToSlide)
	{
		ExitSlide();
	}

	// Rope Walking
	TryRopeWalking();

	// Push Objects
	TryPushing();

	// Crawling
	if (FFindFloorResult FloorHit; IsMovingOnGround() && bWantsToCrawling && CanCrawling(FloorHit))
	{
		EnterCrawling();
	}
	
	if (IsCrawling() && !bWantsToCrawling)
	{
		ExitCrawling();
	}

	// Mantle
	TryMantle();

	// Climb Ladder
	TryClimbLadder();

	// Dash
	TryDash();

	// Ice Sliding
	TryIceSliding();

	Super::UpdateCharacterStateBeforeMovement(DeltaSeconds);
}

void UAriaCharacterMovement::UpdateCharacterStateAfterMovement(float DeltaSeconds)
{
	Super::UpdateCharacterStateAfterMovement(DeltaSeconds);

	const TSharedPtr<FRootMotionSource> RootMotion = GetRootMotionSourceByID(RootMotionSourceID);
	if (RootMotion && RootMotion->Status.HasFlag(ERootMotionSourceStatusFlags::Finished))
	{
		RemoveRootMotionSourceByID(RootMotionSourceID);
	}
}

float UAriaCharacterMovement::GetMaxSpeed() const
{
	const float MaxSpeed = Super::GetMaxSpeed();
	if (MovementMode != MOVE_Custom)
	{
		return MaxSpeed;
	}

	switch (CustomMovementMode)
	{
		case CMOVE_RopeWalk: return MaxRopeWalkingSpeed;
		case CMOVE_Pushing: return MaxPushingSpeed;
		case CMOVE_Crawling: return MaxCrawlingSpeed;
		case CMOVE_IceSliding: return MaxIceSlidingSpeed;
		default: return MaxSpeed;
	}
}

float UAriaCharacterMovement::GetSpeed() const
{
	return Velocity.Length();
}

bool UAriaCharacterMovement::IsWalk() const
{
	if (!IsWalking() || FMath::IsNearlyZero(GetSpeed()))
	{
		return false;
	}

	if (IsCrouching() || IsSliding() || IsRopeWalking() || IsPushing() || IsCrawling())
	{
		return false;
	}

	return true;
}

bool UAriaCharacterMovement::CanAttemptJump() const
{
	return Super::CanAttemptJump() || IsWallSliding();
}

bool UAriaCharacterMovement::DoJump(bool bReplayingMoves)
{
	const bool bIsWallSliding = IsWallSliding();
	if (Super::DoJump(bReplayingMoves))
	{
		if (bIsWallSliding)
		{
			FHitResult HitResult;
			FCollisionQueryParams QueryParams = AriaCharacterOwner->GetQueryParams();
			const FVector Start = UpdatedComponent->GetComponentLocation();
			GetWorld()->LineTraceSingleByProfile(HitResult, Start, EndForwardVector(Start), "BlockAll", QueryParams);
			Velocity += HitResult.Normal * WallJumpOffForce;
		}

		return true;
	}

	return false;
}

void UAriaCharacterMovement::PhysCustom(float deltaTime, int32 Iterations)
{
	Super::PhysCustom(deltaTime, Iterations);

	switch (CustomMovementMode)
	{
		case CMOVE_WallSliding:
			PhysWallSlide(deltaTime, Iterations);
			break;
		case CMOVE_Slide:
			PhysSlide(deltaTime, Iterations);
			break;
		case CMOVE_RopeWalk:
			PhysRopeWalking(deltaTime, Iterations);
			break;
		case CMOVE_Pushing:
			PhysPushing(deltaTime, Iterations);
			break;
		case CMOVE_Crawling:
			PhysCrawling(deltaTime, Iterations);
			break;
		case CMOVE_ClimbLadder:
			PhysClimbLadder(deltaTime, Iterations);
			break;
		case CMOVE_IceSliding:
			PhysIceSliding(deltaTime, Iterations);
			break;
		default:
			UE_LOG(LogAriaCharacterMovement, Error, TEXT("Invalid Movement Mode"))
	}
}

#pragma region "Wall Slide"
void UAriaCharacterMovement::TryWallSlide()
{
	if (!IsFalling())
	{
		return;
	}

	FHitResult FloorHitResult, WallHitResult;
	FCollisionQueryParams QueryParams = AriaCharacterOwner->GetQueryParams();
	const FVector Start = UpdatedComponent->GetComponentLocation();
	
	// exit if the height to floor is smaller than MinHeightToSlide
	if (GetWorld()->LineTraceSingleByProfile(FloorHitResult, Start, EndDownVector(Start), "BlockAll", QueryParams))
	{
		return;
	}

	// exit if the character is not close to the wall
	GetWorld()->LineTraceSingleByProfile(WallHitResult, Start, EndForwardVector(Start), "BlockAll", QueryParams);
	if (!WallHitResult.IsValidBlockingHit() || (Velocity | WallHitResult.Normal) >= 0)
	{
		return;
	}

	// exit if the actor is ladder
	TObjectPtr<AActor> WallActor = WallHitResult.GetActor();
	if (WallActor && WallActor->ActorHasTag(ClimbLadderTag))
	{
		return;
	}
	
	// all is good, go to wall slide
	Velocity = FVector::VectorPlaneProject(Velocity, WallHitResult.Normal);
	Velocity.Z = FMath::Clamp(Velocity.Z, 0, MaxVerticalWallSlideSpeed);
	SetMovementMode(MOVE_Custom, CMOVE_WallSliding);
}

void UAriaCharacterMovement::PhysWallSlide(float DeltaTime, int32 Iterations)
{
	if (DeltaTime < MIN_TICK_TIME)
	{
		return;
	}

	if (CannotPerformPhysMovement())
	{
		Acceleration = FVector::ZeroVector;
		Velocity = FVector::ZeroVector;
		return;
	}

	bJustTeleported = false;
	float RemainingTime = DeltaTime;
	while (CanPerformFrameTickMovement(RemainingTime, Iterations))
	{
		Iterations++;
		bJustTeleported = false;
		const float TimeTick = GetSimulationTimeStep(RemainingTime, Iterations);
		RemainingTime -= TimeTick;

		// exit if the character is grounded
		FHitResult WallHitResult;
		const FVector OldLocation = UpdatedComponent->GetComponentLocation();
		FVector Start = UpdatedComponent->GetComponentLocation();
		FCollisionQueryParams QueryParams = AriaCharacterOwner->GetQueryParams();
		GetWorld()->LineTraceSingleByProfile(WallHitResult, Start, EndForwardVector(Start), "BlockAll", QueryParams);
		if (!WallHitResult.IsValidBlockingHit())
		{
			SetMovementMode(MOVE_Falling);
			StartNewPhysics(RemainingTime, Iterations);
			return;
		}

		// clamp acceleration
		Acceleration = FVector::ZeroVector;

		// apply acceleration
		CalcVelocity(TimeTick, 0.f, false, GetMaxBrakingDeceleration());
		Velocity = FVector::VectorPlaneProject(Velocity, WallHitResult.Normal);
		const bool bVelocityUp = Velocity.Z > 0.f;
		const float TangentAccel = Acceleration.GetSafeNormal2D() | Velocity.GetSafeNormal2D();
		Velocity.Z += GetGravityZ() * WallSlideGravityCurve->GetFloatValue(bVelocityUp ? 0.f : TangentAccel) * TimeTick;

		// compute move parameters
		FHitResult HitResult;
		MoveUpdatedComponent(TimeTick * Velocity, UpdatedComponent->GetComponentQuat(), true, &HitResult);
		if (UpdatedComponent->GetComponentLocation() == OldLocation)
		{
			RemainingTime = 0.f;
			break;
		}

		Velocity = (UpdatedComponent->GetComponentLocation() - OldLocation) / TimeTick;
	}

	// check again if the slide conditions are met
	FHitResult FloorHitResult, WallHitResult;
	FVector Start = UpdatedComponent->GetComponentLocation();
	FCollisionQueryParams QueryParams = AriaCharacterOwner->GetQueryParams();
	GetWorld()->LineTraceSingleByProfile(WallHitResult, Start, EndForwardVector(Start), "BlockAll", QueryParams);
	GetWorld()->LineTraceSingleByProfile(FloorHitResult, Start, EndDownVector(Start), "BlockAll", QueryParams);
	if (FloorHitResult.IsValidBlockingHit() || !WallHitResult.IsValidBlockingHit())
	{
		SetMovementMode(MOVE_Falling);
	}
}

FVector UAriaCharacterMovement::EndDownVector(const FVector& Start) const
{
	return Start + FVector::DownVector * (GetCapsuleRadius() + MinHeightToSlide);
}

FVector UAriaCharacterMovement::EndForwardVector(const FVector& Start) const
{
	return Start + UpdatedComponent->GetForwardVector() * GetCapsuleRadius() * 2.2f;
}
#pragma endregion

#pragma region "Slide"
bool UAriaCharacterMovement::CanSlide(FFindFloorResult& FloorHit) const
{
	if (Velocity.SizeSquared() <= pow(MinSpeedToEnterSlide, 2))
	{
		return false;
	}

	FindFloor(UpdatedComponent->GetComponentLocation(), FloorHit, false);
	return FloorHit.bWalkableFloor;
}

void UAriaCharacterMovement::EnterSlide()
{
	SlidingTime = 0.f;
	Velocity += Velocity.GetSafeNormal2D() * EnterSlideImpulse;

	SetCollisionSizeToSlidingDimensions();
	SetMovementMode(MOVE_Custom, CMOVE_Slide);
}

void UAriaCharacterMovement::ExitSlide()
{
	if (!RestoreDefaultCollisionDimensions())
	{
		return;
	}

	// restore character rotation
	const FQuat NewRotation = FRotationMatrix::MakeFromXZ(UpdatedComponent->GetForwardVector().GetSafeNormal2D(), FVector::UpVector).ToQuat();
	MoveUpdatedComponent(FVector::ZeroVector, NewRotation, true);

	bWantsToSlide = false;
	SetMovementMode(DefaultLandMovementMode);
}

void UAriaCharacterMovement::PhysSlide(float DeltaTime, int32 Iterations)
{
	if (DeltaTime < MIN_TICK_TIME)
	{
		return;
	}

	RestorePreAdditiveRootMotionVelocity();
	FFindFloorResult FloorHit;
	if (!CanSlide(FloorHit))
	{
		ExitSlide();
		StartNewPhysics(DeltaTime, Iterations);
	}

	// strafe
	if (FMath::Abs(FVector::DotProduct(Acceleration.GetSafeNormal(), UpdatedComponent->GetRightVector())) > .5f)
	{
		Acceleration = Acceleration.ProjectOnTo(UpdatedComponent->GetRightVector());
	}
	else
	{
		Acceleration = FVector::ZeroVector;
	}

	// calc velocity
	if (!HasAnimRootMotion() && CurrentRootMotion.HasOverrideVelocity())
	{
		CalcVelocity(DeltaTime, SlideFriction, true, GetMaxBrakingDeceleration());
	}

	ApplyRootMotionToVelocity(DeltaTime);

	Iterations++;
	bJustTeleported = false;
	FVector OldLocation = UpdatedComponent->GetComponentLocation();
	FHitResult HitResult(1.f);
	FVector Adjusted = Velocity * DeltaTime;
	FVector VelPlaneDir = FVector::VectorPlaneProject(Velocity, FloorHit.HitResult.Normal).GetSafeNormal();
	FQuat NewRotation = FRotationMatrix::MakeFromXZ(VelPlaneDir, FloorHit.HitResult.Normal).ToQuat();

	// perform move
	MoveUpdatedComponent(Adjusted, NewRotation, true, &HitResult);
	if (HitResult.Time < 1.f)
	{
		HandleImpact(HitResult, DeltaTime, Adjusted);
		SlideAlongSurface(Adjusted, 1.f - HitResult.Time, HitResult.Normal, HitResult, true);
	}

	// check if sliding conditions are met
	if (FFindFloorResult NewFloorHit; !CanSlide(NewFloorHit))
	{
		ExitSlide();
	}

	// exit slide if maximum slide time is reached
	if (MaxSlidingSeconds > 0.f)
	{
		SlidingTime += DeltaTime;
		if (SlidingTime >= MaxSlidingSeconds)
		{
			ExitSlide();
		}
	}

	// update outgoing velocity && acceleration
	if (!bJustTeleported && !HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity())
	{
		Velocity = (UpdatedComponent->GetComponentLocation() - OldLocation) / DeltaTime;
	}
}
#pragma endregion 

#pragma region "Rope Walking"
bool UAriaCharacterMovement::CanRopeWalking(FHitResult& HitResult) const
{
	const FVector Start = UpdatedComponent->GetComponentLocation();
	const FVector End = Start + FVector::DownVector * (CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 5.f);
	const FCollisionQueryParams QueryParams = AriaCharacterOwner->GetQueryParams();

	// check if the floor is the rope actor
	GetWorld()->LineTraceSingleByProfile(HitResult, Start, End, "BlockAll", QueryParams);
	if (!HitResult.IsValidBlockingHit())
	{
		return false;
	}

	const TObjectPtr<AActor> Actor = HitResult.GetActor();
	if (!Actor)
	{
		return false;
	}

	return Actor->ActorHasTag(RopeTag);
}

void UAriaCharacterMovement::TryRopeWalking()
{
	if (FHitResult HitResult; !CanRopeWalking(HitResult))
	{
		return;
	}

	// all is good, go to rope walking
	SetMovementMode(MOVE_Custom, CMOVE_RopeWalk);
}

void UAriaCharacterMovement::PhysRopeWalking(const float DeltaTime, const int32 Iterations)
{
	if (FHitResult HitResult; !CanRopeWalking(HitResult))
	{
		SetMovementMode(DefaultLandMovementMode);
		StartNewPhysics(DeltaTime, Iterations);
	}

	PhysWalking(DeltaTime, Iterations);
}
#pragma endregion

#pragma region "Hard Landing"
void UAriaCharacterMovement::OnLanded(const FHitResult& HitResult)
{
	if (FMath::Abs(Velocity.Z) < MinHardFallingDistance)
	{
		return;
	}

	if (GetLastInputVector().IsZero() || FMath::IsNearlyZero(GetSpeed()))
	{
		DisableMovement();
		AriaCharacterOwner->PlayAnimMontage(HardLandingAnim);
	}
	else
	{
		bPrevCanJump = MovementState.bCanJump;
		MovementState.bCanJump = false;
		AriaCharacterOwner->PlayAnimMontage(FallingToRollAnim);
	}
}

void UAriaCharacterMovement::OnHardLandingAnimFinished()
{
	SetMovementMode(DefaultLandMovementMode);
}

void UAriaCharacterMovement::OnFallingToRollAnimFinished()
{
	MovementState.bCanJump = bPrevCanJump;
}
#pragma endregion

#pragma region "Pushing"
bool UAriaCharacterMovement::CanPushing() const
{
	// check if the character is on walkable floor
	FFindFloorResult FloorResult;
	FindFloor(UpdatedComponent->GetComponentLocation(), FloorResult, false);
	if (!FloorResult.bWalkableFloor)
	{
		return false;
	}

	// check if exist movable actor in front of the character
	FHitResult HitResult;
	FCollisionQueryParams QueryParams = AriaCharacterOwner->GetQueryParams();
	const FVector Start = UpdatedComponent->GetComponentLocation();
	const FVector End = Start + UpdatedComponent->GetForwardVector() * ForwardSearchPushingLength;
	if (!GetWorld()->LineTraceSingleByProfile(HitResult, Start, End, "BlockAll", QueryParams))
	{
		return false;
	}

	const TObjectPtr<AActor> Actor = HitResult.GetActor();
	if (!Actor)
	{
		return false;
	}
	
	return Actor->ActorHasTag(MovableTag);
}

void UAriaCharacterMovement::TryPushing()
{
	if (!CanPushing())
	{
		return;
	}

	// change capsule size to pushing dimensions
	const float OldUnscaleHalfHeight = CharacterOwner->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
	CharacterOwner->GetCapsuleComponent()->SetCapsuleSize(PushingCapsuleRadius, OldUnscaleHalfHeight);
	SetMovementMode(MOVE_Custom, CMOVE_Pushing);
}

void UAriaCharacterMovement::PhysPushing(const float DeltaTime, const int32 Iterations)
{
	if (!CanPushing())
	{
		// restore default capsule size
		const TObjectPtr<ACharacter> DefaultCharacter = CharacterOwner->GetClass()->GetDefaultObject<ACharacter>();
		const float DefaultUnscaleRadius = DefaultCharacter->GetCapsuleComponent()->GetUnscaledCapsuleRadius();
		const float DefaultUnscaleHalfHeight = DefaultCharacter->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
		CharacterOwner->GetCapsuleComponent()->SetCapsuleSize(DefaultUnscaleRadius, DefaultUnscaleHalfHeight);
		SetMovementMode(DefaultLandMovementMode);
	}

	PhysWalking(DeltaTime, Iterations);
}

#pragma endregion

#pragma region "Crawling"
bool UAriaCharacterMovement::CanCrawling(FFindFloorResult& FloorResult) const
{
	FindFloor(UpdatedComponent->GetComponentLocation(), FloorResult, false);
	return FloorResult.bWalkableFloor;
}

void UAriaCharacterMovement::EnterCrawling()
{
	SetCollisionSizeToSlidingDimensions();
	SetMovementMode(MOVE_Custom, CMOVE_Crawling);
}

void UAriaCharacterMovement::ExitCrawling()
{
	if (!RestoreDefaultCollisionDimensions())
	{
		return;
	}

	if (!bIsCrawlingAnimFinished)
	{
		bIsCrawlingAnimFinished = true;
		AriaCharacterOwner->StopAnimMontage();
	}
	
	// restore character rotation
	const FQuat NewRotation = FRotationMatrix::MakeFromXZ(UpdatedComponent->GetForwardVector().GetSafeNormal2D(), FVector::UpVector).ToQuat();
	MoveUpdatedComponent(FVector::ZeroVector, NewRotation, true);

	bWantsToCrawling = false;
	SetMovementMode(DefaultLandMovementMode);
}

void UAriaCharacterMovement::PhysCrawling(float DeltaTime, int32 Iterations)
{
	// check if crawling conditions are met
	if (FFindFloorResult FloorResult; !CanCrawling(FloorResult))
	{
		ExitCrawling();
	}

	// play a new anim montage if the player holds movement key
	if (bWantsToMove && bIsCrawlingAnimFinished)
	{
		bIsCrawlingAnimFinished = false;
		AriaCharacterOwner->PlayAnimMontage(CrawlingAnim);
	}

	// check if the player does not press Move and the animation is not finished
	if (!bWantsToMove && !bIsCrawlingAnimFinished)
	{
		// add movement input vector to complete the current anim montage
		AddInputVector(UpdatedComponent->GetForwardVector(), false);
	}
	
	PhysWalking(DeltaTime, Iterations);
}

void UAriaCharacterMovement::OnCrawlingAnimFinished()
{
	bIsCrawlingAnimFinished = true;
}
#pragma endregion 

#pragma region "Mantle"
void UAriaCharacterMovement::TryMantle()
{
	if (!IsFalling() && !IsWallSliding() && !IsClimbLadder())
	{
		return;
	}

	// helper variables
	float MinWallSteepnessAngleCos = FMath::Cos(FMath::DegreesToRadians(MantleMinWallSteepnessAngle));
	float MaxSurfaceAngleCos = FMath::Cos(FMath::DegreesToRadians(MantleMaxSurfaceAngle));
	float MaxAlignmentAngleCos = FMath::Cos(FMath::DegreesToRadians(MantleMaxAlignmentAngle));
	FVector ComponentLocation = UpdatedComponent->GetComponentLocation();
	FCollisionQueryParams QueryParams = AriaCharacterOwner->GetQueryParams();

	// check front face
	FHitResult FrontResult;
	FVector FrontStart = ComponentLocation + FVector::UpVector * MantleUpOffsetDistance;
	FVector ForwardVector = UpdatedComponent->GetForwardVector().GetSafeNormal2D();
	float CheckDistance = FMath::Clamp(Velocity | ForwardVector, GetCapsuleRadius() + 30.f, MaxFrontMantleCheckDistance);
	FVector FrontEnd = FrontStart + ForwardVector * CheckDistance;
	if (!GetWorld()->LineTraceSingleByProfile(FrontResult, FrontStart, FrontEnd, "BlockAll", QueryParams))
	{
		return;
	}

	if (!FrontResult.IsValidBlockingHit())
	{
		return;
	}

	float CosWallSteepnessAngle = FrontResult.Normal | FVector::UpVector;
	if (FMath::Abs(CosWallSteepnessAngle) > MinWallSteepnessAngleCos || (ForwardVector | -FrontResult.Normal) < MaxAlignmentAngleCos)
	{
		return;
	}
	
	// check heights
	TArray<FHitResult> HeightHits;
	FHitResult SurfaceHit;
	FVector WallUp = FVector::VectorPlaneProject(FVector::UpVector, FrontResult.Normal).GetSafeNormal();
	float WallCos = FVector::UpVector | FrontResult.Normal;
	float WallSin = FMath::Sqrt(1 - WallCos * WallCos);
	FVector TraceStart = FrontResult.Location + ForwardVector + WallUp * MantleReachHeight / WallSin;
	if (!GetWorld()->LineTraceMultiByProfile(HeightHits, TraceStart, FrontResult.Location + ForwardVector, "BlockAll", QueryParams))
	{
		return;
	}

	for (const FHitResult& Hit : HeightHits)
	{
		if (Hit.IsValidBlockingHit())
		{
			SurfaceHit = Hit;
			break;
		}
	}

	if (!SurfaceHit.IsValidBlockingHit() || (SurfaceHit.Normal | FVector::UpVector) < MaxSurfaceAngleCos)
	{
		return;
	}

	float Height = SurfaceHit.Location - FrontStart | FVector::UpVector;
	if (Height > MantleReachHeight)
	{
		return;
	}

	// check clearance
	float SurfaceCos = FVector::UpVector | SurfaceHit.Normal;
	float SurfaceSin = FMath::Sqrt(1 - SurfaceCos * SurfaceCos);
	FVector TransitionTarget = SurfaceHit.Location + ForwardVector * GetCapsuleRadius() + FVector::UpVector * (GetCapsuleHalfHeight() + GetCapsuleRadius() * 2 * SurfaceSin);
	FCollisionShape CapShape = FCollisionShape::MakeCapsule(GetCapsuleRadius(), GetCapsuleHalfHeight());
	if (GetWorld()->OverlapAnyTestByProfile(TransitionTarget, FQuat::Identity, "BlockAll", CapShape, QueryParams))
	{
		return;
	}

	// perform transition to mantle
	RootMotionSource.Reset();
	RootMotionSource = MakeShared<FRootMotionSource_MoveToForce>();
	RootMotionSource->AccumulateMode = ERootMotionAccumulateMode::Override;
	RootMotionSource->Duration = MantleAnim->GetPlayLength();
	RootMotionSource->StartLocation = ComponentLocation;
	RootMotionSource->TargetLocation = TransitionTarget;

	// apply transition root motion source
	Acceleration = FVector::ZeroVector;
	Velocity = FVector::ZeroVector;

	SetMovementMode(MOVE_Flying);
	RootMotionSourceID = ApplyRootMotionSource(RootMotionSource);

	// animation
	AriaCharacterOwner->PlayAnimMontage(MantleAnim);
}

void UAriaCharacterMovement::OnMantleAnimFinished()
{
	SetMovementMode(DefaultLandMovementMode);
}

#pragma endregion

#pragma region "Climb Ladder"
float UAriaCharacterMovement::GetClimbLadderDirection()
{
	// remember last climb direction to play Idle Up or Down animation
	const FVector Forward = UpdatedComponent->GetForwardVector();
	const float LastInputVector = GetLastInputVector().X * Forward.X;
	if (FMath::IsNearlyZero(LastClimbDirection) || (!FMath::IsNearlyZero(LastInputVector) && LastInputVector != LastClimbDirection))
	{
		LastClimbDirection = LastInputVector;
	}

	return LastClimbDirection;
}

bool UAriaCharacterMovement::CanClimbLadder(FHitResult& LadderHit) const
{
	if (MovementMode == MOVE_Flying)
	{
		return false;
	}
	
	const FVector Start = UpdatedComponent->GetComponentLocation();
	const FVector ForwardVector = UpdatedComponent->GetForwardVector();
	const FCollisionQueryParams QueryParams = AriaCharacterOwner->GetQueryParams();

	// exit climb ladder if the player pressed back button and the height to floor is smaller than MinHeightToClimbLadder
	const FVector InputVector = GetLastInputVector();
	if (!InputVector.IsNearlyZero() && !InputVector.Equals(ForwardVector))
	{
		const FVector DownVector = Start + FVector::DownVector * (GetCapsuleRadius() + MinHeightToClimbLadder);
		if (FHitResult FloorHit; GetWorld()->LineTraceSingleByProfile(FloorHit, Start, DownVector, "BlockAll", QueryParams))
		{
			return false;
		}
	}

	// check if the hit actor is ladder
	const FVector End = Start + ForwardVector * ForwardDistanceToCheckLadder;
	GetWorld()->LineTraceSingleByProfile(LadderHit, Start, End, "BlockAll", QueryParams);
	if (!LadderHit.IsValidBlockingHit())
	{
		return false;
	}

	TObjectPtr<AActor> LadderActor = LadderHit.GetActor();
	if (!LadderActor || !LadderActor->ActorHasTag(ClimbLadderTag))
	{
		return false;
	}

	return true;
}

void UAriaCharacterMovement::TryClimbLadder()
{
	FHitResult LadderHit;
	if (!CanClimbLadder(LadderHit))
	{
		return;;
	}

	// reset velocity if the character falls down to the ladder
	if (IsFalling())
	{
		Velocity = FVector::VectorPlaneProject(Velocity, LadderHit.Normal).GetSafeNormal2D();
	}

	SetMovementMode(MOVE_Custom, CMOVE_ClimbLadder);
}

void UAriaCharacterMovement::PhysClimbLadder(float DeltaTime, int32 Iterations)
{
	if (DeltaTime < MIN_TICK_TIME)
	{
		return;
	}

	if (CannotPerformPhysMovement())
	{
		Acceleration = FVector::ZeroVector;
		Velocity = FVector::ZeroVector;
		return;
	}

	bJustTeleported = false;
	float RemainingTime = DeltaTime;
	const FVector Forward = UpdatedComponent->GetForwardVector();
	while (CanPerformFrameTickMovement(RemainingTime, Iterations))
	{
		Iterations++;
		bJustTeleported = false;
		const float TimeTick = GetSimulationTimeStep(RemainingTime, Iterations);
		RemainingTime -= TimeTick;

		FHitResult LadderHit;
		if (!CanClimbLadder(LadderHit))
		{
			SetMovementMode(MOVE_Falling);
			StartNewPhysics(RemainingTime, Iterations);
			return;
		}

		// clamp acceleration
		Acceleration = FVector::VectorPlaneProject(Acceleration, LadderHit.Normal);

		// apply acceleration
		CalcVelocity(TimeTick, 0.f, false, GetMaxBrakingDeceleration());
		Velocity = FVector::VectorPlaneProject(Velocity, LadderHit.Normal);
		Velocity.Z = MaxClimbLadderSpeed * GetLastInputVector().X * Forward.X;

		// compute move parameters
		FHitResult HitResult;
		MoveUpdatedComponent(TimeTick * Velocity, UpdatedComponent->GetComponentQuat(), true, &HitResult);
	}

	// check again if the climb conditions are met
	if (FHitResult HitResult; !CanClimbLadder(HitResult))
	{
		SetMovementMode(MOVE_Falling);
	}
}

#pragma endregion

#pragma region "Dash"
void UAriaCharacterMovement::TryDash()
{
	if (!CanDash())
	{
		return;
	}

	bWantsToDash = false;
	PerformDash();
}

bool UAriaCharacterMovement::CanDash()
{
	if (!bWantsToDash)
	{
		return false;
	}

	if (bIsDashInProgress)
	{
		return false;
	}

	if (GetWorld()->GetTimeSeconds() - DashStartTime < DashCooldown)
	{
		return false;
	}

	return true;
}

void UAriaCharacterMovement::PerformDash()
{
	if (IsMovingOnGround())
	{
		bIsDashInProgress = true;
		SetMovementMode(MOVE_Flying);
		AriaCharacterOwner->PlayAnimMontage(GroundedDashAnim);
	}
	else
	{
		DashStartTime = GetWorld()->GetTimeSeconds();
		if (Velocity.Z < 0 || GetLastInputVector().IsNearlyZero())
		{
			Velocity.Z -= FallingDashImpulse;
		}
		else
		{
			Velocity += Velocity.GetSafeNormal2D() * ForwardDashImpulse;
		}
	}
}

void UAriaCharacterMovement::OnDashAnimFinished()
{
	bWantsToDash = false;
	bIsDashInProgress = false;
	SetMovementMode(DefaultLandMovementMode);
}
#pragma endregion

#pragma region "Ice Sliding"
void UAriaCharacterMovement::TryIceSliding()
{
	if (!CanIceSliding())
	{
		return;
	}

	if (IsIceSliding())
	{
		return;
	}

	UVariableStorage::Save("GroundFriction", GroundFriction);
	UVariableStorage::Save("BrakingFrictionFactor", BrakingFrictionFactor);
	UVariableStorage::Save("MaxAcceleration", MaxAcceleration);
	
	GroundFriction = IceSlideFriction;
	BrakingFrictionFactor = IceSlidingBrakingFrictionFactor;
	MaxAcceleration = MaxIceSlidingAcceleration;
	SetMovementMode(MOVE_Custom, CMOVE_IceSliding);
}

bool UAriaCharacterMovement::CanIceSliding() const
{
	FFindFloorResult FloorResult;
	FindFloor(UpdatedComponent->GetComponentLocation(), FloorResult, false);
	if (!FloorResult.bWalkableFloor)
	{
		return false;
	}

	const TObjectPtr<AActor> FloorActor = FloorResult.HitResult.GetActor();
	if (!FloorActor)
	{
		return false;
	}

	return FloorActor->ActorHasTag(IceTag);
}

void UAriaCharacterMovement::PhysIceSliding(float DeltaTime, int32 Iterations)
{
	if (!CanIceSliding())
	{
		// restore walking params
		GroundFriction = UVariableStorage::Get("GroundFriction", 8.f);
		BrakingFrictionFactor = UVariableStorage::Get("BrakingFrictionFactor", 2048.f);
		MaxAcceleration = UVariableStorage::Get("MaxAcceleration", 4096.f);
		SetMovementMode(MOVE_Walking);
	}

	PhysWalking(DeltaTime, Iterations);
}
#pragma endregion

#pragma region "Helpers"
float UAriaCharacterMovement::GetCapsuleRadius() const
{
	return CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleRadius();
}

float UAriaCharacterMovement::GetCapsuleHalfHeight() const
{
 return CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
}

bool UAriaCharacterMovement::IsCustomMovementMode(const ECustomMovementMode InCustomMovementMode) const
{
	return MovementMode == MOVE_Custom && CustomMovementMode == InCustomMovementMode;
}

bool UAriaCharacterMovement::CannotPerformPhysMovement() const
{
	return !CharacterOwner || (!CharacterOwner->Controller && !bRunPhysicsWithNoController && !HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity() && CharacterOwner->GetLocalRole() != ROLE_SimulatedProxy);
}

bool UAriaCharacterMovement::CanPerformFrameTickMovement(const float RemainingTime, const int32 Iterations) const
{
	return RemainingTime >= MIN_TICK_TIME && Iterations < MaxSimulationIterations && CharacterOwner && (CharacterOwner->Controller || bRunPhysicsWithNoController || HasAnimRootMotion() || CurrentRootMotion.HasOverrideVelocity() || CharacterOwner->GetLocalRole() == ROLE_SimulatedProxy);
}

void UAriaCharacterMovement::SetCollisionSizeToSlidingDimensions()
{
	// change collision size to sliding dimensions
	const float ComponentScale = CharacterOwner->GetCapsuleComponent()->GetShapeScale();
	const float OldUnscaledHalfHeight = CharacterOwner->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
	const float OldUnscaledRadius = CharacterOwner->GetCapsuleComponent()->GetUnscaledCapsuleRadius();

	// height is not allowed to be smaller than radius
	const float ClampedCrouchedHalfHeight = FMath::Max3(0.f, OldUnscaledRadius, SlideCollisionHalfHeight);
	float HalfHeightAdjust = OldUnscaledHalfHeight - ClampedCrouchedHalfHeight;
	CharacterOwner->GetCapsuleComponent()->SetCapsuleSize(OldUnscaledRadius, ClampedCrouchedHalfHeight);

	if (bCrouchMaintainsBaseLocation)
	{
		// intentionally not using MoveUpdatedComponent
		// where a horizontal plane constraint would prevent the base of the capsule from staying at the same spot
		UpdatedComponent->MoveComponent(FVector(0.f, 0.f, -(HalfHeightAdjust * ComponentScale)), UpdatedComponent->GetComponentQuat(), true, nullptr, MOVECOMP_NoFlags, ETeleportType::TeleportPhysics);
	}

	bForceNextFloorCheck = true;

	// OnStartCrouch takes the change from the Default size, not the current one (though they are usually the same)
	const ACharacter* DefaultCharacter = CharacterOwner->GetClass()->GetDefaultObject<ACharacter>();
	HalfHeightAdjust = DefaultCharacter->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight() - ClampedCrouchedHalfHeight;

	AdjustProxyCapsuleSize();
	CharacterOwner->OnStartCrouch(HalfHeightAdjust, HalfHeightAdjust * ComponentScale);
}

bool UAriaCharacterMovement::RestoreDefaultCollisionDimensions()
{
	TObjectPtr<ACharacter> DefaultCharacter = CharacterOwner->GetClass()->GetDefaultObject<ACharacter>();
	const float ComponentScale = CharacterOwner->GetCapsuleComponent()->GetShapeScale();
	const float OldUnscaledHalfHeight = CharacterOwner->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
	const float HalfHeightAdjust = DefaultCharacter->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight() - OldUnscaledHalfHeight;
	const float ScaledHalfHeightAdjust = HalfHeightAdjust * ComponentScale;
	const FVector PawnLocation = UpdatedComponent->GetComponentLocation();

	// try to stay in place and see if the larger capsule fits.
	// we use a slightly taller capsule to avoid penetration.
	const TObjectPtr<UWorld> MyWorld = GetWorld();
	constexpr float SweepInflation = UE_KINDA_SMALL_NUMBER * 10.f;
	FCollisionQueryParams CapsuleParams(SCENE_QUERY_STAT(CrouchTrace), false, CharacterOwner);
	FCollisionResponseParams ResponseParam;
	InitCollisionParams(CapsuleParams, ResponseParam);

	// compensate for the difference between current capsule size and standing size
	const FCollisionShape StandingCapsuleShape = GetPawnCapsuleCollisionShape(SHRINK_HeightCustom,-SweepInflation - ScaledHalfHeightAdjust);
	const ECollisionChannel CollisionChannel = UpdatedComponent->GetCollisionObjectType();
	bool bEncroached = true;

	if (!bCrouchMaintainsBaseLocation)
	{
		// expand in place
		bEncroached = MyWorld->OverlapBlockingTestByChannel(PawnLocation, FQuat::Identity, CollisionChannel, StandingCapsuleShape, CapsuleParams, ResponseParam);
		if (bEncroached)
		{
			// try adjusting capsule position to see if we can avoid encroachment.
			if (ScaledHalfHeightAdjust > 0.f)
			{
				// shrink to a short capsule, sweep down to base to find where that would hit something, and then try to stand up from there.
				float PawnRadius, PawnHalfHeight;
				CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleSize(PawnRadius, PawnHalfHeight);
				const float ShrinkHalfHeight = PawnHalfHeight - PawnRadius;
				const float TraceDist = PawnHalfHeight - ShrinkHalfHeight;
				const FVector Down = FVector(0.f, 0.f, -TraceDist);

				FHitResult Hit(1.f);
				const FCollisionShape ShortCapsuleShape = GetPawnCapsuleCollisionShape(SHRINK_HeightCustom, ShrinkHalfHeight);
				MyWorld->SweepSingleByChannel(Hit, PawnLocation, PawnLocation + Down, FQuat::Identity, CollisionChannel, ShortCapsuleShape, CapsuleParams);
				if (Hit.bStartPenetrating)
				{
					bEncroached = true;
				}
				else
				{
					// compute where the base of the sweep ended up, and see if we can stand there
					const float DistanceToBase = (Hit.Time * TraceDist) + ShortCapsuleShape.Capsule.HalfHeight;
					const FVector NewLoc = FVector(PawnLocation.X, PawnLocation.Y, PawnLocation.Z - DistanceToBase + StandingCapsuleShape.Capsule.HalfHeight + SweepInflation + MIN_FLOOR_DIST / 2.f);
					bEncroached = MyWorld->OverlapBlockingTestByChannel(NewLoc, FQuat::Identity, CollisionChannel, StandingCapsuleShape, CapsuleParams, ResponseParam);
					if (!bEncroached)
					{
						// intentionally not using MoveUpdatedComponent, where a horizontal plane constraint would prevent the base of the capsule from staying at the same spot.
						UpdatedComponent->MoveComponent(NewLoc - PawnLocation, UpdatedComponent->GetComponentQuat(), false, nullptr, MOVECOMP_NoFlags, ETeleportType::TeleportPhysics);
					}
				}
			}
		}
	}

	// if still encroached then abort.
	if (bEncroached)
	{
		return false;
	}

	// now call SetCapsuleSize() to cause touch/untouch events and actually grow the capsule
	CharacterOwner->GetCapsuleComponent()->SetCapsuleSize(DefaultCharacter->GetCapsuleComponent()->GetUnscaledCapsuleRadius(), DefaultCharacter->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight(), true);
	AdjustProxyCapsuleSize();
	CharacterOwner->OnEndCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);

	return true;
}

void UAriaCharacterMovement::InitAnimations()
{
	if (const auto HardLandingNotify = FindNotifyByClass<UHardLandingAnimNotify>(HardLandingAnim))
	{
		HardLandingNotify->OnNotified.AddUObject(this, &UAriaCharacterMovement::OnHardLandingAnimFinished);
	}

	if (const auto FallingToRollNotify = FindNotifyByClass<UFallingToRollAnimNotify>(FallingToRollAnim))
	{
		FallingToRollNotify->OnNotified.AddUObject(this, &UAriaCharacterMovement::OnFallingToRollAnimFinished);
	}

	if (const auto CrawlingNotify = FindNotifyByClass<UCrawlingAnimNotify>(CrawlingAnim))
	{
		CrawlingNotify->OnNotified.AddUObject(this, &UAriaCharacterMovement::OnCrawlingAnimFinished);
	}

	if (const auto MantleNotify = FindNotifyByClass<UMantleAnimNotify>(MantleAnim))
	{
		MantleNotify->OnNotified.AddUObject(this, &UAriaCharacterMovement::OnMantleAnimFinished);
	}

	if (const auto DashNotify = FindNotifyByClass<UDashAnimNotify>(GroundedDashAnim))
	{
		DashNotify->OnNotified.AddUObject(this, &UAriaCharacterMovement::OnDashAnimFinished);
	}
}
#pragma endregion
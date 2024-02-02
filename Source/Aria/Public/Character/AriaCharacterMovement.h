// Copyright (c) SPC Gaming. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStaticsTypes.h"
#include "AriaCharacterMovement.generated.h"

class AAriaCharacter;

DECLARE_LOG_CATEGORY_EXTERN(LogAriaCharacterMovement, Log, All);

UENUM(BlueprintType)
enum ECustomMovementMode
{
	CMOVE_None UMETA(DisplayName = "None"),
	CMOVE_WallSliding UMETA(DisplayName = "Wall Slide"),
	CMOVE_RopeWalk UMETA(DisplayName = "Rope Walk"),
	CMOVE_Slide UMETA(DisplayName = "Slide"),
	CMOVE_Pushing UMETA(DisplayName = "Pushing"),
	CMOVE_Crawling UMETA(DisplayName = "Crawling"),
	CMOVE_ClimbLadder UMETA(DisplayName = "Climb Ladder"),
	CMOVE_IceSliding UMETA(DisplayName = "Ice Sliding"),
	CMOVE_MAX UMETA(Hidden),
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ARIA_API UAriaCharacterMovement : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:
	UAriaCharacterMovement();

	// Blueprint Animation
	UFUNCTION(BlueprintPure) float GetSpeed() const;
	UFUNCTION(BlueprintPure) bool IsWalk() const;
	UFUNCTION(BlueprintPure) bool IsWallSliding() const { return IsCustomMovementMode(CMOVE_WallSliding); }
	UFUNCTION(BlueprintPure) bool IsSliding() const { return IsCustomMovementMode(CMOVE_Slide); }
	UFUNCTION(BlueprintPure) bool IsRopeWalking() const { return IsCustomMovementMode(CMOVE_RopeWalk); }
	UFUNCTION(BlueprintPure) bool IsPushing() const { return IsCustomMovementMode(CMOVE_Pushing); }
	UFUNCTION(BlueprintPure) bool IsCrawling() const { return IsCustomMovementMode(CMOVE_Crawling); }
	UFUNCTION(BlueprintPure) bool IsClimbLadder() const { return IsCustomMovementMode(CMOVE_ClimbLadder); }
	UFUNCTION(BlueprintPure) bool IsIceSliding() const { return IsCustomMovementMode(CMOVE_IceSliding); }
	UFUNCTION(BlueprintPure) float GetClimbLadderDirection();

	// Move
	bool bWantsToMove;
	
	// Wall Slide
	UPROPERTY(EditDefaultsOnly, Category="Wall Slide") float MinHeightToSlide = 200.f;
	UPROPERTY(EditDefaultsOnly, Category="Wall Slide") float WallJumpOffForce = 400.f;
	UPROPERTY(EditDefaultsOnly, Category="Wall Slide") float MaxVerticalWallSlideSpeed = 0.f;
	UPROPERTY(EditDefaultsOnly, Category="Wall Slide") TObjectPtr<UCurveFloat> WallSlideGravityCurve;

	// Slide
	bool bWantsToSlide;
	UPROPERTY(EditDefaultsOnly, Category="Slide") float MinSpeedToEnterSlide = 350.f;
	UPROPERTY(EditDefaultsOnly, Category="Slide") float EnterSlideImpulse = 500.f;
	UPROPERTY(EditDefaultsOnly, Category="Slide") float SlideFriction = 1.3f;
	UPROPERTY(EditDefaultsOnly, Category="Slide") float MaxSlidingSeconds = 1.f;
	UPROPERTY(EditDefaultsOnly, Category="Slide") float SlideCollisionHalfHeight = 40.f;

	// Rope Walking
	UPROPERTY(EditDefaultsOnly, Category="Rope Walking") float MaxRopeWalkingSpeed = 30.f;
	UPROPERTY(EditDefaultsOnly, Category="Rope Walking") FName RopeTag;

	// Hard Landing
	UPROPERTY(EditDefaultsOnly, Category="Hard Landing") float MinHardFallingDistance = 1700.f;
	UPROPERTY(EditDefaultsOnly, Category="Hard Landing") TObjectPtr<UAnimMontage> HardLandingAnim;
	UPROPERTY(EditDefaultsOnly, Category="Hard Landing") TObjectPtr<UAnimMontage> FallingToRollAnim;

	// Pushing
	UPROPERTY(EditDefaultsOnly, Category="Pushing") float MaxPushingSpeed = 50.f;
	UPROPERTY(EditDefaultsOnly, Category="Pushing") float ForwardSearchPushingLength = 85.f;
	UPROPERTY(EditDefaultsOnly, Category="Pushing") float PushingCapsuleRadius = 75.f;
	UPROPERTY(EditDefaultsOnly, Category="Pushing") FName MovableTag = "movable";

	// Crawling
	bool bWantsToCrawling;
	UPROPERTY(EditDefaultsOnly, Category="Crawling") float MaxCrawlingSpeed = 50.f;
	UPROPERTY(EditDefaultsOnly, Category="Crawling") TObjectPtr<UAnimMontage> CrawlingAnim;

	// Mantle
	UPROPERTY(EditDefaultsOnly, Category="Mantle") float MaxFrontMantleCheckDistance = 50.f;
	UPROPERTY(EditDefaultsOnly, Category="Mantle") float MantleUpOffsetDistance = 30.f;
	UPROPERTY(EditDefaultsOnly, Category="Mantle") float MantleReachHeight = 50.f;
	UPROPERTY(EditDefaultsOnly, Category="Mantle") float MantleMinWallSteepnessAngle = 75.f;
	UPROPERTY(EditDefaultsOnly, Category="Mantle") float MantleMaxSurfaceAngle = 40.f;
	UPROPERTY(EditDefaultsOnly, Category="Mantle") float MantleMaxAlignmentAngle = 55.f;
	UPROPERTY(EditDefaultsOnly, Category="Mantle") TObjectPtr<UAnimMontage> MantleAnim;

	// Climb Ladder
	UPROPERTY(EditDefaultsOnly, Category="Climb Ladder") float MinHeightToClimbLadder = 200.f;
	UPROPERTY(EditDefaultsOnly, Category="Climb Ladder") float ForwardDistanceToCheckLadder = 40.f;
	UPROPERTY(EditDefaultsOnly, Category="Climb Ladder") float MaxClimbLadderSpeed = 50.f;
	UPROPERTY(EditDefaultsOnly, Category="Climb Ladder") FName ClimbLadderTag;

	// Dash
	bool bWantsToDash;
	UPROPERTY(EditDefaultsOnly, Category="Dash") float DashCooldown = 1.f;
	UPROPERTY(EditDefaultsOnly, Category="Dash") float ForwardDashImpulse = 1000.f;
	UPROPERTY(EditDefaultsOnly, Category="Dash") float FallingDashImpulse = 1500.f;
	UPROPERTY(EditDefaultsOnly, Category="Dash") TObjectPtr<UAnimMontage> GroundedDashAnim;

	// Ice Sliding
	UPROPERTY(EditDefaultsOnly, Category="Ice Sliding") float IceSlideFriction = .1f;
	UPROPERTY(EditDefaultsOnly, Category="Ice Sliding") float IceSlidingBrakingFrictionFactor = 3.f;
	UPROPERTY(EditDefaultsOnly, Category="Ice Sliding") float MaxIceSlidingAcceleration = 150.f;
	UPROPERTY(EditDefaultsOnly, Category="Ice Sliding") float MaxIceSlidingSpeed = 400.f;
	UPROPERTY(EditDefaultsOnly, Category="Ice Sliding") FName IceTag;

	virtual void UpdateCharacterStateBeforeMovement(float DeltaSeconds) override;
	virtual void UpdateCharacterStateAfterMovement(float DeltaSeconds) override;
	virtual bool DoJump(bool bReplayingMoves) override;
	virtual bool CanAttemptJump() const override;
	virtual float GetMaxSpeed() const override;

protected:
	virtual void InitializeComponent() override;
	virtual void PhysCustom(float deltaTime, int32 Iterations) override;

private:
	UPROPERTY(Transient) TObjectPtr<AAriaCharacter> AriaCharacterOwner;

	// Wall Slide
	void TryWallSlide();
	void PhysWallSlide(float DeltaTime, int32 Iterations);
	FVector EndDownVector(const FVector& Start) const;
	FVector EndForwardVector(const FVector& Start) const;

	// Slide
	float SlidingTime = 0.f;
	void EnterSlide();
	void ExitSlide();
	void PhysSlide(float DeltaTime, int32 Iterations);
	bool CanSlide(FFindFloorResult& FloorHit) const;

	// Rope Walking
	void TryRopeWalking();
	bool CanRopeWalking(FHitResult& HitResult) const;
	void PhysRopeWalking(float DeltaTime, int32 Iterations);

	// Hard Landing
	bool bPrevCanJump;
	void OnHardLandingAnimFinished();
	void OnFallingToRollAnimFinished();
	UFUNCTION() void OnLanded(const FHitResult& HitResult);

	// Pushing
	void TryPushing();
	bool CanPushing() const;
	void PhysPushing(float DeltaTime, int32 Iterations);

	// Crawling
	bool bIsCrawlingAnimFinished = true;
	void EnterCrawling();
	void ExitCrawling();
	void OnCrawlingAnimFinished();
	bool CanCrawling(FFindFloorResult& FloorResult) const;
	void PhysCrawling(float DeltaTime, int32 Iterations);

	// Mantle
	int RootMotionSourceID;
	TSharedPtr<FRootMotionSource_MoveToForce> RootMotionSource;
	void TryMantle();
	void OnMantleAnimFinished();

	// Climb Ladder
	float LastClimbDirection = 0.f;
	void TryClimbLadder();
	bool CanClimbLadder(FHitResult& LadderHit) const;
	void PhysClimbLadder(float DeltaTime, int32 Iterations);

	// Dash
	bool bIsDashInProgress = false;
	float DashStartTime;
	void TryDash();
	bool CanDash();
	void PerformDash();
	void OnDashAnimFinished();

	// Ice Sliding
	void TryIceSliding();
	bool CanIceSliding() const;
	void PhysIceSliding(float DeltaTime, int32 Iterations);

	// Helpers
	bool IsCustomMovementMode(const ECustomMovementMode InCustomMovementMode) const;
	bool CannotPerformPhysMovement() const;
	bool CanPerformFrameTickMovement(const float RemainingTime, const int32 Iterations) const;
	float GetCapsuleRadius() const;
	float GetCapsuleHalfHeight() const;
	void SetCollisionSizeToSlidingDimensions();
	bool RestoreDefaultCollisionDimensions();
	void InitAnimations();
	template<typename AnimNotify>
	TObjectPtr<AnimNotify> FindNotifyByClass(const TObjectPtr<UAnimSequenceBase> Animation)
	{
		if (!Animation)
		{
			return nullptr;
		}

		for (FAnimNotifyEvent NotifyEvent : Animation->Notifies)
		{
			if (const auto AnimationNotify = Cast<AnimNotify>(NotifyEvent.Notify))
			{
				return AnimationNotify;
			}
		}

		return nullptr;
	}
};

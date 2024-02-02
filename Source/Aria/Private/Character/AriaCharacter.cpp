// Copyright (c) SPC Gaming. All rights reserved.

#include "Character/AriaCharacter.h"
#include "DrawDebugHelpers.h"
#include "Engine/LocalPlayer.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Camera/CameraComponent.h"
#include "Character/AriaCharacterMovement.h"
#include "Components/PostProcessComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/KismetMathLibrary.h"

DEFINE_LOG_CATEGORY(LogAriaCharacter);

AAriaCharacter::AAriaCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UAriaCharacterMovement>(CharacterMovementComponentName))
{
	GetCapsuleComponent()->InitCapsuleSize(20.f, 95.f);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	constexpr float TargetArmLength = 1000.f;
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>("CameraBoom");
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->SetUsingAbsoluteRotation(true);
	CameraBoom->SetWorldRotation(FRotator(0.f, 270.f, 0.f));
	CameraBoom->TargetArmLength = TargetArmLength;
	CameraBoom->bDoCollisionTest = false;
	CameraBoom->bUsePawnControlRotation = false;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>("FollowCamera");
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	PostProcessComponent = CreateDefaultSubobject<UPostProcessComponent>("PostProcessComponent");
	PostProcessComponent->SetupAttachment(RootComponent);
	FPostProcessSettings PostProcessSettings;
	PostProcessSettings.bOverride_DepthOfFieldFstop = true;
	PostProcessSettings.DepthOfFieldFstop = .2f;
	PostProcessSettings.bOverride_DepthOfFieldSensorWidth = true;
	PostProcessSettings.DepthOfFieldSensorWidth = 100.f;
	PostProcessSettings.bOverride_DepthOfFieldFocalDistance = true;
	PostProcessSettings.DepthOfFieldFocalDistance = TargetArmLength;
	PostProcessComponent->bEnabled = true;
	PostProcessComponent->Settings = PostProcessSettings;
}

void AAriaCharacter::BeginPlay()
{
	Super::BeginPlay();

	AriaCharacterMovement = Cast<UAriaCharacterMovement>(GetCharacterMovement());
	check(AriaCharacterMovement);
	
	if (const auto* PlayerController = Cast<APlayerController>(Controller))
	{
		if (const auto Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
}

void AAriaCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	if (auto* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AAriaCharacter::Move);
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Completed, this, &AAriaCharacter::StopMove);
		EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Completed, this, &AAriaCharacter::CrouchPressed);
		EnhancedInputComponent->BindAction(SlideAction, ETriggerEvent::Started, this, &AAriaCharacter::StartSliding);
		EnhancedInputComponent->BindAction(SlideAction, ETriggerEvent::Completed, this, &AAriaCharacter::StopSliding);
		EnhancedInputComponent->BindAction(CrawlingAction, ETriggerEvent::Completed, this, &AAriaCharacter::CrawlingPressed);
		EnhancedInputComponent->BindAction(DashAction, ETriggerEvent::Started, this, &AAriaCharacter::DashPressed);
	}
}

void AAriaCharacter::Move(const FInputActionValue& Value)
{
	if (!Controller)
	{
		return;
	}

	const FVector2D MovementVector = Value.Get<FVector2D>();
	const FRotator Rotation = Controller->GetControlRotation();
	const FRotator YawRotation(0.f, Rotation.Yaw, 0.f);
	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

	AriaCharacterMovement->bWantsToMove = true;
	AddMovementInput(RightDirection, MovementVector.X);
}

// ReSharper disable once CppMemberFunctionMayBeConst
void AAriaCharacter::StopMove()
{
	AriaCharacterMovement->bWantsToMove = false;
}

// ReSharper disable once CppMemberFunctionMayBeConst
void AAriaCharacter::CrouchPressed()
{
	AriaCharacterMovement->bWantsToCrouch = !AriaCharacterMovement->bWantsToCrouch;
}

// ReSharper disable once CppMemberFunctionMayBeConst
void AAriaCharacter::StartSliding()
{
	AriaCharacterMovement->bWantsToSlide = true;
}

// ReSharper disable once CppMemberFunctionMayBeConst
void AAriaCharacter::StopSliding()
{
	AriaCharacterMovement->bWantsToSlide = false;
}

// ReSharper disable once CppMemberFunctionMayBeConst
void AAriaCharacter::CrawlingPressed()
{
	AriaCharacterMovement->bWantsToCrawling = !AriaCharacterMovement->bWantsToCrawling;
}

// ReSharper disable once CppMemberFunctionMayBeConst
void AAriaCharacter::DashPressed()
{
	AriaCharacterMovement->bWantsToDash = true;
}

FCollisionQueryParams AAriaCharacter::GetQueryParams() const
{
	TArray<TObjectPtr<AActor>> CharacterChildren;
	GetAllChildActors(CharacterChildren);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActors(CharacterChildren);
	QueryParams.AddIgnoredActor(this);

	return QueryParams;
}

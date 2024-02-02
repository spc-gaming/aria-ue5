// Copyright (c) SPC Gaming. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Logging/LogMacros.h"
#include "AriaCharacter.generated.h"

class UAriaCharacterMovement;
class UPostProcessComponent;
class UCameraComponent;
class USpringArmComponent;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogAriaCharacter, Log, All);

UCLASS(config=Game)
class AAriaCharacter : public ACharacter
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, Category="Camera") TObjectPtr<USpringArmComponent> CameraBoom;
	UPROPERTY(VisibleAnywhere, Category="Camera") TObjectPtr<UCameraComponent> FollowCamera;
	UPROPERTY(VisibleAnywhere, Category="Camera") TObjectPtr<UPostProcessComponent> PostProcessComponent;
	UPROPERTY(EditAnywhere, Category="Input") TObjectPtr<UInputMappingContext> DefaultMappingContext;
	UPROPERTY(EditAnywhere, Category="Input") TObjectPtr<UInputAction> JumpAction;
	UPROPERTY(EditAnywhere, Category="Input") TObjectPtr<UInputAction> MoveAction;
	UPROPERTY(EditAnywhere, Category="Input") TObjectPtr<UInputAction> CrouchAction;
	UPROPERTY(EditAnywhere, Category="Input") TObjectPtr<UInputAction> SlideAction;
	UPROPERTY(EditAnywhere, Category="Input") TObjectPtr<UInputAction> CrawlingAction;
	UPROPERTY(EditAnywhere, Category="Input") TObjectPtr<UInputAction> DashAction;

public:
	explicit AAriaCharacter(const FObjectInitializer& ObjectInitializer);
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	FCollisionQueryParams GetQueryParams() const;

protected:
	void Move(const FInputActionValue& Value);
	void StopMove();
	void CrouchPressed();
	void StartSliding();
	void StopSliding();
	void CrawlingPressed();
	void DashPressed();
	virtual void BeginPlay() override;

private:
	UPROPERTY(Transient) TObjectPtr<UAriaCharacterMovement> AriaCharacterMovement;
};

// Copyright (c) SPC Gaming. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MovableActor.generated.h"

class UPhysicsConstraintComponent;

UCLASS()
class ARIA_API AMovableActor : public AActor
{
	GENERATED_BODY()

public:
	AMovableActor();

	UPROPERTY(EditAnywhere, Category="Component") TObjectPtr<UStaticMeshComponent> MeshComponent;
	UPROPERTY(EditAnywhere, Category="Component") TObjectPtr<UPhysicsConstraintComponent> PhysicsComponent;
};

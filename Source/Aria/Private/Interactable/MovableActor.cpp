// Copyright (c) SPC Gaming. All rights reserved.

#include "Interactable/MovableActor.h"
#include "Components/StaticMeshComponent.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"

AMovableActor::AMovableActor()
{
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>("MeshComponent");
	MeshComponent->SetSimulatePhysics(true);
	MeshComponent->SetLinearDamping(5.f);
	SetRootComponent(MeshComponent);

	PhysicsComponent = CreateDefaultSubobject<UPhysicsConstraintComponent>("PhysicsComponent");
	PhysicsComponent->SetLinearXLimit(LCM_Free, 0.f);
	PhysicsComponent->SetAngularSwing1Limit(ACM_Locked, 0.f);
	PhysicsComponent->SetAngularSwing2Limit(ACM_Locked, 0.f);
	PhysicsComponent->SetAngularTwistLimit(ACM_Locked, 0.f);
	FConstrainComponentPropName ComponentPropName;
	ComponentPropName.ComponentName = "MeshComponent";
	PhysicsComponent->ComponentName1 = ComponentPropName;
	PhysicsComponent->SetupAttachment(MeshComponent);

	Tags.Add("movable");
}

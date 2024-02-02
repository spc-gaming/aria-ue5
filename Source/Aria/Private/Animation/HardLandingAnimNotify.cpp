// Copyright (c) SPC Gaming. All rights reserved.

#include "Animation/HardLandingAnimNotify.h"

void UHardLandingAnimNotify::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	OnNotified.Broadcast();
	Super::Notify(MeshComp, Animation, EventReference);
}

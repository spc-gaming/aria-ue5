// Copyright (c) SPC Gaming. All rights reserved.

#include "Animation/MantleAnimNotify.h"

void UMantleAnimNotify::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	OnNotified.Broadcast();
	Super::Notify(MeshComp, Animation, EventReference);
}

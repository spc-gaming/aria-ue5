// Copyright (c) SPC Gaming. All rights reserved.

#include "Animation/FallingToRollAnimNotify.h"

void UFallingToRollAnimNotify::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	OnNotified.Broadcast();
	Super::Notify(MeshComp, Animation, EventReference);
}

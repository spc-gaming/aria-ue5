// Copyright (c) SPC Gaming. All rights reserved.

#include "Animation/CrawlingAnimNotify.h"

void UCrawlingAnimNotify::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	OnNotified.Broadcast();
	Super::Notify(MeshComp, Animation, EventReference);
}

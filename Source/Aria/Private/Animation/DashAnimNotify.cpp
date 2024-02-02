// Copyright (c) SPC Gaming. All rights reserved.

#include "Animation/DashAnimNotify.h"

void UDashAnimNotify::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	OnNotified.Broadcast();
	Super::Notify(MeshComp, Animation, EventReference);
}

// Copyright (c) SPC Gaming. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "MantleAnimNotify.generated.h"

DECLARE_MULTICAST_DELEGATE(FOnNotifiedSigature);

UCLASS()
class ARIA_API UMantleAnimNotify : public UAnimNotify
{
	GENERATED_BODY()

public:
	FOnNotifiedSigature OnNotified;
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
};

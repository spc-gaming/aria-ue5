// Copyright (c) SPC Gaming. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "CrawlingAnimNotify.generated.h"

DECLARE_MULTICAST_DELEGATE(FOnNotifiedSignature);

UCLASS()
class ARIA_API UCrawlingAnimNotify : public UAnimNotify
{
	GENERATED_BODY()

public:
	FOnNotifiedSignature OnNotified;
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
};

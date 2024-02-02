// Copyright (c) SPC Gaming. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "VariableStorage.generated.h"

UCLASS()
class ARIA_API UVariableStorage : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION() static void Save(const FString& VariableName, float VariableValue);
	UFUNCTION() static float Get(const FString& VariableName, const float DefaultValue);

private:
	static TMap<FString, float> FloatVariableMap;
};

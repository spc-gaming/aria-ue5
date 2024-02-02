// Copyright (c) SPC Gaming. All rights reserved.

#include "Utils/VariableStorage.h"

TMap<FString, float> UVariableStorage::FloatVariableMap;

void UVariableStorage::Save(const FString& VariableName, const float VariableValue)
{
	FloatVariableMap.Add(VariableName, VariableValue);
}

float UVariableStorage::Get(const FString& VariableName, const float DefaultValue)
{
	float VariableValue = DefaultValue;
	if (FloatVariableMap.Contains(VariableName))
	{
		FloatVariableMap.RemoveAndCopyValue(VariableName, VariableValue);
	}

	return VariableValue;
}

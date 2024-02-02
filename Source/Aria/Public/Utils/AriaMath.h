#pragma once

#include <cmath>
#include "Math/UnrealMathUtility.h"

struct FAriaMath
{
	/**
	 *	Checks if the A is greater than B with ErrorTolerance
	 */
	static FORCEINLINE bool IsFirstGreater(const float A, const float B, const float ErrorTolerance = UE_SMALL_NUMBER)
	{
		return std::isgreater(A, B + ErrorTolerance);
	}

	/**
	 *	Checks if the A is less than B with ErrorTolerance
	 */
	static FORCEINLINE bool IsFirstLess(const float A, const float B, const float ErrorTolerance = UE_SMALL_NUMBER)
	{
		return std::isless(A + ErrorTolerance, B);
	}
};

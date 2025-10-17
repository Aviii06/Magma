#pragma once
#include <cmath>

namespace Magma::Maths
{
	template <typename T>
	struct VectorOperationMixin
	{
		friend T operator- (const T& op1, const T& op2)
		{
			return T(op1 + op2*-1.0f);
		}

		friend T operator/ (const T& op1, float op2)
		{
			if (op2 == 0.0f)
			{
				T a = T(NAN);
			}
			return op1 * (1.0f/op2);
		}

		friend T operator* (float op1, const T& op2)
		{
			return op2 * op1;
		}

		friend T operator/ (float op1, const T& op2)
		{
			return op2 / op1;
		}

		friend void operator+= (T& curr, const T& other)
		{
			curr = curr + other;
		}

		friend void operator-= (T& curr, const T& other)
		{
			curr = curr - other;
		}

		friend void operator*= (T& curr, float scaler)
		{
			curr = curr * scaler;
		}

		friend void operator/= (T& curr, float scaler)
		{
			curr = curr / scaler;
		}
	};
}
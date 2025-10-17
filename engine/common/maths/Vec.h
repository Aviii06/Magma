#pragma once

#include <cmath>
#include <maths/VectorOperatorMixin.h>

namespace Magma::Maths
{
	template <typename T>
	struct Vec4 : VectorOperationMixin<Vec4<T>>
	{
		T x, y, z, w;

		Vec4()
		{
			Vec4(0.0);
		}

		Vec4(T x)
			: x(x)
			, y(x)
			, z(x)
			, w(x)
		{
		}

		friend Vec4 operator*(const Vec4& a, float scalar) { return Vec4(a.x * scalar, a.y * scalar, a.z * scalar, a.w * scalar); }
		friend Vec4 operator+(const Vec4& a, const Vec4& other) { return Vec4(a.x + other.x, a.y + other.y, a.z + other.z, a.w + other.w); }
	};

	template <typename T>
	struct Vec2 : VectorOperationMixin<Vec2<T>>
	{
		T x, y;

		Vec2(T x, T y) : x(x), y(y) {}

		friend Vec2 operator*(const Vec2& curr, float scalar) { return Vec2(curr.x * scalar, curr.y * scalar); }
		friend Vec2 operator+(const Vec2& curr , const Vec2& other) { return Vec2(curr.x + other.x, curr.y + other.y); }

		Vec2 Perpendicular() { return Vec2(-y, x); }

		Vec2 Normalize()
		{
			float length = sqrt(x * x + y * y);
			return Vec2(x / length, y / length);
		}
	};
}
#pragma once
#include "Vec.h"

namespace Revid::Maths
{
Vec3 Cross(Vec3 a, Vec3 b)
{
	return Vec3(
		a.y * b.z - a.z * b.y,
		a.z * b.x - a.x * b.z,
		a.x * b.y - a.y * b.x);
}

float Dot(Vec3 a, Vec3 b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

Vec3 Normalize(Vec3 a)
{
	float length = sqrt(a.x * a.x + a.y * a.y + a.z * a.z);
	return Vec3(a.x / length, a.y / length, a.z / length);
}
}

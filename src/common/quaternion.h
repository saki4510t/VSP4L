/*
 * aAndUsb
 * Copyright (c) 2014-2023 saki t_saki@serenegiant.com
 * Distributed under the terms of the GNU Lesser General Public License (LGPL v3.0) License.
 * License details are in the file license.txt, distributed as part of this software.
 */

#ifndef QUATERNION_H_
#define QUATERNION_H_

#include <math.h>
#include <memory>

#include "vec.h"

namespace serenegiant::math {

class Quaternion : public Vector4 {
public:
	Quaternion() {}

	Quaternion(const float &x, const float &y, const float &z, const float &w)
	: Vector4(x, y, z, w)
	{
	}

	// constructor with rotation
	Quaternion(const Vector3 &axis, const float &angle) {
		setRotation(axis, angle);
	}

	// constructor with setEulerZYX
	Quaternion(const Vector3 &angle) {
		setEulerZYX(angle.z(), angle.y(), angle.x());
	}
  /**
   * constructor with setEulerZYX
   * @param yaw Angle around Z
   * @param pitch Angle around Y
   * @param roll Angle around X */
	Quaternion(const float& yaw, const float& pitch, const float& roll) {
		setEulerZYX(yaw, pitch, roll);
	}

	void setRotation(const Vector3 &axis, const float &angle) {
		float d = axis.length();

		float s = sinf(angle * 0.5f) / d;
		set(axis.x() * s, axis.y() * s, axis.z() * s,
			cosf(angle * 0.5f));
	}
  /**
   * @param yaw Angle around Y
   * @param pitch Angle around X
   * @param roll Angle around Z */
	void setEuler(const float &yaw, const float& pitch, const float& roll) {
		float halfYaw = yaw * 0.5f;
		float halfPitch = pitch * 0.5f;
		float halfRoll = roll * 0.5f;
		float cosYaw = cosf(halfYaw);
		float sinYaw = sinf(halfYaw);
		float cosPitch = cosf(halfPitch);
		float sinPitch = sinf(halfPitch);
		float cosRoll = cosf(halfRoll);
		float sinRoll = sinf(halfRoll);
		set(cosRoll * sinPitch * cosYaw + sinRoll * cosPitch * sinYaw,
			cosRoll * cosPitch * sinYaw - sinRoll * sinPitch * cosYaw,
			sinRoll * cosPitch * cosYaw - cosRoll * sinPitch * sinYaw,
			cosRoll * cosPitch * cosYaw + sinRoll * sinPitch * sinYaw);
	}

  /**
   * @param yaw Angle around Z
   * @param pitch Angle around Y
   * @param roll Angle around X */
	void setEulerZYX(const float &yaw, const float &pitch, const float &roll) {
		float halfYaw = yaw * 0.5f;
		float halfPitch = pitch * 0.5f;
		float halfRoll = roll * 0.5f;
		float cosYaw = cosf(halfYaw);
		float sinYaw = sinf(halfYaw);
		float cosPitch = cosf(halfPitch);
		float sinPitch = sinf(halfPitch);
		float cosRoll = cosf(halfRoll);
		float sinRoll = sinf(halfRoll);
		set(sinRoll * cosPitch * cosYaw - cosRoll * sinPitch * sinYaw, //x
			cosRoll * sinPitch * cosYaw + sinRoll * cosPitch * sinYaw, //y
			cosRoll * cosPitch * sinYaw - sinRoll * sinPitch * cosYaw, //z
			cosRoll * cosPitch * cosYaw + sinRoll * sinPitch * sinYaw); //formerly yzx
	}

	float angle(const Quaternion &q) const {
		float s = sqrtf(length2() * q.length2());
		return acosf(dot(q) / s);
	}

	float getAngle() const {
		float s = 2.f * acosf(w());
		return s;
	}

	Vector3 getAxis() const {
		float s_squared = 1.f - w() * w();
		if (s_squared < 10.f * EPS)			 //Check for divide by zero
			return Vector3(1.f, 0.f, 0.f);  // Arbitrary
		float s = 1.f / sqrtf(s_squared);
		return Vector3(m[0] * s, m[1] * s, m[2] * s);
	}

	Quaternion inverse() const {
		return Quaternion(-m[0], -m[1], -m[2], m[3]);
	}

	Quaternion slerp(const Quaternion &q, const float &t) const {
		float magnitude = sqrtf(length2() * q.length2());

		float product = dot(q) / magnitude;
		if (fabsf(product) != 1.f) 	{
			// Take care of long angle case see http://en.wikipedia.org/wiki/Slerp
			const float sign = (product < 0) ? -1.f : 1.f;

			const float theta = acosf(sign * product);
			const float s1 = sinf(sign * t * theta);
			const float d = 1.f / sinf(theta);
			const float s0 = sinf((1.f - t) * theta);

			return Quaternion(
				(m[0] * s0 + q.x() * s1) * d,
				(m[1] * s0 + q.y() * s1) * d,
				(m[2] * s0 + q.z() * s1) * d,
				(m[3] * s0 + q.w() * s1) * d
			);
		} else {
			return *this;
		}
	}

	static const Quaternion &getIdentity() {
		static const Quaternion identityQuat(0.f, 0.f, 0.f, 1.f);
		return identityQuat;
	}

};

inline Quaternion operator*(const Quaternion &q1, const Quaternion &q2) {
	return Quaternion(
        q1.w() * q2.x() + q1.x() * q2.w() + q1.y() * q2.z() - q1.z() * q2.y(),
		q1.w() * q2.y() + q1.y() * q2.w() + q1.z() * q2.x() - q1.x() * q2.z(),
		q1.w() * q2.z() + q1.z() * q2.w() + q1.x() * q2.y() - q1.y() * q2.x(),
		q1.w() * q2.w() - q1.x() * q2.x() - q1.y() * q2.y() - q1.z() * q2.z()
	);
}

inline Quaternion operator*(const Quaternion &q, const Vector3 &w) {
	return Quaternion(
         q.w() * w.x() + q.y() * w.z() - q.z() * w.y(),
		 q.w() * w.y() + q.z() * w.x() - q.x() * w.z(),
		 q.w() * w.z() + q.x() * w.y() - q.y() * w.x(),
		-q.x() * w.x() - q.y() * w.y() - q.z() * w.z()
	);
}

inline Quaternion operator*(const Vector3 &w, const Quaternion &q) {
	return Quaternion(
        +w.x() * q.w() + w.y() * q.z() - w.z() * q.y(),
		+w.y() * q.w() + w.z() * q.x() - w.x() * q.z(),
		+w.z() * q.w() + w.x() * q.y() - w.y() * q.x(),
		-w.x() * q.x() - w.y() * q.y() - w.z() * q.z()
	);
}

inline float dot(const Quaternion &q1, const Quaternion &q2) {
	return q1.dot(q2);
}

inline float length(const Quaternion &q) {
	return q.length();
}

inline float angle(const Quaternion &q1, const Quaternion &q2) {
	return q1.angle(q2);
}

inline Quaternion inverse(const Quaternion &q) {
	return q.inverse();
}

inline Quaternion slerp(const Quaternion &q1, const Quaternion &q2, const float &t) {
	return q1.slerp(q2, t);
}

inline Vector3 quatRotate(const Quaternion &rotation, const Vector3 &v) {
	Quaternion q = rotation * v;
	q *= rotation.inverse();
	return Vector3(q.x(), q.y(),q. z());
}

// Game Programming Gems 2.10. make sure v0,v1 are normalized
inline Quaternion shortestArcQuat(const Vector3 &v0, const Vector3 &v1) {
	Vector3 c = v0.cross(v1);
	float  d = v0.dot(v1);

	if (d < -1.f + EPS) {
		Vector3 n, unused;
		PlaneSpace1(v0, n, unused);
		// just pick any vector that is orthogonal to v0
		return Quaternion(n.x(), n.y(), n.z(), 0.f);
	}

	float s = sqrtf((1.0f + d) * 2.0f);
	float rs = 1.0f / s;

	return Quaternion(c.x() * rs, c.y() * rs, c.z() * rs, s * 0.5f);
}

inline Quaternion shortestArcQuatNormalize2(Vector3 &v0, Vector3 &v1) {
	v0.normalize();
	v1.normalize();
	return shortestArcQuat(v0,v1);
}

typedef std::shared_ptr<Quaternion> QuaternionSp;
typedef std::unique_ptr<Quaternion> QuaternionUp;

}	// namespace serenegiant::math

#endif /* QUATERNION_H_ */

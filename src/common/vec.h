/*
 * aAndUsb
 * Copyright (c) 2014-2023 saki t_saki@serenegiant.com
 * Distributed under the terms of the GNU Lesser General Public License (LGPL v3.0) License.
 * License details are in the file license.txt, distributed as part of this software.
 */

#ifndef VECTOR_HPP_
#define VECTOR_HPP_

#include <math.h>
#include <memory>

namespace serenegiant::math {

#define EPS 1e-6
#define SQRT12 0.7071067811865475244008443621048490f

#define RecipSqrt(x) ((1.f / sqrtf(float(x))))		/* reciprocal square root */

class Vector3;
class Vector4;

//********************************************************************************
//********************************************************************************

inline Vector3 operator+(const Vector3 &v1, const Vector3 &v2);
inline Vector3 operator-(const Vector3 &v1, const Vector3 &v2);
inline Vector3 operator-(const Vector3 &v1);
inline Vector3 operator*(const Vector3 &v1, const Vector3 &v2);
inline Vector3 operator*(const Vector3 &v1, const float &s);
inline Vector3 operator*(const float &s, const Vector3 &v1);
inline Vector3 operator*(const Vector3 &v1, const Vector4 &v2);
inline Vector3 operator/(const Vector3 &v1, const Vector3 &v2);
inline Vector3 operator/(const Vector3 &v, const float &s);

inline Vector4 operator+(const Vector4 &v1, const Vector4 &v2);
inline Vector4 operator-(const Vector4 &v1, const Vector4 &v2);
inline Vector4 operator-(const Vector4 &v1);
inline Vector4 operator*(const Vector4 &v1, const Vector3 &v2);
inline Vector4 operator*(const Vector4 &v1, const Vector4 &v2);

//********************************************************************************
//********************************************************************************
class Vector3 {
protected:
	float m[4];
	inline Vector3 &set(const float &x, const float &y, const float &z, const float &w) {
		m[0] = x;
		m[1] = y;
		m[2] = z;
		m[3] = w;
		return *this;
	}

	inline Vector3(const float &x, const float &y, const float &z, const float &w) {
		set(x, y, z, w);
	}

public:
	inline Vector3() {
		set(0.f, 0.f, 0.f, 0.f);
	};

	inline Vector3(const float *mat) {
		set(mat[0],  mat[1], mat[2], 0.f);
	}

	inline Vector3(const float &x, const float &y, const float &z) {
		set(x, y, z, 0.f);
	}

	inline Vector3(Vector4 &v);

	inline Vector3 &operator+=(const Vector3 &v) {
		m[0] += v.m[0];
		m[1] += v.m[1];
		m[2] += v.m[2];
		return *this;
	}

	inline Vector3 &operator-=(const Vector3 &v) {
		m[0] -= v.m[0];
		m[1] -= v.m[1];
		m[2] -= v.m[2];
		return *this;
	}

	inline Vector3 &operator*=(const float &s) {
		m[0] *= s;
		m[1] *= s;
		m[2] *= s;
		return *this;
	}

	inline Vector3 &operator*=(const Vector3 &v) {
		m[0] *= v.m[0];
		m[1] *= v.m[1];
		m[2] *= v.m[2];
		return *this;
	}

	inline Vector3 &operator*=(const Vector4 &v);

	inline Vector3 &operator/=(const float &s) {
		m[0] /= s;
		m[1] /= s;
		m[2] /= s;
		return *this;
	}

	inline operator float *() {
		return &m[0];
	}

	inline operator const float *() const {
		return &m[0];
	}

	inline bool operator==(const Vector3 &other) {
		return ( (m[2] == other.m[2]) &&
	             (m[1] == other.m[1]) &&
	             (m[0] == other.m[0]) );
	}

	inline bool operator==(const Vector4 &other);

	inline bool operator!=(const Vector3 &other) const {
		return !(*this == other);
	}

	inline bool operator!=(const Vector4 &other) const;

	inline float dot(const Vector3 &v) const {
		return	m[0] * v.m[0] +
				m[1] * v.m[1] +
				m[2] * v.m[2];
	}

	inline float length2() const {
		return dot(*this);
	}

	inline float length() const {
		return sqrtf(length2());
	}

	inline float distance2(const Vector3 &v) const {
		return (v - *this).length2();
	}

	inline float distance(const Vector3 &v) const {
		return (v - *this).length();
	}

	inline Vector3 &safeNormalize() {
		Vector3 absVec = this->absolute();
		int maxIndex = absVec.maxAxis();
		if (absVec[maxIndex] > 0) {
			*this /= absVec[maxIndex];
			return *this /= length();
		}
		set(1, 0, 0);
		return *this;
	}

	inline Vector3 &normalize() {
		return *this /= length();
	}

	inline Vector3 normalized() const {
		return *this / length();
	}

	inline float angle(const Vector3 &v) const {
		float s = sqrtf(length2() * v.length2());
		return cosf(dot(v) / s);
	}

	inline Vector3 absolute() const {
		return Vector3(
			fabsf(m[0]),
			fabsf(m[1]),
			fabsf(m[2])
		);
	}

	inline Vector3 cross(const Vector3 &v) const {
		return Vector3(
			m[1] * v.m[2] - m[2] * v.m[1],
			m[2] * v.m[0] - m[0] * v.m[2],
			m[0] * v.m[1] - m[1] * v.m[0]
		);
	}

	inline float triple(const Vector3 &v1, const Vector3 &v2) const {
		return
			m[0] * (v1.m[1] * v2.m[2] - v1.m[2] * v2.m[1]) +
			m[1] * (v1.m[2] * v2.m[0] - v1.m[0] * v2.m[2]) +
			m[2] * (v1.m[0] * v2.m[1] - v1.m[1] * v2.m[0]
		);
	}

	inline int minAxis() const {
		return m[0] < m[1] ? (m[0] < m[2] ? 0 : 2) : (m[1] < m[2] ? 1 : 2);
	}

	inline int maxAxis() const {
		return m[0] < m[1] ? (m[1] < m[2] ? 2 : 1) : (m[0] < m[2] ? 2 : 0);
	}

	inline int furthestAxis() const {
		return absolute().minAxis();
	}

	inline int closestAxis() const {
		return absolute().maxAxis();
	}

	inline Vector3 lerp(const Vector3 &v, const float &t) const {
		return Vector3(
			m[0] + (v.m[0] - m[0]) * t,
			m[1] + (v.m[1] - m[1]) * t,
			m[2] + (v.m[2] - m[2]) * t
		);
	}

	inline Vector3 min(const Vector3 &other) {
		return Vector3(
			fminf(m[0], other.m[0]),
			fminf(m[1], other.m[1]),
			fminf(m[2], other.m[2])
		);
	}

	inline Vector3 max(const Vector3 &other) {
		return Vector3(
			fmaxf(m[0], other.m[0]),
			fmaxf(m[1], other.m[1]),
			fmaxf(m[2], other.m[2]) );
	}

	inline void setMin(const Vector3 &other) {
		m[0] = fminf(m[0], other.m[0]);
		m[1] = fminf(m[1], other.m[1]);
		m[2] = fminf(m[2], other.m[2]);
		m[3] = fminf(m[3], other.m[3]);
	}

	inline void setMax(const Vector3 &other) {
		m[0] = fmaxf(m[0], other.m[0]);
		m[1] = fmaxf(m[1], other.m[1]);
		m[2] = fmaxf(m[2], other.m[2]);
		m[3] = fmaxf(m[3], other.m[3]);
	}


	inline Vector3 rotate(const Vector3 &wAxis, const float angle) {
		Vector3 o = wAxis * wAxis.dot(*this);
		Vector3 _x = *this - o;
		Vector3 _y;

		_y = wAxis.cross(*this);

		return ( o + _x * cosf(angle) + _y * sinf(angle) );
	}

	inline Vector3 &set(const float &x, const float &y) {
		set(x, y, 0.f);
		return *this;
	}

	inline Vector3 &set(const float &x, const float &y, const float &z) {
		m[0] = x;
		m[1] = y;
		m[2] = z;
		m[3] = 0.f;
		return *this;
	}

	inline const float &getX() const { return m[0]; }
	inline const float &getY() const { return m[1]; }
	inline const float &getZ() const { return m[2]; }
	inline void	setX(float const &x) { m[0] = x;};
	inline void	setY(float const &y) { m[1] = y;};
	inline void	setZ(float const &z) { m[2] = z;};
	inline const float &x() const { return m[0]; }
	inline const float &y() const { return m[1]; }
	inline const float &z() const { return m[2]; }

};

//********************************************************************************
//********************************************************************************
class Vector4 : public Vector3 {
public:
	inline Vector4() {
	};

	inline Vector4(const float *mat) {
		set(mat[0], mat[1], mat[2], mat[3]);
	}

	inline Vector4(const float &x, const float &y, const float &z, const float &w)
		: Vector3(x, y, z) {
		m[3] = w;
	}

	inline Vector4(const Vector3 &v) {
		set(v[0], v[1], v[2], v[3]);
	}

	inline Vector4(const Vector3 &v, const float &w) {
		set(v[0], v[1], v[2], w);
	}

	inline Vector4 absolute() const {
		return Vector4(
			fabsf(m[0]),
			fabsf(m[1]),
			fabsf(m[2]),
			fabsf(m[3])
		);
	}

	inline Vector4 &set(const float &x, const float &y, const float &z, const float &w) {
		m[0] = x;
		m[1] = y;
		m[2] = z;
		m[3] = w;
		return *this;
	}

	inline Vector4 min(const Vector4 &other) {
		return Vector4(
			fminf(m[0], other.m[0]),
			fminf(m[1], other.m[1]),
			fminf(m[2], other.m[2]),
			fminf(m[3], other.m[3])
		);
	}

	inline Vector4 max(const Vector4 &other) {
		return Vector4(
			fmaxf(m[0], other.m[0]),
			fmaxf(m[1], other.m[1]),
			fmaxf(m[2], other.m[2]),
			fmaxf(m[3], other.m[3])
		);
	}

	inline const float &w() const { return m[3]; }
	inline void	setW(const float &w) { m[3] = w;};
	inline const float &getW() const {
		return m[3];
	}

	inline Vector4 &operator+=(const Vector4 &other) {
		m[0] += other.x();
        m[1] += other.y();
        m[2] += other.z();
        m[3] += other.w();
		return *this;
	}

	inline Vector4 &operator-=(const Vector4 &other) {
		m[0] -= other.x();
        m[1] -= other.y();
        m[2] -= other.z();
        m[3] -= other.w();
        return *this;
	}

	inline Vector4 &operator*=(const float &s) {
		m[0] *= s;
        m[1] *= s;
        m[2] *= s;
        m[3] *= s;
		return *this;
	}

	inline Vector4 &operator*=(const Vector4 &other) {
		set(
            m[3] * other.x() + m[0] * other.w() + m[1] * other.z() - m[2] * other.y(),
			m[3] * other.y() + m[1] * other.w() + m[2] * other.x() - m[0] * other.z(),
			m[3] * other.z() + m[2] * other.w() + m[0] * other.y() - m[1] * other.x(),
			m[3] * other.w() - m[0] * other.x() - m[1] * other.y() - m[2] * other.z()
		);
		return *this;
	}

	inline bool operator==(const Vector4 &other) {
		return ( (m[3] == other.m[3]) &&
	             (m[2] == other.m[2]) &&
	             (m[1] == other.m[1]) &&
	             (m[0] == other.m[0]) );
	}

	inline bool operator==(const Vector3 &other) {
		return ( (m[3] == other[3]) &&
	             (m[2] == other[2]) &&
	             (m[1] == other[1]) &&
	             (m[0] == other[0]) );
	}

	inline float dot(const Vector3 &other) const {
		return  m[0] * other[0] +
                m[1] * other[1] +
                m[2] * other[2] +
                m[3] * other[3];
	}

	inline bool operator!=(const Vector4 &other) const {
		return !(*this == other);
	}

	inline bool operator!=(const Vector3 &other) const {
		return !(*this == other);
	}

	inline float length2() const {
		return dot(*this);
	}

	inline float length() const {
		return sqrtf(length2());
	}

	inline Vector4 &normalize() {
		return *this /= length();
	}

	inline Vector4 operator*(const float &s) const {
		return Vector4(x() * s, y() * s, z() * s, w() * s);
	}

	inline Vector4 operator/(const float &s) const {
		return *this * (1.f / s);
	}

	inline Vector4 &operator/=(const float &s) {
		return *this *= 1.0f / s;
	}

	inline Vector4 normalized() const {
		return *this / length();
	}

	inline Vector4 farthest(const Vector4 &qd) const {
		Vector4 diff = *this - qd;
		Vector4 sum = *this + qd;
		if( diff.dot(diff) > sum.dot(sum) )
			return qd;
		return (-qd);
	}

	inline Vector4 nearest(const Vector4 &qd) const {
		Vector4 diff = *this - qd;
		Vector4 sum = *this + qd;
		if( diff.dot(diff) < sum.dot(sum) )
			return qd;
		return (-qd);
	}

};

//********************************************************************************
//********************************************************************************
inline Vector3::Vector3(Vector4 &v) {
	set(v[0], v[1], v[2], v[3]);
}

inline Vector3 &Vector3::Vector3::operator*=(const Vector4 &v) {
	m[0] *= v[0];
	m[1] *= v[1];
	m[2] *= v[2];
	return *this;
}

inline bool Vector3::operator==(const Vector4 &other) {
	return ( (m[3] == other.m[3]) &&
             (m[2] == other.m[2]) &&
             (m[1] == other.m[1]) &&
             (m[0] == other.m[0]) );
}

inline bool Vector3::operator!=(const Vector4 &other) const {
	return !(*this == other);
}

//********************************************************************************
// グローバル関数
//********************************************************************************
inline Vector3 operator+(const Vector3 &v1, const Vector3 &v2) {
	return Vector3(
		v1[0] + v2[0],
		v1[1] + v2[1],
		v1[2] + v2[2]
	);
}

inline Vector3 operator-(const Vector3 &v1, const Vector3 &v2) {
	return Vector3(
		v1[0] - v2[0],
		v1[1] - v2[1],
		v1[2] - v2[2]
	);
}

inline Vector3 operator-(const Vector3 &v1) {
	return Vector3(-v1[0], -v1[1], -v1[2]);
}

inline Vector3 operator*(const Vector3 &v1, const Vector3 &v2) {
	return Vector3(
		v1[0] * v2[0],
		v1[1] * v2[1],
		v1[2] * v2[2]
	);
}

inline Vector3 operator*(const Vector3 &v1, const float &s) {
	return Vector3(
		v1[0] * s,
		v1[1] * s,
		v1[2] * s
	);
}

inline Vector3 operator*(const float &s, const Vector3 &v1) {
	return Vector3(
		v1[0] * s,
		v1[1] * s,
		v1[2] * s
	);
}

inline Vector3 operator*(const Vector3 &v1, const Vector4 &v2) {
	return Vector3(
		v1[0] * v2[0],
		v1[1] * v2[1],
		v1[2] * v2[2]
	);
}

inline Vector3 operator/(const Vector3 &v1, const Vector3 &v2) {
	return Vector3(
		v1[0] / v2[0],
		v1[1] / v2[1],
		v1[2] / v2[2]
	);
}

inline Vector3 operator/(const Vector3 &v1, const float &s) {
	return v1 * (1.f / s);
}

inline void PlaneSpace1 (const Vector3 &n, Vector3 &p, Vector3 &q) {
	if (fabsf(n[2]) > SQRT12) {
	// choose p in y-z plane
		float a = n[1] * n[1] + n[2] * n[2];
		float k = RecipSqrt(a);
		p[0] = 0;
		p[1] = -n[2] * k;
		p[2] = n[1] * k;
		// set q = n x p
		q[0] = a * k;
		q[1] = -n[0] * p[2];
		q[2] = n[0] * p[1];
	} else {
		// choose p in x-y plane
		float a = n[0] * n[0] + n[1] * n[1];
		float k = RecipSqrt(a);
		p[0] = -n[1] * k;
		p[1] = n[0] * k;
		p[2] = 0;
		// set q = n x p
		q[0] = -n[2] * p[1];
		q[1] = n[2] * p[0];
		q[2] = a * k;
	}
}

inline Vector4 operator+(const Vector4 &v1, const Vector4 &v2) {
	return Vector4(
		v1[0] + v2[0],
		v1[1] + v2[1],
		v1[2] + v2[2],
		v1[3] + v2[3]
	);
}

inline Vector4 operator-(const Vector4 &v1, const Vector4 &v2) {
	return Vector4(
		v1[0] - v2[0],
		v1[1] - v2[1],
		v1[2] - v2[2],
		v1[3] - v2[3]
	);
}

inline Vector4 operator-(const Vector4 &v1) {
	return Vector4(
		-v1[0],
		-v1[1],
		-v1[2],
		-v1[3]
	);
}

inline Vector4 operator*(const Vector4 &v1, const Vector3 &v2) {
	return Vector4(
		v1[0] * v2[0],
		v1[1] * v2[1],
		v1[2] * v2[2],
		v1[3] * v2[3]
	);
}

inline Vector4 operator*(const Vector4 &v1, const Vector4 &v2) {
	return Vector4(
		v1[0] * v2[0],
		v1[1] * v2[1],
		v1[2] * v2[2],
		v1[3] * v2[3]
	);
}

typedef std::shared_ptr<Vector3> Vector3Sp;
typedef std::unique_ptr<Vector3> Vector3Up;
typedef std::shared_ptr<Vector4> Vector4Sp;
typedef std::unique_ptr<Vector4> Vector4Up;

}	// namespace serenegiant::math

#endif /* VECTOR_HPP_ */

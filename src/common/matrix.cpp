/*
 * aAndUsb
 * Copyright (c) 2014-2023 saki t_saki@serenegiant.com
 * Distributed under the terms of the GNU Lesser General Public License (LGPL v3.0) License.
 * License details are in the file license.txt, distributed as part of this software.
 */

#if 1	// set 1 if you don't need debug message
	#ifndef LOG_NDEBUG
		#define	LOG_NDEBUG		// ignore LOGV/LOGD/MARK
	#endif
	#undef USE_LOGALL
#else
//	#define USE_LOGALL
	#define USE_LOGD
	#undef LOG_NDEBUG
	#undef NDEBUG		// depends on definition in Android.mk and Application.mk
#endif

#include <cmath>

#include "matrix.h"

namespace serenegiant::math {

//********************************************************************************
//********************************************************************************
Matrix::Matrix(const Matrix &src) {
	m_vec[0] = src.m_vec[0];
	m_vec[1] = src.m_vec[1];
	m_vec[2] = src.m_vec[2];
	m_vec[3] = src.m_vec[3];
}

Matrix::Matrix(const float *m) {
	m_vec[0].set(m[0], m[1], m[2], m[3]);
	m_vec[1].set(m[4], m[5], m[6], m[7]);
	m_vec[2].set(m[8], m[9], m[10], m[11]);
	m_vec[3].set(m[12], m[13], m[14], m[15]);
}

Matrix::Matrix(const float &xx, const float &xy, const float &xz,
		const float &yx, const float &yy, const float &yz,
		const float &zx, const float &zy, const float &zz) {

	set(
		xx, xy, xz,
		yx, yy, yz,
		zx, zy, zz
	);
}

Matrix::Matrix(
	const float &xx, const float &xy, const float &xz, const float &xw,
	const float &yx, const float &yy, const float &yz, const float &yw,
	const float &zx, const float &zy, const float &zz, const float &zw,
	const float &wx, const float &wy, const float &wz, const float &ww) {

	set(
		xx, xy, xz, xw,
		yx, yy, yz, yw,
		zx, zy, zz, zw,
		wx, wy, wz, ww
	);
}

// デフォルトの代入演算子でOK
/*Matrix &Matrix::operator=(const Matrix &other) {
	m_vec[0] = other.m_vec[0];
	m_vec[1] = other.m_vec[1];
	m_vec[2] = other.m_vec[2];
	m_vec[3] = other.m_vec[3];
	return *this;
} */

Vector4 Matrix::getColumn(int i) const {
	return Vector4(m_vec[0][i], m_vec[1][i], m_vec[2][i], m_vec[3][i]);
}

const Vector4 &Matrix::getRow(int i) const {
	return m_vec[i];
}

Vector4 &Matrix::operator[](int i) {
	return m_vec[i];
}

const Vector4 &Matrix::operator[](int i) const {
	return m_vec[i];
}

Matrix &Matrix::operator*=(const Matrix &m) {
//	return (*this = *this * other);
/*	set(
        m.tdotx(m_vec[0]), m.tdoty(m_vec[0]), m.tdotz(m_vec[0]), m.tdotw(m_vec[0]),
		m.tdotx(m_vec[1]), m.tdoty(m_vec[1]), m.tdotz(m_vec[1]), m.tdotw(m_vec[1]),
		m.tdotx(m_vec[2]), m.tdoty(m_vec[2]), m.tdotz(m_vec[2]), m.tdotw(m_vec[2]),
		m.tdotx(m_vec[3]), m.tdoty(m_vec[3]), m.tdotz(m_vec[3]), m.tdotw(m_vec[3])
	); */
	// 上の式だと思っていたのと逆順に掛けられてしまうので、かける順を逆に変更
	set(
        tdotx(m[0]), tdoty(m[0]), tdotz(m[0]), tdotw(m[0]),
		tdotx(m[1]), tdoty(m[1]), tdotz(m[1]), tdotw(m[1]),
		tdotx(m[2]), tdoty(m[2]), tdotz(m[2]), tdotw(m[2]),
		tdotx(m[3]), tdoty(m[3]), tdotz(m[3]), tdotw(m[3])
	);
	return *this;
}

Matrix &Matrix::operator+=(const Matrix &m) {
//	return (*this = *this + other);
	m_vec[0] += m.m_vec[0];
	m_vec[1] += m.m_vec[1];
	m_vec[2] += m.m_vec[2];
	m_vec[3] += m.m_vec[3];
	return *this;
}

Matrix &Matrix::operator-=(const Matrix &m) {
//	return (*this = *this - other);
	m_vec[0] -= m.m_vec[0];
	m_vec[1] -= m.m_vec[1];
	m_vec[2] -= m.m_vec[2];
	m_vec[3] -= m.m_vec[3];
	return *this;
}

bool Matrix::operator==(const Matrix &m) {
	return
		(m_vec[0] == m[0]) &&
		(m_vec[1] == m[1]) &&
		(m_vec[2] == m[2]) &&
		(m_vec[3] == m[3]);
}
bool Matrix::operator==(const Matrix &m) const {
	return
		(m_vec[0] == m[0]) &&
		(m_vec[1] == m[1]) &&
		(m_vec[2] == m[2]) &&
		(m_vec[3] == m[3]);
}

Matrix &Matrix::setFromOpenGLSubMatrix(const float *m) {
	m_vec[0].set(m[0], m[4], m[8], m[12]);
	m_vec[1].set(m[1], m[5], m[9], m[13]);
	m_vec[2].set(m[2], m[6], m[10], m[14]);
	m_vec[3].set(m[3], m[7], m[11], m[15]);
	return *this;
}

Matrix &Matrix::set(
	const float &xx, const float &xy, const float &xz,
	const float &yx, const float &yy, const float &yz,
	const float &zx, const float &zy, const float &zz) {

	m_vec[0].set(xx, xy, xz, 0.f);
	m_vec[1].set(yx, yy, yz, 0.f);
	m_vec[2].set(zx, zy, zz, 0.f);
	m_vec[3].set(0.f, 0.f, 0.f, 1.f);
	return *this;
}

Matrix &Matrix::set(
	const float &xx, const float &xy, const float &xz, const float &xw,
	const float &yx, const float &yy, const float &yz, const float &yw,
	const float &zx, const float &zy, const float &zz, const float &zw,
	const float &wx, const float &wy, const float &wz, const float &ww) {

	m_vec[0].set(xx, xy, xz, xw);
	m_vec[1].set(yx, yy, yz, yw);
	m_vec[2].set(zx, zy, zz, zw);
	m_vec[3].set(wx, wy, wz, ww);
	return *this;
}

Matrix &Matrix::setIdentity(void) {
	set(1.f, 0.f, 0.f, 0.f,
		0.f, 1.f, 0.f, 0.f,
		0.f, 0.f, 1.f, 0.f,
		0.f, 0.f, 0.f, 1.f
	);
	return *this;
}

void Matrix::getOpenGLSubMatrix(float *m) {
	m[0]  = m_vec[0].x();
	m[1]  = m_vec[1].x();
	m[2]  = m_vec[2].x();
	m[3]  = m_vec[3].x();

	m[4]  = m_vec[0].y();
	m[5]  = m_vec[1].y();
	m[6]  = m_vec[2].y();
	m[7]  = m_vec[3].y();

	m[8]  = m_vec[0].z();
	m[9]  = m_vec[1].z();
	m[10] = m_vec[2].z();
	m[11] = m_vec[3].z();

	m[12] = m_vec[0].w();
	m[13] = m_vec[1].w();
	m[14] = m_vec[2].w();
	m[15] = m_vec[3].w();
}

const float *Matrix::getOpenGLSubMatrix(void) {
	getOpenGLSubMatrix(m_glmatrix);
	return m_glmatrix;
}

Matrix::operator float *() {
	getOpenGLSubMatrix(m_glmatrix);
	return m_glmatrix;
}

float *Matrix::asArray() {
	getOpenGLSubMatrix(m_glmatrix);
	return m_glmatrix;
}

Matrix Matrix::scaled(const float &scale) const {
	auto m = Matrix(*this);
	return m.scale(scale);
}

Matrix Matrix::scaled(const Vector3 &scale) const {
	auto m = Matrix(*this);
	return m.scale(scale);
}

Matrix Matrix::absolute() const {
	return Matrix(
		fabsf(m_vec[0].x()), fabsf(m_vec[0].y()), fabsf(m_vec[0].z()), fabsf(m_vec[0].w()),
		fabsf(m_vec[1].x()), fabsf(m_vec[1].y()), fabsf(m_vec[1].z()), fabsf(m_vec[1].w()),
		fabsf(m_vec[2].x()), fabsf(m_vec[2].y()), fabsf(m_vec[2].z()), fabsf(m_vec[2].w()),
		fabsf(m_vec[3].x()), fabsf(m_vec[3].y()), fabsf(m_vec[3].z()), fabsf(m_vec[3].w())
	);
}
Matrix Matrix::transpose() const {
	return Matrix(
		m_vec[0].x(), m_vec[1].x(), m_vec[2].x(), m_vec[3].x(),
		m_vec[0].y(), m_vec[1].y(), m_vec[2].y(), m_vec[3].y(),
		m_vec[0].z(), m_vec[1].z(), m_vec[2].z(), m_vec[3].z(),
		m_vec[0].w(), m_vec[1].w(), m_vec[2].w(), m_vec[3].w()
	);
}

Matrix Matrix::inverse() const {
	Vector3 co(cofac(1, 1, 2, 2), cofac(1, 2, 2, 0), cofac(1, 0, 2, 1));
	float det = Vector3((*this)[0]).dot(co);

	float s = 1.0f / det;
	return Matrix(
		co.x() * s, cofac(0, 2, 2, 1) * s, cofac(0, 1, 1, 2) * s,
		co.y() * s, cofac(0, 0, 2, 2) * s, cofac(0, 2, 1, 0) * s,
		co.z() * s, cofac(0, 1, 2, 0) * s, cofac(0, 0, 1, 1) * s
	);
}

Matrix &Matrix::setRotate(const Vector3 &angle) {
	return setRotate(Quaternion(angle));
}

Matrix &Matrix::setRotate(const Quaternion &q) {
	float d = q.length2();
	float s = 2.f / d;
	float xs = q.x() * s,   ys = q.y() * s,   zs = q.z() * s;
	float wx = q.w() * xs,  wy = q.w() * ys,  wz = q.w() * zs;
	float xx = q.x() * xs,  xy = q.x() * ys,  xz = q.x() * zs;
	float yy = q.y() * ys,  yz = q.y() * zs,  zz = q.z() * zs;
	set(
        1.f - (yy + zz), xy - wz, xz + wy,
		xy + wz, 1.f - (xx + zz), yz - wx,
		xz - wy, yz + wx, 1.f - (xx + yy)
	);
	return *this;
}

Matrix &Matrix::setTranslate(const Vector3 &offset) {
	return setTranslate(offset.x(), offset.y(), offset.z());
}

Matrix &Matrix::setTranslate(const float &x, const float &y, const float &z) {
	set(
		1.f, 0.f, 0.f, x,
		0.f, 1.f, 0.f, y,
		0.f, 0.f, 1.f, z,
		0.f, 0.f, 0.f, 1.f
	);
	return *this;
}

Matrix &Matrix::setScale(const float &scale) {
	return setScale(scale, scale, scale);
}

Matrix &Matrix::setScale(const float &sx, const float &sy, const float &sz) {
	set(
		sx, 0.f, 0.f,
		0.f, sy, 0.f,
		0.f, 0.f, sz
	);
	return *this;
}

Matrix &Matrix::setScale(const Vector3 &scale) {
	return setScale(scale.x(), scale.y(), scale.z());
}

Matrix &Matrix::rotate(const Vector3 &angle) {
	return rotate(Quaternion(angle));
}

Matrix &Matrix::rotate(const Quaternion &q) {
	Matrix rot;
//	*this = rot.setRotate(q) * *this;
//	return *this;
	return (*this *= rot.setRotate(q));
}

Matrix &Matrix::translate(const Vector3 &offset) {
/*	m_vec[0][3] += offset[0];
	m_vec[1][3] += offset[1];
	m_vec[2][3] += offset[2]; */
	Matrix t;
//	*this = t.setTranslate(offset) * *this;
//	return *this;
	return (*this *= t.setTranslate(offset));
}

Matrix &Matrix::translate(const float &x, const float &y, const float &z) {
/*	m_vec[0][3] += x;
	m_vec[1][3] += y;
	m_vec[2][3] += z; */
	Matrix t;
//	*this = t.setTranslate(x, y, z) * *this;
//	return *this;
	return (*this *= t.setTranslate(x, y, z));
}

Matrix &Matrix::scale(const float &scale) {
	Matrix s;
//	*this = s.setScale(scale) * *this;
//	return *this;
	return (*this *= s.setScale(scale));
}

Matrix &Matrix::scale(const Vector3 &scale) {
	Matrix s;
//	*this = s.setScale(scale) * *this;
//	return *this;
	return (*this *= s.setScale(scale));
}

Matrix &Matrix::ortho(
	const float &left, const float &right,
	const float &bottom, const float &top,
	const float &near, const float &far) {

	const float width = right - left;
	const float height = top - bottom;
	const float depth = far - near;

	float a = 2.f * near / width;
	float b = 2.f * near / height;
	float c = -2.f / depth;
	float tx = (right + left) / width;
	float ty = (top + bottom) / height;
	float tz = (far + near) / depth;

	set(
		a, 0.f, 0.f, 0.f,
		0.f, b, 0.f, 0.f,
		0.f, 0.f, c, 0.f,
		tx, ty, tz, 1.f
	);
	return *this;
}

Matrix &Matrix::frustum(
	const float &left, const float &right,
	const float &bottom, const float &top,
	const float &near, const float &far) {

	const float width = right - left;
	const float height = top - bottom;
	const float depth = far - near;

	float a = 2.f * near / width;
	float b = 2.f * near / height;
	float c = (right + left) / width;
	float d = (top + bottom) / height;
	float e = -(far + near) / depth;
	float f = -2.f * far * near / depth;

	set(
		a, 0, c, 0,
		0, b, d, 0,
		0, 0, e, f,
		0, 0, -1, 0
	);
	return *this;
}

Matrix &Matrix::perspective(
	const float &fovy, const float &aspect,
	const float &zNear, const float &zFar) {

    float ymax = zNear * tanf((fovy > 0 ? fovy : 67.f) * M_PI / 360.f);
    float ymin = -ymax;
    float xmin = ymin * aspect;
    float xmax = ymax * aspect;

    return frustum(xmin, xmax, ymin, ymax, zNear, zFar);
}

Matrix &Matrix::lookAt(const Vector3 &eye, const Vector3 &at, const Vector3 &up) {

    Vector3 forward = (eye - at).normalized();
    Vector3 upNorm = up.normalized();
    Vector3 side = upNorm.cross(forward);
    upNorm = forward.cross(side);

    m_vec[0] = Vector4(side, 0.f);
    m_vec[1] = Vector4(upNorm, 0.f);
    m_vec[2] = Vector4(forward, 0.f);
    m_vec[3] = Vector4(0.f, 0.f, 0.f, 1.f);
//	result.f[0]=side.x;    result.f[4]=side.y;    result.f[8]=side.z;        result.f[12]=0;
//	result.f[1]=upNorm.x;  result.f[5]=upNorm.y;  result.f[9]=upNorm.z;      result.f[13]=0;
//	result.f[2]=forward.x; result.f[6]=forward.y; result.f[10]=forward.z;    result.f[14]=0;
//	result.f[3]=0;         result.f[7]=0;         result.f[11]=0;            result.f[15]=1.0;

    return translate(-eye);
}

Matrix &Matrix::lookAt(
	const float &eyeX, const float &eyeY, const float &eyeZ,
	const float &centerX, const float &centerY, const float &centerZ,
	const float &upX, const float &upY, const float &upZ) {

	Vector3 forward = Vector3(centerX - eyeX, centerY - eyeY, centerZ - eyeZ);
	forward.normalize();
/*  float fx = centerX - eyeX;
    float fy = centerY - eyeY;
    float fz = centerZ - eyeZ;

    // Normalize f
    float rlf = 1.0f / Matrix.length(fx, fy, fz);
    fx *= rlf;
    fy *= rlf;
    fz *= rlf; */


    // compute side = forward x up (x means "cross product")
	Vector3 up = Vector3(upX, upY, upZ);
	Vector3 side = forward.cross(up).normalize();
/*	float sx = fy * upZ - fz * upY;
    float sy = fz * upX - fx * upZ;
    float sz = fx * upY - fy * upX;

    // and normalize s
    float rls = 1.0f / Matrix.length(sx, sy, sz);
    sx *= rls;
    sy *= rls;
    sz *= rls; */

    // compute u = s x f
	Vector3 u = side.cross(forward);
/*	float ux = sy * fz - sz * fy;
    float uy = sz * fx - sx * fz;
    float uz = sx * fy - sy * fx; */

/*	set(
    	sx,		sy,		sz,		0.f,
    	ux,		uy,		uz,		0.f,
    	-fx,	-fy,	-fz,	0.f,
    	0.f,	0.f,	0.f,	1.f
    ); */
    set(
    	side.x(),	side.y(),	side.z(),	0.f,
    	u.x(),	u.y(),	u.z(),	0.f,
    	-forward.x(),	-forward.y(),	-forward.z(),	0.f,
    	0.f,	0.f,	0.f,	1.f
    );

    return translate(-eyeX, -eyeY, -eyeZ);
}

/**@brief Calculate the matrix cofactor
* @param r1 The first row to use for calculating the cofactor
* @param c1 The first column to use for calculating the cofactor
* @param r1 The second row to use for calculating the cofactor
* @param c1 The second column to use for calculating the cofactor
* See http://en.wikipedia.org/wiki/Cofactor_(linear_algebra) for more details
*/
inline float Matrix::cofac(int r1, int c1, int r2, int c2) const  {
	return m_vec[r1][c1] * m_vec[r2][c2] - m_vec[r1][c2] * m_vec[r2][c1];
}

inline float Matrix::tdotx(const Vector3 &v) const {
	return m_vec[0].x() * v.x() + m_vec[1].x() * v.y() + m_vec[2].x() * v.z() + m_vec[3].x() * v[3];
}

inline float Matrix::tdoty(const Vector3 &v) const {
	return m_vec[0].y() * v.x() + m_vec[1].y() * v.y() + m_vec[2].y() * v.z() + m_vec[3].y() * v[3];
}

inline float Matrix::tdotz(const Vector3 &v) const {
	return m_vec[0].z() * v.x() + m_vec[1].z() * v.y() + m_vec[2].z() * v.z() + m_vec[3].z() * v[3];
}

inline float Matrix::tdotw(const Vector3 &v) const {
	return m_vec[0].w() * v.x() + m_vec[1].w() * v.y() + m_vec[2].w() * v.z() + m_vec[3].w() * v[3];
}

//********************************************************************************
//********************************************************************************
Matrix operator*(const Matrix &m, const float &k) {
	return Matrix(
		m[0].x() * k, m[0].y() * k, m[0].z() * k,
		m[1].x() * k, m[1].y() * k, m[1].z() * k,
		m[2].x() * k, m[2].y() * k, m[2].z() * k
	);
}

// return m1 * m2
Matrix operator*(const Matrix &m1, const Matrix m2) {
/*    return Matrix(
		m2.tdotx(m1[0]), m2.tdoty(m1[0]), m2.tdotz(m1[0]), m2.tdotw(m1[0]),
		m2.tdotx(m1[1]), m2.tdoty(m1[1]), m2.tdotz(m1[1]), m2.tdotw(m1[1]),
		m2.tdotx(m1[2]), m2.tdoty(m1[2]), m2.tdotz(m1[2]), m2.tdotw(m1[2]),
		m2.tdotx(m1[3]), m2.tdoty(m1[3]), m2.tdotz(m1[3]), m2.tdotw(m1[3])
	); */
	// 上の式だと思っていたのと逆順に掛けられてしまうので、かける順を逆に変更
    return Matrix(
        m1.tdotx(m2[0]), m1.tdoty(m2[0]), m1.tdotz(m2[0]), m1.tdotw(m2[0]),
        m1.tdotx(m2[1]), m1.tdoty(m2[1]), m1.tdotz(m2[1]), m1.tdotw(m2[1]),
        m1.tdotx(m2[2]), m1.tdoty(m2[2]), m1.tdotz(m2[2]), m1.tdotw(m2[2]),
        m1.tdotx(m2[3]), m1.tdoty(m2[3]), m1.tdotz(m2[3]), m1.tdotw(m2[3])
	);
}

Vector3 operator*(const Matrix &m, const Vector3 &v) {
	Vector4 v4 = Vector4(v, 1.f);
	return Vector3(m[0].dot(v4), m[1].dot(v4), m[2].dot(v4));
}


Vector3 operator*(const Vector3 &v, const Matrix &m) {
	Vector4 v4 = Vector4(v, 1.f);
	return Vector3(m.tdotx(v4), m.tdoty(v4), m.tdotz(v4));
}

Matrix operator+(const Matrix &m1, const Matrix &m2) {
	return Matrix(
        m1[0][0] + m2[0][0],
        m1[0][1] + m2[0][1],
        m1[0][2] + m2[0][2],
        m1[0][3] + m2[0][3],

        m1[1][0] + m2[1][0],
        m1[1][1] + m2[1][1],
        m1[1][2] + m2[1][2],
        m1[1][3] + m2[1][3],

        m1[2][0] + m2[2][0],
        m1[2][1] + m2[2][1],
        m1[2][2] + m2[2][2],
        m1[2][3] + m2[2][3],

        m1[3][0] + m2[3][0],
        m1[3][1] + m2[3][1],
        m1[3][2] + m2[3][2],
        m1[3][3] + m2[3][3]
	);
}

Matrix operator-(const Matrix &m1, const Matrix &m2) {
	return Matrix(
		m1[0][0] - m2[0][0],
		m1[0][1] - m2[0][1],
		m1[0][2] - m2[0][2],
		m1[0][3] - m2[0][3],

		m1[1][0] - m2[1][0],
		m1[1][1] - m2[1][1],
		m1[1][2] - m2[1][2],
		m1[1][3] - m2[1][3],

		m1[2][0] - m2[2][0],
		m1[2][1] - m2[2][1],
		m1[2][2] - m2[2][2],
		m1[2][3] - m2[2][3],

		m1[3][0] - m2[3][0],
		m1[3][1] - m2[3][1],
		m1[3][2] - m2[3][2],
		m1[3][3] - m2[3][3]
	);
}

}	// namespace serenegiant::math

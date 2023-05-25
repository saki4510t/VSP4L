/*
 * aAndUsb
 * Copyright (c) 2014-2023 saki t_saki@serenegiant.com
 * Distributed under the terms of the GNU Lesser General Public License (LGPL v3.0) License.
 * License details are in the file license.txt, distributed as part of this software.
 */

#ifndef MATRIX_H_
#define MATRIX_H_

#include <memory>

#include "vec.h"
#include "quaternion.h"

namespace serenegiant::math {

class Matrix;
//********************************************************************************
//********************************************************************************
Matrix operator*(const Matrix &m, const float &k);
Matrix operator*(const Matrix &m1, const Matrix m2);
Vector3 operator*(const Matrix &m, const Vector3 &v);
Vector3 operator*(const Vector3 &v, const Matrix &m);
Matrix operator+(const Matrix &m, const Matrix &m2);
Matrix operator-(const Matrix &m1, const Matrix &m2);

//********************************************************************************
//********************************************************************************
class Matrix {
private:
	float m_glmatrix[16]{};
protected:
	Vector4 m_vec[4];
public:
	Matrix() { setIdentity(); };
	Matrix(const Matrix &src);
	Matrix(const float *mat);
	Matrix(const float &xx, const float &xy, const float &xz,
			const float &yx, const float &yy, const float &yz,
			const float &zx, const float &zy, const float &zz);
	Matrix(
		const float &xx, const float &xy, const float &xz, const float &xw,
		const float &yx, const float &yy, const float &yz, const float &yw,
		const float &zx, const float &zy, const float &zz, const float &zw,
		const float &wx, const float &wy, const float &wz, const float &ww);
//	Matrix &operator=(const Matrix &other);	// デフォルトの代入演算子でOK
	Vector4 getColumn(int i) const;
	const Vector4 &getRow(int i) const;
	Vector4 &operator[](int i);
	const Vector4 &operator[](int i) const;
	Matrix &operator*=(const Matrix &m);
	Matrix &operator+=(const Matrix &m);
	Matrix &operator-=(const Matrix &m);
	bool operator==(const Matrix &m);
	bool operator==(const Matrix &m) const;
	Matrix &setFromOpenGLSubMatrix(const float *m);
	Matrix &set(
		const float &xx, const float &xy, const float &xz,
		const float &yx, const float &yy, const float &yz,
		const float &zx, const float &zy, const float &zz);
	Matrix &set(
		const float &xx, const float &xy, const float &xz, const float &xw,
		const float &yx, const float &yy, const float &yz, const float &yw,
		const float &zx, const float &zy, const float &zz, const float &zw,
		const float &wx, const float &wy, const float &wz, const float &ww);
	Matrix &setIdentity(void);
	void getOpenGLSubMatrix(float *m);
	const float *getOpenGLSubMatrix(void);
	operator float *();
	float *asArray();
	Matrix scaled(const float &scale) const;
	Matrix scaled(const Vector3 &s) const;
	Matrix absolute() const;
	Matrix transpose() const;
	Matrix inverse() const;
	Matrix &setRotate(const Vector3 &angle);
	Matrix &setRotate(const Quaternion &q);
	Matrix &setTranslate(const Vector3 &offset);
	Matrix &setTranslate(const float &x, const float &y, const float &z);
	Matrix &setScale(const float &scale);
	Matrix &setScale(const float &sx, const float &sy, const float &sz);
	Matrix &setScale(const Vector3 &scale);
	Matrix &rotate(const Vector3 &angle);
	Matrix &rotate(const Quaternion &q);
	Matrix &translate(const Vector3 &offset);
	Matrix &translate(const float &x, const float &y, const float &z);
	Matrix &scale(const float &scale);
	Matrix &scale(const Vector3 &scale);
	Matrix &ortho(
		const float &left, const float &right,
		const float &bottom, const float &top,
		const float &near, const float &far);
	Matrix &frustum(
		const float &left, const float &right,
		const float &bottom, const float &top,
		const float &near, const float &far);
	Matrix &perspective(
		const float &fovy, const float &aspect,
		const float &zNear, const float &zFar);
	Matrix &lookAt(
		const float &eyeX, const float &eyeY, const float &eyeZ,
		const float &centerX, const float &centerY, const float &centerZ,
		const float &upX, const float &upY, const float &upZ);
	Matrix &lookAt(const Vector3 &eye, const Vector3 &at, const Vector3 &up);

	inline float cofac(int r1, int c1, int r2, int c2) const;
	inline float tdotx(const Vector3 &v) const;
	inline float tdoty(const Vector3 &v) const;
	inline float tdotz(const Vector3 &v) const;
	inline float tdotw(const Vector3 &v) const;
};

typedef std::shared_ptr<Matrix> MatrixSp;
typedef std::unique_ptr<Matrix> MatrixUp;

} // namespace serenegiant::math

#endif /* MATRIX_H_ */

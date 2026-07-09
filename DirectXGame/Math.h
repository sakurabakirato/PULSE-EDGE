#pragma once

#include <KamataEngine.h>

using namespace KamataEngine;

// class Math
//{
// public:

// 加算
Vector3 Add(const Vector3& v1, const Vector3& v2);

// 減算
Vector3 Subtract(const Vector3& v1, const Vector3& v2);

// スカラー倍
Vector3 Multiply(float scalar, const Vector3& v1);

// 内積
float Dot(const Vector3& v1, const Vector3& v2);

// 長さ(ノルム)
float Length(const Vector3& v1);

// 正規化
Vector3 Normalize(const Vector3& v);

// 1.行列の加法
Matrix4x4 Add(const Matrix4x4& m1, const Matrix4x4& m2);

// 2.行列の減法
Matrix4x4 Subtract(const Matrix4x4& m1, const Matrix4x4& m2);

// 3.行列の積
Matrix4x4 Multiply(const Matrix4x4& m1, const Matrix4x4& m2);

// 4.逆行列
Matrix4x4 Inverse(const Matrix4x4& m);

// 5.転置行列
Matrix4x4 Transpose(const Matrix4x4& m);

// 6.単位行列の作成
Matrix4x4 MakeIdenity4x4();

// 1.平行移動行列
Matrix4x4 MakeTranslateMatrix(const Vector3& translate);

// 2.拡大縮小行列
Matrix4x4 MakeScaleMatrix(const Vector3& scale);

// 3.座標変換
Vector3 Transform(const Vector3& vector, const Matrix4x4& matrix);

// X軸回転行列
Matrix4x4 MakeRotateXMatrix(float radian);

// Y軸回転行列
Matrix4x4 MakeRotateYMatrix(float radian);

// Z軸回転行列
Matrix4x4 MakeRotateZMatrix(float radian);

// 3次元アフィン変換行列
Matrix4x4 MakeAffineMatrix(const Vector3& scale, const Vector3& rotate, const Vector3& translate);

// イージング
float EaseInOut(float start, float end, float t);

Vector3& operator+=(Vector3& lhv, const Vector3& rhv);

Vector3& operator-=(Vector3& lhv, const Vector3& rhv);

Vector3& operator*=(Vector3& v, float s);

Vector3& operator/=(Vector3& v, float s);

Vector3 operator*(const Vector3& v1, const float f);

Matrix4x4& operator*=(Matrix4x4& lhm, const Matrix4x4& rhm);

Vector3 operator+(const Vector3& v1, const Vector3& v2);

Matrix4x4 operator*(const Matrix4x4& m1, const Matrix4x4& m2);

float Lerp(float x1, float x2, float t);

Vector3 Lerp(const Vector3& v1, const Vector3& v2, float t);

struct AABB {
	Vector3 min;
	Vector3 max;
};

bool IsCollision(const AABB& aabb1, const AABB& aabb2);

//}
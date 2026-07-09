#pragma once

#include "KamataEngine.h"
#include "Math.h"
#include "UpData.h"
#include <algorithm>
#include <array>
#include <numbers>

using namespace KamataEngine;

/// <summary>
/// デス演出用パーティクル
/// </summary>
// 02_11_7枚目 デス演出用パーティクル
class DeathParticles {
public:
	// 02_11_8枚目 Initialize,Update,Draw関数追加
	void Initialize(Model* model, Camera* camera, const Vector3& position);
	void Update();
	void Draw();

	// 02_12 34枚目で実装
	bool IsFinished() { return isFinished_; }

private:
	// 02_11_10枚目パーティクルの個数
	static inline const uint32_t kNumParticles = 8;

	// 02_11_10枚目パーティクル座標配列
	std::array<WorldTransform, kNumParticles> worldTransforms_;

	// 02_11_13枚目 モデル
	Model* model_ = nullptr;

	// 02_11_13枚目 カメラ
	Camera* camera_ = nullptr;

	// 02_11_22枚目 存続時間（消滅までの時間）<秒>
	static inline const float kDuration_ = 2.0f;

	// 02_11_22枚目 移動の速さ
	static inline const float kSpeed_ = 0.05f;

	// 02_11_22枚目 分割した1個分の角度(#include <numbers>)
	static inline const float kAngleUnit_ = 2.0f * std::numbers::pi_v<float> / kNumParticles;

	// 02_11_25枚目 終了フラグ
	bool isFinished_ = false;

	// 02_11_25枚目 経過時間カウント
	float counter_ = 0.0f;

	// 02_11_30枚目 色変更オブジェクト
	ObjectColor objectColor_;

	// 02_11_30枚目 色の数値
	Vector4 color_;

	UpData* upData = nullptr;

	// ワールド変換データ
	WorldTransform worldTransform_;
};
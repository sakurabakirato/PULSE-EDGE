#pragma once
#include "Enemy.h"
#include "KamataEngine.h"
#include "MapChipField.h"
#include "Math.h"
#include "UpData.h"

#include <algorithm>
#include <cassert>
#include <numbers>

class MapChipField;

using namespace KamataEngine;

// 02_10 21枚目
class Enemy;

class Player {
public:
	// 左右
	enum class LRDirection {
		kRight,
		kLeft,
	};

	// 角 02_07スライド16枚目
	enum Corner { kRightBottom, kLeftBottom, kRightTop, kLeftTop, kNumCorner };

	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize(Model* model_, Camera* camera_, const Vector3& position);

	/// <summary>
	/// 更新
	/// </summary>
	void UpDate();

	/// <summary>
	/// 描画
	/// </summary>
	void Draw();

	// getter(02_06スライド11枚目で追加)
	const WorldTransform& GetWorldTransform() const { return worldTransform_; }

	// 02_06スライド28枚目で追加
	const Vector3& GetVelocity() const { return velocity_; }

	// 02_07 スライド4枚目
	void SetMapChipField(MapChipField* mapChipField) { mapChipField_ = mapChipField; }

	// 02_10 10枚目 ワールド座標を取得
	Vector3 GetWorldPosition();

	// AABBを取得
	AABB GetAABB();

	// 02_10 21枚目 衝突応答
	void OnCollision(const Enemy* enemy);

	// 02_12 11枚目 デスフラグ
	bool IsDead() const { return isDead_; }

private:
	// ワールド変換データ
	WorldTransform worldTransform_;

	// モデル
	Model* model_ = nullptr;

	// テクスチャハンドル
	//  uint32_t textureHandle_ = 0u;

	Camera* camera_ = nullptr;

	// Math* math_ = nullptr;

	UpData* upData = nullptr;

	Vector3 velocity_ = {};

	// 02_05  フレームごとの加速度
	static inline const float kAcceleration = 0.1f;

	// 02_05 顔の向き
	LRDirection lrDirection_ = LRDirection::kRight;

	// 02_05 非入力時の摩擦係数
	static inline const float kAttenuation = 0.05f;

	// 速度制限
	//  02_05 最高速度
	static inline const float kLimitRunSpeed = 0.3f;

	// std::vector<std::vector<WorldTransform*>> worldTransformBlocks_;

	// 旋回開始時の角度
	float turnFirstRotationY_ = 0.0f;

	// 旋回タイマー
	float turnTimer_ = 0.0f;

	// 旋回時間<秒>
	static inline const float kTimeTurn = 0.3f;

	// 接地状態フラグ
	bool onGround_ = true;

	// 02_05 ジャンプ定数もろもろ

	// ジャンプ初速(上方向)
	static inline const float kJumpAcceleration = 20.0f;

	// 重力加速度(下方向)
	static inline const float kGravityAcceleration = 0.98f;

	// 最大落下速度(下方向)
	static inline const float kLimitFallSpeed = 0.5f;

	// 02_07 マップチップによるフィールド
	MapChipField* mapChipField_ = nullptr;

	// キャラクターの当たり判定サイズ
	static inline const float kWidth = 0.8f;
	static inline const float kHeight = 0.8f;

	// 02_07スライド34枚目
	static inline const float kBlank = 0.04f;

	// 着地時の速度減算衰率
	static inline const float kAttenuationLanding = 0.0f;

	// 02_08スライド21枚目 微小な数値
	static inline const float kGroundSearchHeight = 0.06f;

	// 02_08スライド27枚目 着地時の速度減衰率
	static inline const float kAttenuationWall = 0.2f;

	void InputMove();

	// マップチップとの当たり判定情報
	// 02_07 スライド12枚目
	struct CollisionMapInfo {
		// 天井衝突フラグ
		bool ceiling = false;
		// 着地フラグ
		bool landing = false;
		// 壁接触フラグ
		bool hitWall = false;
		// 移動量
		Vector3 move;
	};

	// 02_07 スライド13枚目
	void CheckMapCollision(CollisionMapInfo& info);

	// 02_07 スライド14枚目
	void CheckMapCollisionUp(CollisionMapInfo& info);
	void CheckMapCollisionDown(CollisionMapInfo& info);
	void CheckMapCollisionRight(CollisionMapInfo& info);
	void CheckMapCollisionLeft(CollisionMapInfo& info);

	// 02_07 スライド17枚目
	Vector3 CornerPosition(const Vector3& center, Corner corner);

	// 02_08スライド14枚目 設置状態の切り替え処理
	void UpdateOnGround(const CollisionMapInfo& info);

	// 02_08 スライド27枚目 壁接触している場合の処理
	void UpdateOnWall(const CollisionMapInfo& info);

	// 02_12 11枚目 デスフラグ
	bool isDead_ = false;
};
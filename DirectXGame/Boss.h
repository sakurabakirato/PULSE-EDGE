#pragma once
#include "KamataEngine.h"
#include "MapChipField.h"
#include "Math.h"
#include "UpDate.h"
#include "Player.h"

using namespace KamataEngine;

class Player;

class Boss
{
public:
	/// <summary>
	/// 初期化
	/// </summary>
	/// <param name="model_"></param>
	/// <param name="camera_"></param>
	/// <param name="position"></param>
	void Initialize(Model* model, Camera* camera, MapChipField* mapChipField, const Vector3& position);

	/// <summary>
	/// 更新
	/// </summary>
	void Update();

	/// <summary>
	/// 描画
	/// </summary>
	void Draw();

	// 02_10 スライド14枚目
	AABB GetAABB();
	// 02_10 スライド14枚目 ワールド座標を取得
	Vector3 GetWorldPosition();
	// 02_10 スライド20枚目 衝突応答
	void OnCollision(Player* player);

	// ふるまい
	enum class Behavior
	{
		kUnknown = -1, // 無効な状態

		kWalk,     // 歩数状態
		kDefeated, // やられ状態

	};

	void CheckWallAndReverse();

	bool ShouldDropBlock() const { return canBlockAttack_; }
	void ConsumeBlockAttack() { canBlockAttack_ = false; }

	bool IsDead() const { return hp_ == 0; }

	void OnDamage(int amount);

	bool IsCollisionDisabled() const { return isCollisionDisabled; }

	bool IsAttack();

	bool isDead = false;

	int hp_ = 2;


private:
	// ワールド変換データ
	WorldTransform worldTransform_;

	// モデル
	Model* model_ = nullptr;

	UpDate* upDate_ = nullptr;

	// テクスチャハンドル
	//  uint32_t textureHandle_ = 0u;

	Camera* camera_ = nullptr;

	// 02_09 15枚目
	static inline const float kWalkSpeed = 0.1f;
	// 02_09 15枚目
	Vector3 velocity_ = {};

	// 02_09 19枚目
	// 最初の角度
	static inline const float kWalkMotionAngleStart = -10.0f;

	// 02_09 19枚目
	// 最後の角度
	static inline const float kWalkMotionAngleEnd = 40.0f;

	// 02_09 19枚目
	static inline const float kWalkMotionTime = 1.0f;
	// 02_09 20枚目
	float walkTimer = 0.0f;

	// 02_10 14枚目 当たり判定サイズ
	static inline const float kWidth = 0.8f;
	static inline const float kHeight = 0.8f;

	float counter = 0.0f;
	float kDefeatedTime = 0.0f;

	bool isFacingRight_ = false;  // false = 左向き、true = 右向き

	// デスフラグ
	bool isCollisionDisabled = false;
	bool gameScene = true;

	// bool IsAttack();
	Behavior behavior = Behavior::kWalk;
	// 次の振るまいリクエスト
	Behavior behaviorRequest = Behavior::kUnknown;


	MapChipField* mapChipField_ = nullptr;

	AABB aabb_ = {};

	int blockAttackTimer_ = 0;
	bool canBlockAttack_ = true;



};


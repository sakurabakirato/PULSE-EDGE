#pragma once
#include "KamataEngine.h"
#include "MapChipField.h"
#include "Math.h"
#include "Player.h"
#include <list>

using namespace KamataEngine;

class Player;

class Boss
{
public:
	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize(Model* model, Model* bulletModel, Camera* camera, MapChipField* mapChipField, Player* player, const Vector3& position);

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

	bool ShouldDropBlock() const { return canBlockAttack_; }
	void ConsumeBlockAttack() { canBlockAttack_ = false; }

	//ボスのHPが0以下なら死ぬ
	bool IsDead() const { return hp_ <= 0; }

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

	// ボスが移動する4か所
	Vector3 movePositions_[4] = {};

	// 現在向かっている場所
	int movePositionIndex_ = 0;

	// 到着後の待機時間
	float moveWaitTimer_ = 0.0f;

	// ボスの弾
	struct Bullet
	{
		WorldTransform worldTransform;
		Vector3 velocity = {};
		float lifeTimer = 0.0f;
		bool homing = false;
	};

	Model* bulletModel_ = nullptr;
	Player* player_ = nullptr;

	std::list<Bullet> bullets_;

	// 0=円形、1=ホーミング、2=3列攻撃
	int attackType_ = 0;

	// 現在の待機中に攻撃したか
	bool attackDone_ = false;

	// 弾を発射
	void Attack();

};


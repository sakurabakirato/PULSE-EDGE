#pragma once
#include "KamataEngine.h"
#include "MapChipField.h"
#include "Math.h"
#include "UpDate.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <numbers>

class MapChipField;

using namespace KamataEngine;

class Boss;

class Player 
{
public:

	//デストラクタ
	~Player();

	// 左右
	enum class LRDirection 
	{
		kRight,
		kLeft,
	};

	enum class PlayerState 
	{
		kIdle, // 待機
		kRun,  // 走行
		kJump, // 上昇
		kFall, // 落下
		kDead, // 死亡
	};

	// ふるまい
	enum class Behavior
	{
		kUnknown = -1, // 無効な状態
		kRoot,
		kAttack,

	};

	// 攻撃フェーズ
	enum class AttackPhase
	{
		kUnknown = -1, // 無効な状態
		kAnticipation, // 予備の動作
		kAction,       // 前進の動作
		kRecovery,     // 余韻の動作
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
	void Update();
	// 登場演出専用。キー入力や攻撃を処理せず、移動・重力・接地だけを更新。
	bool UpdateEntrance(float targetX);
	void StopEntrance();

	/// <summary>
	/// 描画
	/// </summary>
	void Draw();
	// GameSceneのModel::PostDraw()の直後、Sprite描画より前に呼ぶ。
	void DrawAttackEffects();

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
	// 攻撃判定を取得
	AABB GetAttackAABB();

	// 02_10 21枚目 衝突応答
	void OnCollision(Boss* boss);
	void OnBulletCollision(); // ボスの弾の当たり判定

	// 02_12 11枚目 デスフラグ
	bool IsDead() const { return isDead_; }
	int GetHp() const { return hp_; }

	//攻撃中かどうかを確認する
	bool IsAttack();
	// GameSceneの攻撃判定から呼ぶ。命中済みの同じ攻撃は再適用しない。
	bool TryHitBoss(Boss* boss);

private:
	// ブリンク攻撃。既存と同じ60Hz更新を前提とする。
	static constexpr float kBlinkDistance = 2.4f;
	static constexpr float kBlinkCooldown = 0.45f;
	static constexpr float kBlinkRecovery = 0.15f;
	// 攻撃の時間割（秒）。構え→振り抜き→ビーム。
	static constexpr float kWindupTime = 0.05f;
	static constexpr float kSwingTime = 0.10f;
	static constexpr float kBeamStartTime = kWindupTime + kSwingTime;
	static constexpr float kSlashDuration = 0.40f;
	static constexpr float kBeamGrowTime = 0.05f;
	static constexpr float kBeamMaxLength = 8.0f;
	static constexpr float kBeamHalfWidth = 0.30f;
	static constexpr float kSwordLength = 1.65f;
	// CreateSphereの元の半径。見た目の大きさが違う場合はここを調整。
	static constexpr float kEffectSphereRadius = 1.0f;
	static constexpr int kArcSegments = 20;
	float slashElapsed_ = 0.0f;
	bool beamSpawned_ = false;
	Vector3 gripPosition_ = {};
	Vector3 beamStart_ = {};
	float beamMaxLength_ = 0.0f;
	float beamLength_ = 0.0f;
	WorldTransform swordTransform_;
	WorldTransform beamOuter_;
	WorldTransform beamCore_;
	std::array<WorldTransform, kArcSegments> slashArc_;
	std::array<ObjectColor, kArcSegments> arcColors_;
	ObjectColor swordColor_;
	ObjectColor beamOuterColor_;
	ObjectColor beamCoreColor_;
	ObjectColor flashColor_;
	ObjectColor trailColor_;
	void UpdateSwordAndBeam();
	float GetBeamWallDistance(const Vector3& origin, float direction, float maximum, float halfHeight);
	static void SetEffectSegment(WorldTransform& transform, const Vector3& a, const Vector3& b, float halfThickness, float halfDepth);
	static constexpr int kTrailCount = 6;
	float blinkCooldownTimer_ = 0.0f;
	float blinkRecoveryTimer_ = 0.0f;
	float trailTimer_ = 0.0f;
	float attackDirection_ = 1.0f;
	bool attackHit_ = false; // 現在の単体ボス用。一発につき一回だけダメージ。
	Vector3 slashPosition_ = {};
	std::array<WorldTransform, kTrailCount> blinkTrails_;
	WorldTransform slashFlash_;
	void StartBlinkAttack();
	void UpdateBlinkEffects();
	// 全経路のブロック境界を調べ、壁の手前までの距離を返す。
	float GetSafeHorizontalMove(float distance, float halfWidth, float halfHeight);
	static void UpdateTransform(WorldTransform& transform);

	// ワールド変換データ
	WorldTransform worldTransform_;

	// モデル
	Model* model_ = nullptr;

	Camera* camera_ = nullptr;



	Vector3 velocity_ = {};

	// 02_05 顔の向き
	LRDirection lrDirection_ = LRDirection::kRight;

	// 速度制限
	//  02_05 最高速度
	static inline const float kLimitRunSpeed = 0.15f;

	// 旋回開始時の角度
	float turnFirstRotationY_ = 0.0f;

	// 旋回タイマー
	float turnTimer_ = 0.0f;

	// 旋回時間<秒>
	static inline const float kTimeTurn = 0.3f;

	// 接地状態フラグ
	bool onGround_ = true;

	// 現在のジャンプ回数（地上と空中で合計2回まで）
	int jumpCount_ = 0;

	// 回避ダッシュ中か
	bool isDashing_ = false;

	// ダッシュの残り時間
	float dashTimer_ = 0.0f;

	// 次にダッシュできるまでの時間
	float dashCooldownTimer_ = 0.0f;

	// 現在の滞空中にダッシュを使用したか
	bool airDashUsed_ = false;

	// ダッシュ速度
	static inline const float kDashSpeed = 0.45f;

	// ダッシュ継続時間
	static inline const float kDashDuration = 0.18f;

	// ダッシュの再使用待ち時間
	static inline const float kDashCooldown = 0.35f;


	// ジャンプ開始時の上向き速度を決める値
    // 実際に velocity_.y に入る値は、この値 / 60
	static inline const float kJumpAcceleration = 24.0f;

	// 上昇中の重力
	static inline const float kGravityAcceleration = 1.4f;

	// 落下中は重力を強めて、着地までを素早くする
	static inline const float kFallGravityMultiplier = 1.7f;

	// 最大落下速度
	static inline const float kLimitFallSpeed = 0.5f;

	// SPACEを離したときに残す上向き速度の上限
	static inline const float kJumpReleaseSpeed = 0.12f;

	// 空中で左右キーを離したときの減速量
	static inline const float kAirBrake = 0.025f;



	// 02_07 マップチップによるフィールド
	MapChipField* mapChipField_ = nullptr;

	// キャラクターの当たり判定サイズ
	static inline const float kWidth = 0.8f;
	static inline const float kHeight = 0.8f;

	// 02_07スライド34枚目
	static inline const float kBlank = 0.04f;

	// 着地時の速度減算衰率
	static inline const float kAttenuationLanding = 0.0f;

	// 微小な数値
	static inline const float kGroundSearchHeight = 0.06f;

	// 着地時の速度減衰率
	static inline const float kAttenuationWall = 0.2f;

	void InputMove();

	// マップチップとの当たり判定情報
	struct CollisionMapInfo 
	{
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

	// 設置状態の切り替え処理
	void UpdateOnGround(const CollisionMapInfo& info);

	// 壁接触している場合の処理
	void UpdateOnWall(const CollisionMapInfo& info);

	// デスフラグ
	bool isDead_ = false;

	// プレイヤーのHP
	int hp_ = 3;

	// 被弾後の無敵時間
	float invincibleTimer_ = 0.0f;

	// 攻撃エフェクトの残り表示時間
	float attackEffectTimer_ = 0.0f;

	// 攻撃エフェクトの表示時間
	static inline const float kAttackEffectDuration = 0.15f;

	// 予備動作
	static inline const uint32_t kAnticipationTime = 8;
	// 前進動作の時間
	static inline const uint32_t kActionTime = 5;
	// 余韻動作の時間
	static inline const uint32_t kRecoveryTime = 12;
	Model* modelAttack = nullptr;
	Behavior behavior_ = Behavior::kRoot;
	// 次の振るまいリクエスト
	Behavior behaviorRequest_ = Behavior::kUnknown;
	// 攻撃フェーズ
	AttackPhase attackPhase = AttackPhase::kUnknown;
	AttackPhase attackPhaseRequest = AttackPhase::kUnknown;
};

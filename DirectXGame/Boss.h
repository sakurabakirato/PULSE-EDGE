#pragma once
#include "KamataEngine.h"
#include "MapChipField.h"
#include "Math.h"
#include "Player.h"
#include <list>
#include <array>

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
	void BeginIntroPose();
	bool UpdateIntroPose();
	void EndIntroPose();
	// 通常のModel::PostDraw()の後に呼ぶ。
	void DrawIntroShockwaves();

	
	AABB GetAABB();
	// ワールド座標を取得
	Vector3 GetWorldPosition();
	// 衝突応答
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

	// ボスの最大HP
	static inline const int kMaxHp = 30;

	// 現在HPを取得
	int GetHp() const { return hp_; }

	// 最大HPを取得
	int GetMaxHp() const { return kMaxHp; }


private:
	// 被弾演出。移動・当たり判定用のworldTransform_には揺れを加えない。
	static constexpr float kHitReactionDuration = 0.18f;
	static constexpr float kHitShakeAmplitude = 0.08f;
	static constexpr float kHitFlashInterval = 0.045f;
	float hitReactionTimer_ = 0.0f;
	WorldTransform renderTransform_;
	ObjectColor bodyColor_;
	void UpdateHitReaction();
	void UpdateRenderTransform();

	static constexpr float kIntroPoseDuration = 1.8f;
	static constexpr float kIntroLiftHeight = 0.65f;
	static constexpr float kIntroTiltAngle = -0.35f;
	static constexpr float kShockMaxRadius = 2.2f;
	static constexpr int kShockSegments = 32;
	bool introPoseActive_ = false;
	float introPoseTime_ = 0.0f;
	Vector3 introBasePosition_ = {};
	Vector3 introBaseRotation_ = {};
	Vector3 introBaseScale_ = {1.0f, 1.0f, 1.0f};
	std::array<WorldTransform, kShockSegments * 2> introShockTransforms_;
	std::array<ObjectColor, 2> introShockColors_;
	std::array<bool, 2> introShockVisible_ = {};
	void UpdateIntroMatrix();
	// ワールド変換データ
	WorldTransform worldTransform_;

	// モデル
	Model* model_ = nullptr;

	Camera* camera_ = nullptr;

	// 02_09 15枚目
	static inline const float kWalkSpeed = 0.2f;
	// 02_09 15枚目
	Vector3 velocity_ = {};

	// 最初の角度
	static inline const float kWalkMotionAngleStart = -10.0f;

	// 最後の角度
	static inline const float kWalkMotionAngleEnd = 40.0f;

	// 02_09 19枚目
	static inline const float kWalkMotionTime = 1.0f;
	// 02_09 20枚目
	float walkTimer = 0.0f;

	// 当たり判定サイズ
	static inline const float kWidth = 0.8f;
	static inline const float kHeight = 0.8f;

	float counter = 0.0f;
	float kDefeatedTime = 0.0f;

	bool isFacingRight_ = false;  // false = 左向き、true = 右向き

	// デスフラグ
	bool isCollisionDisabled = false;
	bool gameScene = true;

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

		// 発射までの待ち時間
		float launchDelay = 0.0f;

		bool homing = false;
		bool exploding = false;
	};

	Model* bulletModel_ = nullptr;
	Player* player_ = nullptr;

	std::list<Bullet> bullets_;

	// 0 = 円形、1 = ホーミング、2 = 3列攻撃、3 = 狙い撃ち連射
	int attackType_ = 0;

	// 現在の待機中に攻撃したか
	bool attackDone_ = false;
	bool isDead = false;

	// 弾を発射
	void Attack();

	// ボスの現在HP
	int hp_ = kMaxHp;

	// ボスが現在行っている行動
	enum class Action
	{
		kDashReady, // 突進の予備動作
		kDash,      // 突進
		kRecovery   // 攻撃後の硬直
	};

	//何をしてるか
	Action action_ = Action::kDashReady;

	//何秒経ったか
	// 現在の行動を開始してからの時間
	float actionTimer_ = 0.0f;

	// 突進方向。今回は左右だけなので、-1か1
	float dashDirectionX_ = -1.0f;

	// 調整する数値
	static constexpr float kDeltaTime = 1.0f / 60.0f;
	static constexpr float kDashReadyTime = 0.6f;
	static constexpr float kDashTime = 0.35f;
	static constexpr float kRecoveryTime = 1.0f;

	// 1秒あたりの移動量
	static constexpr float kDashSpeed = 15.0f;

	// 行動の管理
	void ChangeAction(Action next);
	void UpdateAction();

	// 各行動の具体的な処理
	void UpdateDashReady();
	void UpdateDash();
	void UpdateRecovery();

};


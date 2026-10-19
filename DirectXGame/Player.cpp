#define NOMINMAX
#include "Player.h"
#include "MapChipField.h"
#include "Math.h"
#include "UpDate.h"
#include "Boss.h"
#include <algorithm>
#include <cassert>
#include <numbers>
#include <cmath>

using namespace KamataEngine;

Player::~Player()
{
	delete modelAttack;
}

void Player::Initialize(Model* model, Camera* camera, const Vector3& position) 
{
	assert(model);
	assert(camera);

	model_ = model;
	camera_ = camera;

	worldTransform_.Initialize();
	worldTransform_.translation_ = position;
	worldTransform_.rotation_.y = std::numbers::pi_v<float> / 2.0f;

	velocity_ = {};
	onGround_ = true;
	jumpCount_ = 0;
	lrDirection_ = LRDirection::kRight;
	turnTimer_ = 0.0f;
	hp_ = 3;
	isDead_ = false;
	invincibleTimer_ = 0.0f;
	blinkCooldownTimer_ = 0.0f;
	blinkRecoveryTimer_ = 0.0f;
	trailTimer_ = 0.0f;
	attackHit_ = false;
	attackDirection_ = 1.0f;
	for (auto& trail : blinkTrails_) { trail.Initialize(); }
	swordTransform_.Initialize();
	beamOuter_.Initialize();
	beamCore_.Initialize();
	swordColor_.Initialize();
	beamOuterColor_.Initialize();
	beamCoreColor_.Initialize();
	flashColor_.Initialize();
	trailColor_.Initialize();
	for (int i = 0; i < kArcSegments; ++i)
	{
		slashArc_[i].Initialize();
		arcColors_[i].Initialize();
	}
	swordColor_.SetColor({0.85f, 0.95f, 1.0f, 1.0f});
	slashElapsed_ = 0.0f;
	beamSpawned_ = false;
	beamLength_ = 0.0f;
	beamMaxLength_ = 0.0f;
	slashFlash_.Initialize();
	UpdateTransform(worldTransform_);

	// 攻撃エフェクトには追加素材が不要な球モデルを使用
	delete modelAttack;
	modelAttack = Model::CreateSphere(8, 8);
	attackEffectTimer_ = 0.0f;

	isDashing_ = false;
	dashTimer_ = 0.0f;
	dashCooldownTimer_ = 0.0f;
	airDashUsed_ = false;
}

void Player::InputMove() 
{
	// ダッシュ中は向いている方向へ一定速度で進む
	if (isDashing_)
	{
		if (lrDirection_ == LRDirection::kRight)
		{
			velocity_.x = kDashSpeed;
		}
		else
		{
			velocity_.x = -kDashSpeed;
		}

		velocity_.y = 0.0f;
		return;
	}

	// 左Shiftを押すと回避ダッシュを開始
	if (Input::GetInstance()->TriggerKey(DIK_LSHIFT) &&
		dashCooldownTimer_ <= 0.0f &&
		!airDashUsed_)
	{
		isDashing_ = true;
		dashTimer_ = kDashDuration;
		dashCooldownTimer_ = kDashCooldown;

		if (!onGround_)
		{
			airDashUsed_ = true;
		}

		if (lrDirection_ == LRDirection::kRight)
		{
			velocity_.x = kDashSpeed;
		}
		else
		{
			velocity_.x = -kDashSpeed;
		}

		velocity_.y = 0.0f;
		return;
	}

	// 地上・空中共通の左右操作
	if (Input::GetInstance()->PushKey(DIK_D)) 
	{
		// 押した瞬間に右方向の最高速度へ切り替える
		velocity_.x = kLimitRunSpeed;

		if (lrDirection_ != LRDirection::kRight) 
		{
			lrDirection_ = LRDirection::kRight;

			turnFirstRotationY_ = worldTransform_.rotation_.y;
			turnTimer_ = kTimeTurn;
		}
	} 
	else if (Input::GetInstance()->PushKey(DIK_A)) 
	{
		// 押した瞬間に左方向の最高速度へ切り替える
		velocity_.x = -kLimitRunSpeed;

		if (lrDirection_ != LRDirection::kLeft) 
		{
			lrDirection_ = LRDirection::kLeft;

			turnFirstRotationY_ = worldTransform_.rotation_.y;
			turnTimer_ = kTimeTurn;
		}
	}
	else
	{
		if (onGround_)
		{
			// 地上ではすぐ停止する
			velocity_.x = 0.0f;
		} 
		else
		{
			// 空中では短時間で減速する
			if (velocity_.x > 0.0f)
			{
				velocity_.x = (std::max)(velocity_.x - kAirBrake, 0.0f);
			} 
			else if (velocity_.x < 0.0f)
			{
				velocity_.x = (std::min)(velocity_.x + kAirBrake, 0.0f);
			}
		}
	}
	
	// 今回の更新でジャンプを開始したか
	bool jumpedThisFrame = false;

	// SPACEを押した瞬間にジャンプする
	if (Input::GetInstance()->TriggerKey(DIK_SPACE) &&
		jumpCount_ < 2)
	{
		// 二段ジャンプでも上向き速度を設定し直す
		velocity_.y = kJumpAcceleration / 60.0f;

		onGround_ = false;
		++jumpCount_;

		jumpedThisFrame = true;
	}

	if (!onGround_)
	{
		// 上昇中にSPACEを離したら、上向き速度を抑える
		// 一度弱めたあとに長押ししても、速度は戻らない
		if (!jumpedThisFrame &&
			!Input::GetInstance()->PushKey(DIK_SPACE) &&
			velocity_.y > kJumpReleaseSpeed)
		{
			velocity_.y = kJumpReleaseSpeed;
		}

		// ジャンプ開始フレームは初速をそのまま使う
		if (!jumpedThisFrame)
		{
			float gravity = kGravityAcceleration / 60.0f;

			// 落下中は重力を強める
			if (velocity_.y <= 0.0f)
			{
				gravity *= kFallGravityMultiplier;
			}

			velocity_.y -= gravity;

			// 落下速度が上がり続けないよう制限する
			velocity_.y =
				(std::max)(velocity_.y, -kLimitFallSpeed);
		}
	}
}

// 02_07 スライド13枚目
void Player::CheckMapCollision(CollisionMapInfo& info) 
{
	CheckMapCollisionUp(info);
	CheckMapCollisionDown(info);
	CheckMapCollisionRight(info);
	CheckMapCollisionLeft(info);
}

// マップ衝突判定上方向
void Player::CheckMapCollisionUp(CollisionMapInfo& info) 
{
	// 上昇あり？
	if (info.move.y <= 0) 
	{
		return;
	}
	// 移動後の4つの角の計算
	std::array<Vector3, kNumCorner> positionsNew{};

	for (uint32_t i = 0; i < positionsNew.size(); ++i) 
	{
		positionsNew[i] = CornerPosition(worldTransform_.translation_ + info.move, static_cast<Corner>(i));
	}

	MapChipType mapChipType;
	MapChipType mapChipTypeNext;
	// 真上の当たり判定を行う
	bool hit = false;
	// 左上点の判定
	MapChipField::IndexSet indexSet;
	indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kLeftTop]);
	mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
	mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex + 1);
	if (mapChipType == MapChipType::kBlock && mapChipTypeNext != MapChipType::kBlock) 
	{
		hit = true;
	}
	// 右上点の判定
	indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kRightTop]);
	mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
	mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex + 1);
	if (mapChipType == MapChipType::kBlock && mapChipTypeNext != MapChipType::kBlock) 
	{
		hit = true;
	}
	// ブロックにヒット？
	if (hit) 
	{
		// めり込みを排除する方向に移動量を設定する
		indexSet = mapChipField_->GetMapChipIndexSetByPosition(worldTransform_.translation_ + info.move + Vector3(0, +kHeight / 2.0f, 0));
		// 現在座標が壁の外か判定
		MapChipField::IndexSet indexSetNow;
		indexSetNow = mapChipField_->GetMapChipIndexSetByPosition(worldTransform_.translation_ + Vector3(0, +kHeight / 2.0f, 0));
		if (indexSetNow.yIndex != indexSet.yIndex) 
		{
			// めり込み先ブロックの範囲矩形
			MapChipField::Rect rect = mapChipField_->GetRectByIndex(indexSet.xIndex, indexSet.yIndex);
			info.move.y = std::max(0.0f, rect.bottom - worldTransform_.translation_.y - (kHeight / 2.0f + kBlank));
			info.ceiling = true;
		}
	}
}

// マップ衝突判定下方向
void Player::CheckMapCollisionDown(CollisionMapInfo& info) 
{
	// 下降あり？
	if (info.move.y >= 0) 
	{
		return;
	}

	// 移動後の4つの角の計算
	std::array<Vector3, kNumCorner> positionsNew{};

	for (uint32_t i = 0; i < positionsNew.size(); ++i) 
	{
		positionsNew[i] = CornerPosition(worldTransform_.translation_ + info.move, static_cast<Corner>(i));
	}

	MapChipType mapChipType;
	MapChipType mapChipTypeNext;
	// 真下の当たり判定を行う
	bool hit = false;
	// 左下点の判定
	MapChipField::IndexSet indexSet;
	indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kLeftBottom]);
	mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
	mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex - 1);
	// 隣接セルがともにブロックであればヒット
	if (mapChipType == MapChipType::kBlock && mapChipTypeNext != MapChipType::kBlock) 
	{
		hit = true;
	}
	// 右下点の判定
	indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kRightBottom]);
	mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
	mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex - 1);
	if (mapChipType == MapChipType::kBlock && mapChipTypeNext != MapChipType::kBlock) 
	{
		hit = true;
	}

	// ブロックにヒット？
	if (hit) 
	{
		// めり込みを排除する方向に移動量を設定する
		indexSet = mapChipField_->GetMapChipIndexSetByPosition(worldTransform_.translation_ + info.move + Vector3(0, -kHeight / 2.0f, 0));
		// 現在座標が壁の外か判定
		MapChipField::IndexSet indexSetNow;
		indexSetNow = mapChipField_->GetMapChipIndexSetByPosition(worldTransform_.translation_ + Vector3(0, -kHeight / 2.0f, 0));
		if (indexSetNow.yIndex != indexSet.yIndex) 
		{
			// めり込み先ブロックの範囲矩形
			MapChipField::Rect rect = mapChipField_->GetRectByIndex(indexSet.xIndex, indexSet.yIndex);
			info.move.y = std::min(0.0f, rect.top - worldTransform_.translation_.y + (kHeight / 2.0f + kBlank));
			// 地面に当たったことを記録する
			info.landing = true;
		}
	}
}

void Player::UpdateOnGround(const CollisionMapInfo& info) 
{
	// 自キャラが接地状態？
	if (onGround_) 
	{
		// ジャンプ開始(スライド18）
		if (velocity_.y > 0.0f) 
		{
			onGround_ = false;
		} 
		else 
		{

			// 移動後の4つの角の座標(スライド19）
			std::array<Vector3, kNumCorner> positionsNew{};

			for (uint32_t i = 0; i < positionsNew.size(); ++i) 
			{
				//現在位置で接地確認する
				positionsNew[i] = CornerPosition(worldTransform_.translation_, static_cast<Corner>(i));
			}
			MapChipType mapChipType;
			// 真下の当たり判定を行う
			bool hit = false;
			// 左下点の判定
			MapChipField::IndexSet indexSet;
			indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kLeftBottom] + Vector3(0.0f, -kGroundSearchHeight, 0.0f));
			mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
			if (mapChipType == MapChipType::kBlock) 
			{
				hit = true;
			}

			// 右下点の判定
			indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kRightBottom] + Vector3(0.0f, -kGroundSearchHeight, 0.0f));
			mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
			if (mapChipType == MapChipType::kBlock) 
			{
				hit = true;
			}

			// 落下開始
			if (!hit) 
			{
				// 空中状態に切り替える
				onGround_ = false;

				// 足場から落ちた場合は地上ジャンプを使用済みにする
				if (jumpCount_ == 0) 
				{
					jumpCount_ = 1;
				}
			}
		}

	} 
	else 
	{
		// 着地フラグ
		//  接地フラグ(スライド16）
		if (info.landing) 
		{
			// 接地状態に切り替える（落下を止める）
			onGround_ = true;
			// 着地したため二段ジャンプの回数を戻す
			jumpCount_ = 0;
			// 着地したため空中ダッシュを再使用可能にする
			airDashUsed_ = false;
			// 接地時にX速度を減衰
			velocity_.x *= (1.0f - kAttenuationLanding);
			// Y速度をゼロにする
			velocity_.y = 0.0f;
		}
	}
}

// 02_08スライド27枚目 壁接地中の処理
void Player::UpdateOnWall(const CollisionMapInfo& info) 
{

	if (info.hitWall) 
	{
		velocity_.x *= (1.0f - kAttenuationWall);
	}
}

// マップ衝突判定右方向
void Player::CheckMapCollisionRight(CollisionMapInfo& info) 
{
	// 右移動あり？
	if (info.move.x <= 0) 
	{
		return;
	}

	// 移動後の4つの角の計算
	std::array<Vector3, kNumCorner> positionsNew{};

	for (uint32_t i = 0; i < positionsNew.size(); ++i) 
	{
		positionsNew[i] = CornerPosition(worldTransform_.translation_ + info.move, static_cast<Corner>(i));
	}

	MapChipType mapChipType;
	MapChipType mapChipTypeNext;
	// 右の当たり判定を行う
	bool hit = false;
	// 右上点の判定
	MapChipField::IndexSet indexSet;
	indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kRightTop]);
	mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
	mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex - 1, indexSet.yIndex);
	if (mapChipType == MapChipType::kBlock && mapChipTypeNext != MapChipType::kBlock) 
	{
		hit = true;
	}
	// 右下点の判定
	indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kRightBottom]);
	mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
	mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex - 1, indexSet.yIndex);
	if (mapChipType == MapChipType::kBlock && mapChipTypeNext != MapChipType::kBlock) 
	{
		hit = true;
	}

	// ブロックにヒット？
	if (hit) 
	{
		// めり込みを排除する方向に移動量を設定する
		indexSet = mapChipField_->GetMapChipIndexSetByPosition(worldTransform_.translation_ + info.move + Vector3(+kWidth / 2.0f, 0, 0));
		// 現在座標が壁の外か判定
		MapChipField::IndexSet indexSetNow;
		indexSetNow = mapChipField_->GetMapChipIndexSetByPosition(worldTransform_.translation_ + Vector3(+kWidth / 2.0f, 0, 0));
		if (indexSetNow.xIndex != indexSet.xIndex) 
		{
			// めり込み先ブロックの範囲矩形
			MapChipField::Rect rect = mapChipField_->GetRectByIndex(indexSet.xIndex, indexSet.yIndex);
			info.move.x = std::max(0.0f, rect.left - worldTransform_.translation_.x - (kWidth / 2.0f + kBlank));
			info.hitWall = true;
		}
	}
}

// マップチップ衝突判定左方向
void Player::CheckMapCollisionLeft(CollisionMapInfo& info) 
{

	// 左移動あり？
	if (info.move.x >= 0) 
	{
		return;
	}

	// 移動後の4つの角の計算
	std::array<Vector3, kNumCorner> positionsNew{};

	for (uint32_t i = 0; i < positionsNew.size(); ++i) 
	{
		positionsNew[i] = CornerPosition(worldTransform_.translation_ + info.move, static_cast<Corner>(i));
	}

	MapChipType mapChipType;
	MapChipType mapChipTypeNext;
	// 左の当たり判定を行う
	bool hit = false;
	// 左上点の判定
	MapChipField::IndexSet indexSet;
	indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kLeftTop]);
	mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
	mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex + 1, indexSet.yIndex);
	if (mapChipType == MapChipType::kBlock && mapChipTypeNext != MapChipType::kBlock) 
	{
		hit = true;
	}
	// 左下点の判定
	indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kLeftBottom]);
	mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
	mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex + 1, indexSet.yIndex);
	if (mapChipType == MapChipType::kBlock && mapChipTypeNext != MapChipType::kBlock) 
	{
		hit = true;
	}

	// ブロックにヒット？
	if (hit) 
	{
		// めり込みを排除する方向に移動量を設定する
		indexSet = mapChipField_->GetMapChipIndexSetByPosition(worldTransform_.translation_ + info.move + Vector3(-kWidth / 2.0f, 0, 0));
		// 現在座標が壁の外か判定
		MapChipField::IndexSet indexSetNow;
		indexSetNow = mapChipField_->GetMapChipIndexSetByPosition(worldTransform_.translation_ + Vector3(-kWidth / 2.0f, 0, 0));
		if (indexSetNow.xIndex != indexSet.xIndex) 
		{
			// めり込み先ブロックの範囲矩形
			MapChipField::Rect rect = mapChipField_->GetRectByIndex(indexSet.xIndex, indexSet.yIndex);
			info.move.x = std::min(0.0f,rect.right - worldTransform_.translation_.x + (kWidth / 2.0f + kBlank));
			info.hitWall = true;
		}
	}
}

Vector3 Player::CornerPosition(const Vector3& center, Corner corner) 
{

	Vector3 offsetTable[] = {
	    {+kWidth / 2.0f, -kHeight / 2.0f, 0}, //  kRightBottom
	    {-kWidth / 2.0f, -kHeight / 2.0f, 0}, //  kLeftBottom
	    {+kWidth / 2.0f, +kHeight / 2.0f, 0}, //  kRightTop
	    {-kWidth / 2.0f, +kHeight / 2.0f, 0}  //  kLeftTop
	};

	return center + offsetTable[static_cast<uint32_t>(corner)];
}

void Player::Update() 
{
	if (isDead_) { return; }
	constexpr float dt = 1.0f / 60.0f;
	blinkCooldownTimer_ = (std::max)(0.0f, blinkCooldownTimer_ - dt);
	blinkRecoveryTimer_ = (std::max)(0.0f, blinkRecoveryTimer_ - dt);
	if (attackEffectTimer_ > 0.0f)
	{
		slashElapsed_ += dt;
		attackEffectTimer_ = (std::max)(0.0f, kSlashDuration - slashElapsed_);
	}
	trailTimer_ = (std::max)(0.0f, trailTimer_ - dt);

	// ダッシュの再使用待ち時間を減らす
	if (dashCooldownTimer_ > 0.0f)
	{
		dashCooldownTimer_ -= 1.0f / 60.0f;

		if (dashCooldownTimer_ < 0.0f)
		{
			dashCooldownTimer_ = 0.0f;
		}
	}

	// 被弾後の無敵時間を減らす
	if (invincibleTimer_ > 0.0f)
	{
		invincibleTimer_ -= 1.0f / 60.0f;

		if (invincibleTimer_ < 0.0f)
		{
			invincibleTimer_ = 0.0f;
		}
	}

	// 移動入力(02_07 スライド10枚目)
	if (Input::GetInstance()->TriggerKey(DIK_J) &&
	    blinkCooldownTimer_ <= 0.0f && attackEffectTimer_ <= 0.0f && !isDashing_)
	{
		// 同時に方向キーを押した場合はその方向を優先。
		if (Input::GetInstance()->PushKey(DIK_D)) { lrDirection_ = LRDirection::kRight; }
		else if (Input::GetInstance()->PushKey(DIK_A)) { lrDirection_ = LRDirection::kLeft; }
		StartBlinkAttack();
	}
	if (blinkRecoveryTimer_ <= 0.0f)
	{
		InputMove();
	}
	else
	{
		// 振り抜くまで横入力を止める。重力は止めない。
		velocity_.x = 0.0f;
		if (!onGround_)
		{
			// 攻撃中もSPACEを離したときの小ジャンプ制御を維持。
			if (!Input::GetInstance()->PushKey(DIK_SPACE) && velocity_.y > kJumpReleaseSpeed)
			{ velocity_.y = kJumpReleaseSpeed; }
			float gravity = kGravityAcceleration / 60.0f;
			if (velocity_.y <= 0.0f) { gravity *= kFallGravityMultiplier; }
			velocity_.y = (std::max)(velocity_.y - gravity, -kLimitFallSpeed);
		}
	}

	// 移動入力
	// 衝突情報を初期化
	CollisionMapInfo collisionMapInfo = {};
	collisionMapInfo.move = velocity_;
	// 移動量に速度の値をコピー
	//  マップ衝突チェック(02_07 スライド13枚目)
	CheckMapCollision(collisionMapInfo);

	// 移動(02_07 スライド36枚目)
	worldTransform_.translation_ += collisionMapInfo.move;

	// 天井接触による落下開始(02_07 スライド38枚目)
	if (collisionMapInfo.ceiling) 
	{
		velocity_.y = 0;
	}

	// 02_08 スライド27枚目 壁接触している場合の処理
	UpdateOnWall(collisionMapInfo);

	// 接地判定
	UpdateOnGround(collisionMapInfo);

	// ダッシュ時間を減らす
	if (isDashing_)
	{
		dashTimer_ -= 1.0f / 60.0f;

		// ダッシュ中は横長にして速度感を出す
		worldTransform_.scale_ = {
			1.35f,
			0.75f,
			1.0f
		};

		if (dashTimer_ <= 0.0f)
		{
			dashTimer_ = 0.0f;
			isDashing_ = false;
		}
	}
	else
	{
		worldTransform_.scale_ = {
			1.0f,
			1.0f,
			1.0f
		};
	}

	// 旋回制御
	if (turnTimer_ > 0.0f) 
	{
		// タイマーを進める
		turnTimer_ = std::max(turnTimer_ - (1.0f / 60.0f), 0.0f);

		float destinationRotationYTable[] = {std::numbers::pi_v<float> / 2.0f, std::numbers::pi_v<float> * 3.0f / 2.0f};

		float destinationRotationY = destinationRotationYTable[static_cast<uint32_t>(lrDirection_)];

		worldTransform_.rotation_.y = EaseInOut(destinationRotationY, turnFirstRotationY_, turnTimer_ / kTimeTurn);
	}

	UpdateTransform(worldTransform_);
	UpdateBlinkEffects();
}

void Player::Draw() 
{
	if (isDead_) { return; }
	// 剣の本体は通常描画。光・残像はDrawAttackEffectsで描く。
	if (attackEffectTimer_ > 0.0f && slashElapsed_ < kBeamStartTime + 0.05f)
	{
		modelAttack->Draw(swordTransform_, *camera_, &swordColor_);
	}

	// 無敵時間中はプレイヤーを点滅させる
	if (invincibleTimer_ > 0.0f)
	{
		if (static_cast<int>(invincibleTimer_ * 20.0f) % 2 == 0)
		{
			return;
		}
	}

	model_->Draw(worldTransform_, *camera_); 
}

// 02_10 10枚目
Vector3 Player::GetWorldPosition() 
{
	Vector3 worldPos{};
	// ワールド行列の平行移動成分を取得（ワールド座標）
	worldPos.x = worldTransform_.matWorld_.m[3][0];
	worldPos.y = worldTransform_.matWorld_.m[3][1];
	worldPos.z = worldTransform_.matWorld_.m[3][2];
	return worldPos;
}

// 02_10 14枚目
AABB Player::GetAABB() 
{
	Vector3 worldPos = GetWorldPosition();

	AABB aabb{};

	aabb.min = {worldPos.x - kWidth / 2.0f, worldPos.y - kHeight / 2.0f, worldPos.z - kWidth / 2.0f};
	aabb.max = {worldPos.x + kWidth / 2.0f, worldPos.y + kHeight / 2.0f, worldPos.z + kWidth / 2.0f};

	return aabb;
}

AABB Player::GetAttackAABB()
{
	// 斬撃の近接判定。ビームにはダメージ判定を付けない。
	return {
		{slashPosition_.x - 0.75f, slashPosition_.y - 0.5f, slashPosition_.z - 0.5f},
		{slashPosition_.x + 0.75f, slashPosition_.y + 0.5f, slashPosition_.z + 0.5f}
	};
}

bool Player::IsAttack()
{
	return !isDead_ && !attackHit_ &&
	       attackEffectTimer_ > 0.0f && slashElapsed_ >= kWindupTime && slashElapsed_ < kBeamStartTime;
}

bool Player::TryHitBoss(Boss* boss)
{
	if (!boss || boss->IsDead() || !IsAttack()) { return false; }
	const AABB attack = GetAttackAABB();
	const AABB target = boss->GetAABB();
	const bool hit = attack.min.x <= target.max.x && attack.max.x >= target.min.x &&
	                 attack.min.y <= target.max.y && attack.max.y >= target.min.y &&
	                 attack.min.z <= target.max.z && attack.max.z >= target.min.z;
	if (!hit) { return false; }
	const float targetNear = attackDirection_ > 0.0f ? target.min.x : target.max.x;
	const float needed = (std::max)(0.0f, (targetNear - gripPosition_.x) * attackDirection_);
	if (GetBeamWallDistance(gripPosition_, attackDirection_, needed, 0.05f) + 0.001f < needed)
	{ return false; }
	attackHit_ = true;
	boss->OnDamage(1);
	return true;
}

void Player::OnCollision(Boss* boss)
{
	if (!boss)
	{
		return;
	}

	// ボス本体との接触も弾と同じダメージ
	OnBulletCollision();
}

void Player::OnBulletCollision()
{
	// ダッシュ中・被弾後の無敵時間中はダメージを受けない
	if (isDashing_ || blinkRecoveryTimer_ > 0.0f || invincibleTimer_ > 0.0f || isDead_)
	{
		return;
	}

	hp_ -= 1;
	invincibleTimer_ = 1.0f;

	if (hp_ <= 0)
	{
		hp_ = 0;
		isDead_ = true;
	}
}

// 未初期化のUpDateポインタを経由せず、既存Math関数で行列を更新する。
void Player::UpdateTransform(WorldTransform& transform)
{
	transform.matWorld_ = MakeAffineMatrix(transform.scale_, transform.rotation_, transform.translation_);
	transform.TransferMatrix();
}

float Player::GetSafeHorizontalMove(float distance, float halfWidth, float halfHeight)
{
	if (!mapChipField_) { return 0.0f; }
	const Vector3 p = worldTransform_.translation_;
	float allowed = distance;
	// 端点だけでなく経路全体を見るので、薄い壁も飛び越えない。
	for (uint32_t y = 0; y < mapChipField_->GetNumBlockVirtical(); ++y)
	{
		for (uint32_t x = 0; x < mapChipField_->GetNumBlockHorizontal(); ++x)
		{
			if (mapChipField_->GetMapChipTypeByIndex(x, y) != MapChipType::kBlock) { continue; }
			const auto rect = mapChipField_->GetRectByIndex(x, y);
			if (p.y + halfHeight <= rect.bottom || p.y - halfHeight >= rect.top) { continue; }
			if (distance > 0.0f && rect.right > p.x - halfWidth)
			{
				const float gap = rect.left - (p.x + halfWidth) - kBlank;
				allowed = (std::min)(allowed, (std::max)(0.0f, gap));
			}
			else if (distance < 0.0f && rect.left < p.x + halfWidth)
			{
				const float gap = rect.right - (p.x - halfWidth) + kBlank;
				allowed = (std::max)(allowed, (std::min)(0.0f, gap));
			}
		}
	}
	return allowed;
}

void Player::StartBlinkAttack()
{
	const Vector3 start = worldTransform_.translation_;
	attackDirection_ = lrDirection_ == LRDirection::kRight ? 1.0f : -1.0f;
	worldTransform_.rotation_.y = attackDirection_ > 0.0f ?
	    std::numbers::pi_v<float> / 2.0f : std::numbers::pi_v<float> * 1.5f;
	turnTimer_ = 0.0f;
	const float move = GetSafeHorizontalMove(attackDirection_ * kBlinkDistance, kWidth / 2.0f, kHeight / 2.0f);
	worldTransform_.translation_.x += move;
	// 残像は固有のWorldTransformを持ち、移動経路に固定する。
	for (int i = 0; i < kTrailCount; ++i)
	{
		auto& trail = blinkTrails_[i];
		trail.translation_ = start;
		trail.translation_.x += move * static_cast<float>(i) / kTrailCount;
		trail.rotation_ = worldTransform_.rotation_;
	}
	trailTimer_ = std::abs(move) > 0.01f ? kSlashDuration : 0.0f;
	blinkCooldownTimer_ = kBlinkCooldown;
	blinkRecoveryTimer_ = kBlinkRecovery;
	attackEffectTimer_ = kSlashDuration;
	slashElapsed_ = 0.0f;
	beamSpawned_ = false;
	beamLength_ = 0.0f;
	beamMaxLength_ = 0.0f;
	attackHit_ = false;
	velocity_.x = 0.0f;
	// 上昇速度と二段ジャンプ回数は維持する。
	slashPosition_ = worldTransform_.translation_;
	slashPosition_.x += attackDirection_ * 0.9f;
	CollisionMapInfo info{};
	UpdateOnGround(info);
}

void Player::UpdateBlinkEffects()
{
	if (trailTimer_ > 0.0f)
	{
		const float life = trailTimer_ / kSlashDuration;
		for (int i = 0; i < kTrailCount; ++i)
		{
			// 半透明機能を使わず、小さくして消す残像。
			const float size = life * (0.3f + 0.09f * i);
			blinkTrails_[i].scale_ = {size, size, size};
			UpdateTransform(blinkTrails_[i]);
		}
	}
	trailColor_.SetColor({0.1f, 0.65f, 1.0f, 0.35f * trailTimer_ / kSlashDuration});
	UpdateSwordAndBeam();
}

// aからbへ伸びる細長い球。中心の移動と回転を同時に設定する。
void Player::SetEffectSegment(WorldTransform& transform, const Vector3& a, const Vector3& b, float halfThickness, float halfDepth)
{
	const float dx = b.x - a.x;
	const float dy = b.y - a.y;
	const float length = std::sqrt(dx * dx + dy * dy);
	transform.translation_ = {(a.x + b.x) * 0.5f, (a.y + b.y) * 0.5f, (a.z + b.z) * 0.5f};
	transform.rotation_ = {0.0f, 0.0f, std::atan2(dy, dx)};
	transform.scale_ = {(std::max)(length * 0.5f, 0.001f) / kEffectSphereRadius,
	    (std::max)(halfThickness, 0.001f) / kEffectSphereRadius,
	    (std::max)(halfDepth, 0.001f) / kEffectSphereRadius};
	UpdateTransform(transform);
}

float Player::GetBeamWallDistance(const Vector3& origin, float direction, float maximum, float halfHeight)
{
	if (!mapChipField_) { return 0.0f; }
	float length = maximum;
	for (uint32_t y = 0; y < mapChipField_->GetNumBlockVirtical(); ++y)
	{
		for (uint32_t x = 0; x < mapChipField_->GetNumBlockHorizontal(); ++x)
		{
			if (mapChipField_->GetMapChipTypeByIndex(x, y) != MapChipType::kBlock) { continue; }
			const auto rect = mapChipField_->GetRectByIndex(x, y);
			if (origin.y + halfHeight <= rect.bottom || origin.y - halfHeight >= rect.top) { continue; }
			if (direction > 0.0f && rect.right > origin.x)
			{ length = (std::min)(length, (std::max)(0.0f, rect.left - origin.x - kBlank)); }
			else if (direction < 0.0f && rect.left < origin.x)
			{ length = (std::min)(length, (std::max)(0.0f, origin.x - rect.right - kBlank)); }
		}
	}
	return length;
}

void Player::UpdateSwordAndBeam()
{
	if (attackEffectTimer_ <= 0.0f) { return; }
	// 剣を持つ仮の位置。腕のボーンではなく自機座標を基準にする。
	// 振り抜くまでは自機に追従し、ビーム発射後はその位置に固定。
	if (!beamSpawned_)
	{
		gripPosition_ = worldTransform_.translation_;
		gripPosition_.y += 0.15f;
		slashPosition_ = gripPosition_;
		slashPosition_.x += attackDirection_ * 0.9f;
	}
	const float pi = std::numbers::pi_v<float>;
	float angle;
	float swing = 0.0f;
	if (slashElapsed_ < kWindupTime)
	{
		const float t = slashElapsed_ / kWindupTime;
		angle = 0.8f + (2.1f - 0.8f) * t;
	}
	else
	{
		swing = (std::clamp)((slashElapsed_ - kWindupTime) / kSwingTime, 0.0f, 1.0f);
		// 最初に速く振り抜き、最後に減速する。
		const float eased = 1.0f - (1.0f - swing) * (1.0f - swing);
		angle = 2.1f + (-1.0f - 2.1f) * eased;
	}
	const float worldAngle = attackDirection_ > 0.0f ? angle : pi - angle;
	const Vector3 tip = {gripPosition_.x + std::cos(worldAngle) * kSwordLength,
	    gripPosition_.y + std::sin(worldAngle) * kSwordLength, gripPosition_.z};
	SetEffectSegment(swordTransform_, gripPosition_, tip, 0.055f, 0.035f);

	// 細長い球20本を弧に沿って並べて、斬撃の帯を作る。
	// 各区間は別のWorldTransformとObjectColorを持つ。
	const float swept = (std::max)(0.0f, 2.1f - angle);
	const float tailAngle = angle + (std::min)(swept, 1.8f);
	const float fade = 1.0f - (std::clamp)((slashElapsed_ - kBeamStartTime) / 0.10f, 0.0f, 1.0f);
	for (int i = 0; i < kArcSegments; ++i)
	{
		const float t0 = static_cast<float>(i) / kArcSegments;
		const float t1 = static_cast<float>(i + 1) / kArcSegments;
		float a0 = tailAngle + (angle - tailAngle) * t0;
		float a1 = tailAngle + (angle - tailAngle) * t1;
		if (attackDirection_ < 0.0f) { a0 = pi - a0; a1 = pi - a1; }
		const float radius = kSwordLength * 0.9f;
		Vector3 a = {gripPosition_.x + std::cos(a0) * radius, gripPosition_.y + std::sin(a0) * radius, gripPosition_.z};
		Vector3 b = {gripPosition_.x + std::cos(a1) * radius, gripPosition_.y + std::sin(a1) * radius, gripPosition_.z};
		SetEffectSegment(slashArc_[i], a, b, (0.02f + 0.18f * t1) * fade, 0.025f);
		arcColors_[i].SetColor({0.08f + 0.6f * t1, 0.65f + 0.35f * t1, 1.0f, fade * t1});
	}

	// >=で境界を判定する。秒数の完全一致には依存しない。
	if (!beamSpawned_ && slashElapsed_ >= kBeamStartTime)
	{
		beamSpawned_ = true;
		beamStart_ = gripPosition_;
		// 始点を壁の向こうへ置かない。始点から経路全体を確認。
		beamMaxLength_ = GetBeamWallDistance(beamStart_, attackDirection_, kBeamMaxLength, kBeamHalfWidth);
	}
	if (beamSpawned_)
	{
		const float age = slashElapsed_ - kBeamStartTime;
		const float grow = (std::clamp)(age / kBeamGrowTime, 0.0f, 1.0f);
		beamLength_ = beamMaxLength_ * (1.0f - (1.0f - grow) * (1.0f - grow));
		const float life = 1.0f - (std::clamp)((age - kBeamGrowTime) /
		    (kSlashDuration - kBeamStartTime - kBeamGrowTime), 0.0f, 1.0f);
		const Vector3 end = {beamStart_.x + attackDirection_ * beamLength_, beamStart_.y, beamStart_.z};
		SetEffectSegment(beamOuter_, beamStart_, end, kBeamHalfWidth * life, 0.025f);
		SetEffectSegment(beamCore_, beamStart_, end, kBeamHalfWidth * 0.24f * life, 0.025f);
		beamOuterColor_.SetColor({0.0f, 0.5f, 1.0f, 0.7f * life});
		beamCoreColor_.SetColor({0.8f, 1.0f, 1.0f, life});
		slashFlash_.translation_ = beamStart_;
		slashFlash_.scale_ = {0.25f + age * 2.0f, 0.25f + age * 2.0f, 0.04f};
		UpdateTransform(slashFlash_);
		flashColor_.SetColor({0.6f, 0.95f, 1.0f, (std::max)(0.0f, 1.0f - age / 0.07f)});
	}
}

void Player::DrawAttackEffects()
{
	// 外側でModel::PostDraw()を済ませてから呼ぶ。ここで描画を開始・終了する。
	if (isDead_ || (attackEffectTimer_ <= 0.0f && trailTimer_ <= 0.0f)) { return; }
	// 青い外側は通常α合成。白背景でも色を残す。深度には書き込まない。
	Model::PreDraw(Model::CullingMode::kBack, Model::BlendMode::kNormal, Model::DepthTestMode::kReadOnly);
	if (trailTimer_ > 0.0f)
	{ for (auto& trail : blinkTrails_) { model_->Draw(trail, *camera_, &trailColor_); } }
	if (attackEffectTimer_ > 0.0f)
	{
		if (slashElapsed_ >= kWindupTime && slashElapsed_ < kBeamStartTime + 0.10f)
		{ for (int i = 0; i < kArcSegments; ++i) { modelAttack->Draw(slashArc_[i], *camera_, &arcColors_[i]); } }
		if (beamSpawned_ && beamLength_ > 0.001f)
		{ modelAttack->Draw(beamOuter_, *camera_, &beamOuterColor_); }
	}
	Model::PostDraw();
	// 中心の白い線と発射光を加算で重ねる（ブルームではない）。
	if (attackEffectTimer_ > 0.0f && beamSpawned_ && beamLength_ > 0.001f)
	{
		Model::PreDraw(Model::CullingMode::kBack, Model::BlendMode::kAdd, Model::DepthTestMode::kReadOnly);
		modelAttack->Draw(beamCore_, *camera_, &beamCoreColor_);
		if (slashElapsed_ < kBeamStartTime + 0.07f)
		{ modelAttack->Draw(slashFlash_, *camera_, &flashColor_); }
		Model::PostDraw();
	}
}

bool Player::UpdateEntrance(float targetX)
{
	const float remaining = targetX - worldTransform_.translation_.x;
	velocity_.x = (std::clamp)(remaining, -0.08f, 0.08f);
	if (std::abs(velocity_.x) > 0.001f)
	{
		lrDirection_ = velocity_.x > 0.0f ? LRDirection::kRight : LRDirection::kLeft;
		worldTransform_.rotation_.y = velocity_.x > 0.0f ?
		    std::numbers::pi_v<float> / 2.0f : std::numbers::pi_v<float> * 1.5f;
	}
	if (!onGround_)
	{ velocity_.y = (std::max)(velocity_.y - kGravityAcceleration / 60.0f, -kLimitFallSpeed); }
	CollisionMapInfo info{};
	info.move = velocity_;
	CheckMapCollision(info);
	worldTransform_.translation_ += info.move;
	if (info.ceiling) { velocity_.y = 0.0f; }
	UpdateOnWall(info);
	UpdateOnGround(info);
	UpdateTransform(worldTransform_);
	return std::abs(targetX - worldTransform_.translation_.x) < 0.01f && onGround_;
}

void Player::StopEntrance()
{
	velocity_ = {};
	turnTimer_ = 0.0f;
	UpdateTransform(worldTransform_);
}

#define NOMINMAX
#include "Player.h"
#include "MapChipField.h"
#include "Math.h"
#include "UpDate.h"
#include "Boss.h"
#include <algorithm>
#include <cassert>
#include <numbers>

using namespace KamataEngine;

Player::~Player()
{
	delete modelAttack;
}

void Player::Initialize(Model* model, Camera* camera, const Vector3& position) 
{
	assert(model);

	model_ = model;
	camera_ = camera;

	worldTransform_.Initialize();
	worldTransform_.translation_ = position;
	worldTransform_.rotation_.y = std::numbers::pi_v<float> / 2.0f;

	// 攻撃エフェクトには追加素材が不要な球モデルを使用
	modelAttack = Model::CreateSphere(8, 8);
	worldTransformAttack.Initialize();
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
	else if (onGround_) 
	{
		// 地上ではキーを離した瞬間に停止する
		velocity_.x = 0.0f;
	}

	if (!onGround_) 
	{
		// 落下速度
		velocity_ = Add(Vector3(0, -kGravityAcceleration / 60.0f, 0), velocity_);
		velocity_.y = std::max(velocity_.y, -kLimitFallSpeed);
	}

	// スペースを押した瞬間だけジャンプする
	if (Input::GetInstance()->TriggerKey(DIK_SPACE) &&
		jumpCount_ < 2) 
	{
		// 1回目と2回目を同じ上向き速度で開始する
		velocity_.y = kJumpAcceleration / 60.0f;
		onGround_ = false;
		jumpCount_++;
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
				positionsNew[i] = CornerPosition(worldTransform_.translation_ + info.move, static_cast<Corner>(i));
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
			info.move.x = std::max(0.0f, rect.right - worldTransform_.translation_.x - (kWidth / 2.0f + kBlank));
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
	// Jキーを押した瞬間に攻撃エフェクトを開始
	if (Input::GetInstance()->TriggerKey(DIK_J))
	{
		attackEffectTimer_ = kAttackEffectDuration;
	}

	if (attackEffectTimer_ > 0.0f)
	{
		attackEffectTimer_ -= 1.0f / 60.0f;

		if (attackEffectTimer_ < 0.0f)
		{
			attackEffectTimer_ = 0.0f;
		}
	}

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
	InputMove();

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

	upDate_->WorldTransformUpDate(worldTransform_);

	// 攻撃判定と同じく、向いている方向へエフェクトを配置
	worldTransformAttack.translation_ = worldTransform_.translation_;

	if (lrDirection_ == LRDirection::kRight)
	{
		worldTransformAttack.translation_.x += 0.9f;
	}
	else
	{
		worldTransformAttack.translation_.x -= 0.9f;
	}

	// 表示中に少し広がる横長のエフェクト
	float effectScale = 1.0f;
	if (attackEffectTimer_ > 0.0f)
	{
		effectScale =
			1.0f -
			attackEffectTimer_ / kAttackEffectDuration * 0.4f;
	}

	worldTransformAttack.scale_ = {
		0.9f * effectScale,
		0.25f * effectScale,
		0.25f * effectScale
	};

	worldTransformAttack.rotation_.z += 0.35f;
	upDate_->WorldTransformUpDate(worldTransformAttack);
}

void Player::Draw() 
{
	// 攻撃中だけ球を横長にしたエフェクトを表示
	if (attackEffectTimer_ > 0.0f)
	{
		modelAttack->Draw(worldTransformAttack, *camera_);
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
	Vector3 worldPosition = GetWorldPosition();

	// 攻撃判定の中心
	Vector3 attackPosition = worldPosition;

	// 向いている方向へ攻撃判定を移動
	if (lrDirection_ == LRDirection::kRight)
	{
		attackPosition.x += 0.9f;
	} 
	else
	{
		attackPosition.x -= 0.9f;
	}

	// 攻撃判定の大きさ
	const float attackHalfWidth = 0.75f;
	const float attackHalfHeight = 0.5f;
	const float attackHalfDepth = 0.5f;

	AABB attackAABB = {};

	attackAABB.min = {
		attackPosition.x - attackHalfWidth,
		attackPosition.y - attackHalfHeight,
		attackPosition.z - attackHalfDepth
	};

	attackAABB.max = {
		attackPosition.x + attackHalfWidth,
		attackPosition.y + attackHalfHeight,
		attackPosition.z + attackHalfDepth
	};

	return attackAABB;
}

bool Player::IsAttack()
{
	// Jキーを押した瞬間だけ攻撃状態
	if (Input::GetInstance()->TriggerKey(DIK_J))
	{
		return true;
	}
	return false;
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
	if (isDashing_ || invincibleTimer_ > 0.0f || isDead_)
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

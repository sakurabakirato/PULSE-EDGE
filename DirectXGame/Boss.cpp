#include "Boss.h"
#include "UpDate.h"
#include <algorithm>
#include <cassert>
#include <numbers>

using namespace KamataEngine;

void Boss::Initialize(Model* model, Camera* camera, MapChipField* mapChipField, const Vector3& position)
{
	assert(model);

	// 02_09 7枚目
	model_ = model;
	// 02_09 7枚目
	camera_ = camera;

	mapChipField_ = mapChipField;

	// 02_09 7枚目
	worldTransform_.Initialize();
	worldTransform_.translation_ = position;
	// 02_09 7枚目 角度調整
	worldTransform_.rotation_.y = std::numbers::pi_v<float> *3.0f / 2.0f;

	// 02_09 16枚目
	velocity_ = { -kWalkSpeed, 0, 0 };

	isFacingRight_ = false;

	// 02_09 20枚目
	walkTimer = 0.0f;


	// ---- AABB 初期化 ----
	const float halfSize = 0.5f; // ボスの大きさに応じて調整
	aabb_.min = { position.x - halfSize, position.y - halfSize, position.z - halfSize };
	aabb_.max = { position.x + halfSize, position.y + halfSize, position.z + halfSize };

}

// 02_09 スライド5枚目
void Boss::Update() 
{
	// 変更リクエストがあったら
	if (behaviorRequest != Behavior::kUnknown)
	{
		// 振る舞いを変更する
		behavior = behaviorRequest;
		// 各振る舞いごとの初期化を実行
		switch (behavior)
		{
		case Behavior::kDefeated:
			counter = 0.0f;
			break;
		default:
			break;
		}
		// 振る舞いを変更する
		behavior = behaviorRequest;
	}

	switch (behavior)
	{
	case Behavior::kWalk: // 歩行
		// 移動
		worldTransform_.translation_ += velocity_;
		// 回転アニメーション
		walkTimer = 1.0f / 60.0f;
		//worldTransform_.rotation_.x = std::sin(std::numbers::piv<float> *2.0f * walkTimer / kWalkMotionTime);
		upDate_->WorldTransformUpDate(worldTransform_);
		break;

	case Behavior::kDefeated: // やられ
		counter += 1.0f / 60.0f;
		worldTransform_.rotation_.y += 0.3f;
		//worldTransform.rotation.x = EaseOut(ToRadians(kDefeatedMotionAngleStart), ToRadians(kDefeatedMotionAngleEnd), counter / kDefeatedTime);
		upDate_->WorldTransformUpDate(worldTransform_);
		if (counter >= kDefeatedTime)
		{
			isDead = true;
		}
		break;
	}

	// 壁判定
	CheckWallAndReverse();

	// ==== ブロック落下攻撃タイマー ====
	blockAttackTimer_++;
	if (blockAttackTimer_ > 180)  // 3秒に1回発射
	{
		canBlockAttack_ = true;
		blockAttackTimer_ = 0;
	}

}

// 02_10 スライド14枚目
AABB Boss::GetAABB() 
{
	Vector3 worldPos = GetWorldPosition();

	AABB aabb{};

	aabb.min = { worldPos.x - kWidth / 2.0f, worldPos.y - kHeight / 2.0f, worldPos.z - kWidth / 2.0f };
	aabb.max = { worldPos.x + kWidth / 2.0f, worldPos.y + kHeight / 2.0f, worldPos.z + kWidth / 2.0f };

	return aabb;
}


void Boss::CheckWallAndReverse() 
{
	// 現在位置からAABBを取得
	AABB aabb = GetAABB();

	// ボスの「足元」のタイルを調べる
	Vector3 pos = GetWorldPosition();

	// タイル座標に変換
	MapChipField::IndexSet index = mapChipField_->GetMapChipIndexSetByPosition(pos);

	// 左側のタイル
	if (velocity_.x < 0) 
	{
		if (mapChipField_->GetMapChipTypeByIndex(index.xIndex - 1, index.yIndex) == MapChipType::kBlock) 
		{
			velocity_.x = +kWalkSpeed;
			isFacingRight_ = true;
		}
	}

	// 右側のタイル
	if (velocity_.x > 0) 
	{
		if (mapChipField_->GetMapChipTypeByIndex(index.xIndex + 1, index.yIndex) == MapChipType::kBlock) 
		{
			velocity_.x = -kWalkSpeed;
			isFacingRight_ = false;
		}
	}

	// モデルの向きを方向に合わせる
	worldTransform_.rotation_.y = isFacingRight_
		? std::numbers::pi_v<float> / 2.0f
		: std::numbers::pi_v<float> *3.0f / 2.0f;
}

void Boss::OnDamage(int amount)
{
	hp_ -= amount;
	if (hp_ == 0) 
	{
		isDead = true;
	}
}

// 02_10 スライド14枚目
Vector3 Boss::GetWorldPosition()
{

	Vector3 worldPos{};

	// ワールド行列の平行移動成分を取得（ワールド座標）
	worldPos.x = worldTransform_.matWorld_.m[3][0];
	worldPos.y = worldTransform_.matWorld_.m[3][1];
	worldPos.z = worldTransform_.matWorld_.m[3][2];

	return worldPos;
}

// 02_10 スライド20枚目
void Boss::OnCollision(Player* player)
{
	(void)player;

	// desu
	if (behavior == Behavior::kDefeated)
	{
		return;
	}
	isDead = true;
	if (player->IsAttack())
	{

		if (gameScene == 1)
		{
			//敵と自キャラの中間位置にエフェクトを生成
			//Vector3 effectPos = ((GetWorldPosition() + player_->GetWorldPosition())/2.0f);
		}
		behaviorRequest = Behavior::kDefeated;
		isCollisionDisabled = true;
	}
}

// 02_09 スライド5枚目
void Boss::Draw()
{
	// 02_09 スライド9枚目  モデル描画
	if (isDead == 0)
	{
		model_->Draw(worldTransform_, *camera_);
	} 
	else
	{

	}
}
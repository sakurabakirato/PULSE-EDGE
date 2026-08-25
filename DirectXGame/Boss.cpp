#include "Boss.h"
#include <cmath>
#include <algorithm>
#include <cassert>
#include <numbers>

using namespace KamataEngine;

void Boss::Initialize(Model* model, Model* bulletModel, Camera* camera, MapChipField* mapChipField, Player* player, const Vector3& position)
{
	assert(model);
	assert(bulletModel);
	assert(player);

	model_ = model;
	camera_ = camera;
	mapChipField_ = mapChipField;
	bulletModel_ = bulletModel;
	player_ = player;

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

	// 左上の足場
	movePositions_[0] =
		mapChipField_->GetMapChipPositionByIndex(17, 10);

	// 左下の床
	movePositions_[1] =
		mapChipField_->GetMapChipPositionByIndex(10, 18);

	// 右下の床
	movePositions_[2] =
		mapChipField_->GetMapChipPositionByIndex(89, 18);

	// 右上の足場
	movePositions_[3] =
		mapChipField_->GetMapChipPositionByIndex(82, 10);

	movePositionIndex_ = 0;
	moveWaitTimer_ = 0.0f;

	attackType_ = 0;
	attackDone_ = false;

}

void Boss::Attack()
{
	Vector3 bossPosition = worldTransform_.translation_;
	Vector3 playerPosition = player_->GetWorldPosition();

	// 1. ボスを中心に円形に広がる攻撃
	if (attackType_ == 0)
	{
		// 1周あたりの弾数
		const int bulletCount = 24;

		// 円形弾を2重に発射
		const int circleCount = 2;

		for (int circle = 0; circle < circleCount; ++circle)
		{
			// 1周目は遅く、2周目は少しだけ速くする
			float bulletSpeed =
				0.07f + static_cast<float>(circle) * 0.015f;

			for (int i = 0; i < bulletCount; ++i)
			{
				float angle =
					std::numbers::pi_v<float> *2.0f *
					static_cast<float>(i) /
					static_cast<float>(bulletCount);

				// 2周目の角度を少しずらす
				if (circle == 1)
				{
					angle +=
						std::numbers::pi_v<float> /
						static_cast<float>(bulletCount);
				}

				bullets_.emplace_back();
				Bullet& bullet = bullets_.back();

				bullet.worldTransform.Initialize();
				bullet.worldTransform.translation_ =
					bossPosition;

				bullet.worldTransform.scale_ = {
					0.2f,
					0.2f,
					0.2f
				};

				bullet.velocity.x =
					std::cos(angle) * bulletSpeed;

				bullet.velocity.y =
					std::sin(angle) * bulletSpeed;

				bullet.velocity.z = 0.0f;

				bullet.homing = false;
				bullet.lifeTimer = 0.0f;
			}
		}
	}

	// 2. 複数のホーミング弾
	if (attackType_ == 1)
	{
		const int bulletCount = 5;

		// 弾ごとに発射タイミングを変える
		float launchDelays[bulletCount] = {
			0.00f,
			0.12f,
			0.28f,
			0.46f,
			0.68f
		};

		// 弾ごとに左右の広がりを変える
		float sideSpeeds[bulletCount] = {
			-0.07f,
			-0.035f,
			0.0f,
			0.035f,
			0.07f
		};

		// 弾ごとに上方向の速度を変える
		float upSpeeds[bulletCount] = {
			0.17f,
			0.20f,
			0.18f,
			0.21f,
			0.19f
		};

		for (int i = 0; i < bulletCount; ++i)
		{
			bullets_.emplace_back();
			Bullet& bullet = bullets_.back();

			bullet.worldTransform.Initialize();

			// ボスより少し上から発射
			bullet.worldTransform.translation_ =
				bossPosition;

			bullet.worldTransform.translation_.y += 0.6f;

			bullet.worldTransform.scale_ = {
				0.3f,
				0.3f,
				0.3f
			};

			// 最初は斜め上へ飛ばす
			bullet.velocity.x = sideSpeeds[i];
			bullet.velocity.y = upSpeeds[i];
			bullet.velocity.z = 0.0f;

			bullet.launchDelay = launchDelays[i];
			bullet.lifeTimer = 0.0f;
			bullet.homing = true;
			bullet.exploding = false;
		}
	}

	// 3. プレイヤーへ向かう3列の弾
	if (attackType_ == 2)
	{
		const float bulletSpeed = 0.18f;

		Vector3 direction = {};
		direction.x = playerPosition.x - bossPosition.x;
		direction.y = playerPosition.y - bossPosition.y;

		float distance = std::sqrt(
			direction.x * direction.x +
			direction.y * direction.y);

		if (distance > 0.0f)
		{
			direction.x /= distance;
			direction.y /= distance;
		}

		// 進行方向に対して垂直な方向
		Vector3 sideDirection = {};
		sideDirection.x = -direction.y;
		sideDirection.y = direction.x;

		for (int line = -1; line <= 1; ++line)
		{
			for (int i = 0; i < 5; ++i)
			{
				bullets_.emplace_back();
				Bullet& bullet = bullets_.back();

				bullet.worldTransform.Initialize();

				bullet.worldTransform.translation_.x =
					bossPosition.x +
					sideDirection.x *
					static_cast<float>(line) * 1.2f -
					direction.x *
					static_cast<float>(i) * 0.7f;

				bullet.worldTransform.translation_.y =
					bossPosition.y +
					sideDirection.y *
					static_cast<float>(line) * 1.2f -
					direction.y *
					static_cast<float>(i) * 0.7f;

				bullet.worldTransform.translation_.z =
					bossPosition.z;

				bullet.worldTransform.scale_ = {
					0.25f, 0.25f, 0.25f
				};

				bullet.velocity.x =
					direction.x * bulletSpeed;

				bullet.velocity.y =
					direction.y * bulletSpeed;

				bullet.velocity.z = 0.0f;

				bullet.homing = false;
				bullet.lifeTimer = 0.0f;
			}
		}
	}

	// 次の攻撃へ進める
	attackType_++;

	if (attackType_ >= 3)
	{
		attackType_ = 0;
	}
}

// 02_09 スライド5枚目
void Boss::Update() 
{

	// ボス死亡後は移動と攻撃を更新しない
	if (IsDead())
	{
		return;
	}



	// 振る舞いの変更
	if (behaviorRequest != Behavior::kUnknown)
	{
		behavior = behaviorRequest;

		switch (behavior)
		{
		case Behavior::kDefeated:
			counter = 0.0f;
			break;

		default:
			break;
		}

		behaviorRequest = Behavior::kUnknown;
	}

	switch (behavior)
	{
	case Behavior::kWalk:
		if (moveWaitTimer_ > 0.0f)
		{
			moveWaitTimer_ -= 1.0f / 60.0f;

			// 到着してから約0.5秒後に1回だけ攻撃
			if (moveWaitTimer_ <= 3.5f)
			{
				if (!attackDone_)
				{
					Attack();
					attackDone_ = true;
				}
			}

			if (moveWaitTimer_ < 0.0f)
			{
				moveWaitTimer_ = 0.0f;
			}
		}
		else
		{
			Vector3 targetPosition =
				movePositions_[movePositionIndex_];

			Vector3 difference = {};

			difference.x =
				targetPosition.x - worldTransform_.translation_.x;

			difference.y =
				targetPosition.y - worldTransform_.translation_.y;

			difference.z =
				targetPosition.z - worldTransform_.translation_.z;

			float distance = std::sqrt(
				difference.x * difference.x +
				difference.y * difference.y +
				difference.z * difference.z);

			// 移動先へ到着
			if (distance <= kWalkSpeed)
			{
				worldTransform_.translation_ = targetPosition;
				velocity_ = { 0.0f, 0.0f, 0.0f };

				// 合計4秒間停止
				moveWaitTimer_ = 4.0f;

				// 次の待機中に攻撃できるようにする
				attackDone_ = false;

				// 次の移動先へ進める
				movePositionIndex_++;

				if (movePositionIndex_ >= 4)
				{
					movePositionIndex_ = 0;
				}
			} 
			else
			{
				// 移動先の方向へ一定速度で移動
				velocity_.x =
					difference.x / distance * kWalkSpeed;

				velocity_.y =
					difference.y / distance * kWalkSpeed;

				velocity_.z =
					difference.z / distance * kWalkSpeed;

				worldTransform_.translation_ += velocity_;

				// ボスの向きを移動方向へ合わせる
				if (velocity_.x > 0.0f)
				{
					isFacingRight_ = true;

					worldTransform_.rotation_.y =
						std::numbers::pi_v<float> / 2.0f;
				} 
				else if (velocity_.x < 0.0f)
				{
					isFacingRight_ = false;

					worldTransform_.rotation_.y =
						std::numbers::pi_v<float> *
						3.0f / 2.0f;
				}
			}
		}
		break;

	case Behavior::kDefeated:
		counter += 1.0f / 60.0f;
		worldTransform_.rotation_.y += 0.3f;

		if (counter >= kDefeatedTime)
		{
			isDead = true;
		}
		break;
	}

	// ワールド行列を更新
	worldTransform_.matWorld_ = MakeAffineMatrix(
		worldTransform_.scale_,
		worldTransform_.rotation_,
		worldTransform_.translation_);

	worldTransform_.TransferMatrix();

	// ボスの弾を更新
	for (auto bulletIterator = bullets_.begin();
		bulletIterator != bullets_.end();)
	{
		Bullet& bullet = *bulletIterator;

		bool eraseBullet = false;

		// まだ発射時刻になっていない
		if (bullet.launchDelay > 0.0f)
		{
			bullet.launchDelay -= 1.0f / 60.0f;

			if (bullet.launchDelay < 0.0f)
			{
				bullet.launchDelay = 0.0f;
			}

			++bulletIterator;
			continue;
		}

		// 爆発していない弾
		if (!bullet.exploding)
		{
			// 発射後0.35秒は上昇し、その後1.45秒だけ追尾
			if (bullet.homing &&
				bullet.lifeTimer >= 0.35f &&
				bullet.lifeTimer < 1.8f)
			{

				Vector3 playerPosition =
					player_->GetWorldPosition();

				Vector3 direction = {};

				direction.x =
					playerPosition.x -
					bullet.worldTransform.translation_.x;

				direction.y =
					playerPosition.y -
					bullet.worldTransform.translation_.y;

				float distance = std::sqrt(
					direction.x * direction.x +
					direction.y * direction.y);

				if (distance > 0.0f)
				{
					direction.x /= distance;
					direction.y /= distance;

					// 現在の弾速を維持
					float homingSpeed = std::sqrt(
						bullet.velocity.x *
						bullet.velocity.x +
						bullet.velocity.y *
						bullet.velocity.y);

					// 少しずつプレイヤー方向へ曲げる
					bullet.velocity.x =
						bullet.velocity.x * 0.96f +
						direction.x *
						homingSpeed * 0.04f;

					bullet.velocity.y =
						bullet.velocity.y * 0.96f +
						direction.y *
						homingSpeed * 0.04f;

					float velocityLength = std::sqrt(
						bullet.velocity.x *
						bullet.velocity.x +
						bullet.velocity.y *
						bullet.velocity.y);

					if (velocityLength > 0.0f)
					{
						bullet.velocity.x =
							bullet.velocity.x /
							velocityLength *
							homingSpeed;

						bullet.velocity.y =
							bullet.velocity.y /
							velocityLength *
							homingSpeed;
					}
				}
			}
		
			// 移動前の位置を保存
			Vector3 previousPosition =
				bullet.worldTransform.translation_;

			// 弾を移動
			bullet.worldTransform.translation_ +=
				bullet.velocity;

			// ホーミング弾だけ地形との接触を確認
			if (bullet.homing)
			{
				MapChipField::IndexSet index =
					mapChipField_->
					GetMapChipIndexSetByPosition(
						bullet.worldTransform.translation_);

				MapChipType mapChipType =
					mapChipField_->
					GetMapChipTypeByIndex(
						index.xIndex,
						index.yIndex);

				// 地面または足場に着弾
				if (mapChipType == MapChipType::kBlock)
				{
					// ブロックの中へ入らないように戻す
					bullet.worldTransform.translation_ =
						previousPosition;

					bullet.velocity = {
						0.0f,
						0.0f,
						0.0f
					};

					// 爆発開始
					bullet.exploding = true;

					// 同じタイマーを爆発時間に再利用
					bullet.lifeTimer = 0.0f;

					bullet.worldTransform.scale_ = {
						0.3f,
						0.3f,
						0.3f
					};
				}
			}
		} 
        else
		{
			// 爆発中は球を大きくする
			bullet.worldTransform.scale_.x += 0.06f;
			bullet.worldTransform.scale_.y += 0.06f;
			bullet.worldTransform.scale_.z += 0.06f;
		}

		bullet.lifeTimer += 1.0f / 60.0f;

		// ワールド行列を更新
		bullet.worldTransform.matWorld_ =
			MakeAffineMatrix(
				bullet.worldTransform.scale_,
				bullet.worldTransform.rotation_,
				bullet.worldTransform.translation_);

		bullet.worldTransform.TransferMatrix();

		// 通常弾の当たり判定サイズ
		float bulletHalfSize = 0.25f;

		// 爆発中は見た目に合わせて当たり判定を拡大
		if (bullet.exploding)
		{
			bulletHalfSize =
				bullet.worldTransform.scale_.x;
		}

		AABB bulletAABB = {};

		bulletAABB.min = {
			bullet.worldTransform.translation_.x -
				bulletHalfSize,
			bullet.worldTransform.translation_.y -
				bulletHalfSize,
			bullet.worldTransform.translation_.z -
				bulletHalfSize
		};

		bulletAABB.max = {
			bullet.worldTransform.translation_.x +
				bulletHalfSize,
			bullet.worldTransform.translation_.y +
				bulletHalfSize,
			bullet.worldTransform.translation_.z +
				bulletHalfSize
		};

		// 弾または爆発とプレイヤーの判定
		if (IsCollision(
			bulletAABB,
			player_->GetAABB()))
		{
			player_->OnBulletCollision();
			eraseBullet = true;
		}

		if (bullet.exploding)
		{
			// 爆発は0.4秒で終了
			if (bullet.lifeTimer >= 0.4f)
			{
				eraseBullet = true;
			}
		} 
		else
		{
			// 通常状態の弾は8秒で消す
			if (bullet.lifeTimer >= 8.0f)
			{
				eraseBullet = true;
			}
		}

		if (eraseBullet)
		{
			bulletIterator =
				bullets_.erase(bulletIterator);
		} 
		else
		{
			++bulletIterator;
		}
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

void Boss::OnDamage(int amount)
{
	hp_ -= amount;

	if (hp_ <= 0)
	{
		hp_ = 0;
		isDead = true;

		// ボス撃破時に残っている弾を消す
		bullets_.clear();
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

	if (!player)
	{
		return;
	}

	if (IsDead())
	{
		return;
	}

	if (player->IsAttack())
	{
		OnDamage(1);
	}
}

// 02_09 スライド5枚目
void Boss::Draw()
{
	// 02_09 スライド9枚目  モデル描画
	if (!isDead)
	{
		model_->Draw(worldTransform_, *camera_);
	} 

	for (Bullet& bullet : bullets_)
	{
		// 発射時刻になっていない弾は表示しない
		if (bullet.launchDelay > 0.0f)
		{
			continue;
		}

		bulletModel_->Draw(
			bullet.worldTransform,
			*camera_);
	}
}
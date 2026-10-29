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

	worldTransform_.Initialize();
	renderTransform_.Initialize();
	bodyColor_.Initialize();
	hitReactionTimer_ = 0.0f;
	hp_ = kMaxHp;
	isDead = false;
	bullets_.clear();
	worldTransform_.translation_ = position;
	// 角度調整
	worldTransform_.rotation_.y = std::numbers::pi_v<float> *3.0f / 2.0f;

	velocity_ = { -kWalkSpeed, 0, 0 };

	isFacingRight_ = false;

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

	introPoseActive_ = false;
	introPoseTime_ = 0.0f;
	introShockVisible_.fill(false);
	for (auto& transform : introShockTransforms_) { transform.Initialize(); }
	for (auto& color : introShockColors_) { color.Initialize(); }
	// 待機中も最初のフレームから正しい位置で描画する。
	UpdateIntroMatrix();
	ChangeAction(Action::kDashReady);

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

				bullet.worldTransform.scale_ = 
				{
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
		float launchDelays[bulletCount] = 
		{
			0.00f,
			0.12f,
			0.28f,
			0.46f,
			0.68f
		};

		// 弾ごとに左右の広がりを変える
		float sideSpeeds[bulletCount] = 
		{
			-0.07f,
			-0.035f,
			0.0f,
			0.035f,
			0.07f
		};

		// 弾ごとに上方向の速度を変える
		float upSpeeds[bulletCount] = 
		{
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

			bullet.worldTransform.scale_ = 
			{
				0.45f,
				0.45f,
				0.45f
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

	// 4.狙い撃ち連射
	if (attackType_ == 3)
	{

	}


	// 次の攻撃へ進める
	attackType_++;

	if (attackType_ >= 4)
	{
		attackType_ = 0;
	}
}

void Boss::ChangeAction(Action next)
{
	action_ = next;
	actionTimer_ = 0.0f;
	velocity_ = {};

	// 予備動作に入った瞬間だけ、突進方向を決める
	if (action_ == Action::kDashReady)
	{
		const Vector3 playerPosition =
			player_->GetWorldPosition();

		dashDirectionX_ =
			playerPosition.x >= worldTransform_.translation_.x
			? 1.0f
			: -1.0f;

		// プレイヤー側を向く
		worldTransform_.rotation_.y =
			dashDirectionX_ > 0.0f
			? std::numbers::pi_v<float> / 2.0f
			: std::numbers::pi_v<float> *3.0f / 2.0f;
	}
}

void Boss::UpdateAction()
{
	actionTimer_ += kDeltaTime;

	switch (action_)
	{
	case Action::kDashReady:
		UpdateDashReady();
		break;

	case Action::kDash:
		UpdateDash();
		break;

	case Action::kRecovery:
		UpdateRecovery();
		break;
	}
}

void Boss::UpdateDashReady()
{
	// この段階では移動しない。
   // 構える姿勢などの表示演出も、後でここに対応させる。

	if (actionTimer_ >= kDashReadyTime)
	{
		ChangeAction(Action::kDash);
	}
}

void Boss::UpdateDash()
{
	const float moveX =
		dashDirectionX_ * kDashSpeed * kDeltaTime;

	worldTransform_.translation_.x += moveX;

	// 実装時はここに壁との衝突判定が必要。
	// 壁に当たったら位置を補正し、
	// ChangeAction(Action::kRecovery)で突進を終了する。

	if (actionTimer_ >= kDashTime)
	{
		ChangeAction(Action::kRecovery);
	}
}

void Boss::UpdateRecovery()
{
	// この間もダメージは受け付ける。
    // 移動と新しい攻撃は行わない。

	if (actionTimer_ >= kRecoveryTime)
	{
		ChangeAction(Action::kDashReady);
	}
}

void Boss::Update() 
{

	// ボス死亡後は移動と攻撃を更新しない
	if (IsDead())
	{
		return;
	}

	// 被弾演出の時間を更新
	UpdateHitReaction();

	// 現在の行動を更新
	UpdateAction();

	// 移動結果を当たり判定用の行列へ反映
	worldTransform_.matWorld_ = MakeAffineMatrix(
		worldTransform_.scale_,
		worldTransform_.rotation_,
		worldTransform_.translation_);

	worldTransform_.TransferMatrix();

	// 被弾時の揺れなどを表示用の位置へ反映
	UpdateRenderTransform();

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

				// 下降中にブロックの上面へ触れた場合だけ着弾
				if (mapChipType == MapChipType::kBlock &&
					bullet.velocity.y < 0.0f)
				{
					MapChipField::Rect blockRect =
						mapChipField_->GetRectByIndex(
							index.xIndex,
							index.yIndex);

					// 横から壁へ入った場合は通過させる
					if (previousPosition.y >= blockRect.top)
					{
						// ブロックの上面へ着弾位置を合わせる
						bullet.worldTransform.translation_.y =
							blockRect.top + 0.25f;

						bullet.velocity = 
						{
							0.0f,
							0.0f,
							0.0f
						};

						// 爆発開始
						bullet.exploding = true;

						// 同じタイマーを爆発時間に再利用
						bullet.lifeTimer = 0.0f;

						bullet.worldTransform.scale_ = 
						{
							0.45f,
							0.45f,
							0.45f
						};
					}
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

		bulletAABB.min = 
		{
			bullet.worldTransform.translation_.x -
				bulletHalfSize,
			bullet.worldTransform.translation_.y -
				bulletHalfSize,
			bullet.worldTransform.translation_.z -
				bulletHalfSize
		};

		bulletAABB.max = 
		{
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
	// 0以下のダメージや、撃破後の追加ヒットは受け付けない。
	if (amount <= 0 || IsDead()) 
	{
		return; 
	}

	if (amount >= hp_)
	{
		hp_ = 0;
		isDead = true;
		hitReactionTimer_ = 0.0f;
		bullets_.clear();
	}
	else
	{
		hp_ -= amount;
		// 被弾するたびに演出を先頭から再生。行動は中断しない。
		hitReactionTimer_ = kHitReactionDuration;
	}
	// Update後に当たり判定があっても、そのフレームから色を反映する。
	UpdateRenderTransform();
}

void Boss::UpdateHitReaction()
{
	hitReactionTimer_ = (std::max)(0.0f, hitReactionTimer_ - 1.0f / 60.0f);
}

void Boss::UpdateRenderTransform()
{
	// GPUリソースを含むWorldTransform全体ではなく、座標・姿勢だけコピー。
	renderTransform_.translation_ = worldTransform_.translation_;
	renderTransform_.rotation_ = worldTransform_.rotation_;
	renderTransform_.scale_ = worldTransform_.scale_;
	bodyColor_.SetColor({1.0f, 1.0f, 1.0f, 1.0f});

	if (hitReactionTimer_ > 0.0f)
	{
		const float elapsed = kHitReactionDuration - hitReactionTimer_;
		const float strength = hitReactionTimer_ / kHitReactionDuration;
		// 約3往復しながら揺れを弱める。毎回元の位置から計算する。
		renderTransform_.translation_.x += std::sin(
			elapsed / kHitReactionDuration * 6.0f * std::numbers::pi_v<float>)
			* kHitShakeAmplitude * strength;
		const int flashIndex = static_cast<int>(elapsed / kHitFlashInterval);
		if (flashIndex % 2 == 0)
		{
			bodyColor_.SetColor({1.0f, 0.25f, 0.25f, 1.0f});
		}
	}
	renderTransform_.matWorld_ = MakeAffineMatrix(
		renderTransform_.scale_, renderTransform_.rotation_, renderTransform_.translation_);
	renderTransform_.TransferMatrix();
}

Vector3 Boss::GetWorldPosition()
{

	Vector3 worldPos{};

	// ワールド行列の平行移動成分を取得（ワールド座標）
	worldPos.x = worldTransform_.matWorld_.m[3][0];
	worldPos.y = worldTransform_.matWorld_.m[3][1];
	worldPos.z = worldTransform_.matWorld_.m[3][2];

	return worldPos;
}

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

void Boss::Draw()
{
	// 02_09 スライド9枚目  モデル描画
	if (!isDead)
	{
		model_->Draw(renderTransform_, *camera_, &bodyColor_);
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

void Boss::UpdateIntroMatrix()
{
	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);
	worldTransform_.TransferMatrix();
	UpdateRenderTransform();
}

void Boss::BeginIntroPose()
{
	introBasePosition_ = worldTransform_.translation_;
	introBaseRotation_ = worldTransform_.rotation_;
	introBaseScale_ = worldTransform_.scale_;
	introPoseTime_ = 0.0f;
	introPoseActive_ = true;
	introShockVisible_.fill(false);
}

bool Boss::UpdateIntroPose()
{
	if (!introPoseActive_) { return true; }
	introPoseTime_ = (std::min)(introPoseTime_ + 1.0f / 60.0f, kIntroPoseDuration);
	const float t = introPoseTime_;
	// 0～0.45秒で浮く。1.25秒から元の位置・姿勢へ戻す。
	float amount = (std::clamp)(t / 0.45f, 0.0f, 1.0f);
	if (t > 1.25f) { amount = 1.0f - (std::clamp)((t - 1.25f) / 0.55f, 0.0f, 1.0f); }
	amount = amount * amount * (3.0f - 2.0f * amount);
	const float nod = t >= 0.45f && t <= 1.25f ?
	    std::sin((t - 0.45f) / 0.8f * 4.0f * std::numbers::pi_v<float>) : 0.0f;
	worldTransform_.translation_ = introBasePosition_;
	worldTransform_.translation_.y += kIntroLiftHeight * amount + nod * 0.035f;
	worldTransform_.rotation_ = introBaseRotation_;
	// 単一モデルなので、頭の代わりに全体をローカルX軸で軽くうなずかせる。
	worldTransform_.rotation_.x += kIntroTiltAngle * amount + nod * 0.045f;
	UpdateIntroMatrix();
	for (int wave = 0; wave < 2; ++wave)
	{
		const float age = t - (0.55f + 0.38f * wave);
		introShockVisible_[wave] = age >= 0.0f && age < 0.55f;
		if (!introShockVisible_[wave]) { continue; }
		const float progress = age / 0.55f;
		const float radius = 0.35f + (kShockMaxRadius - 0.35f) * progress;
		introShockColors_[wave].SetColor({0.08f, 0.65f, 1.0f, (1.0f - progress) * 0.8f});
		for (int i = 0; i < kShockSegments; ++i)
		{
			const float angle = 2.0f * std::numbers::pi_v<float> * (i + 0.5f) / kShockSegments;
			auto& transform = introShockTransforms_[wave * kShockSegments + i];
			transform.translation_ = introBasePosition_;
			transform.translation_.x += std::cos(angle) * radius;
			transform.translation_.y += 0.45f + std::sin(angle) * radius;
			transform.translation_.z -= 0.15f;
			transform.rotation_ = {0.0f, 0.0f, angle + std::numbers::pi_v<float> / 2.0f};
			transform.scale_ = {radius * std::sin(std::numbers::pi_v<float> / kShockSegments) * 1.15f,
			    0.035f * (1.0f - progress) + 0.008f, 0.025f};
			transform.matWorld_ = MakeAffineMatrix(transform.scale_, transform.rotation_, transform.translation_);
			transform.TransferMatrix();
		}
	}
	if (t >= kIntroPoseDuration) { EndIntroPose(); return true; }
	return false;
}

void Boss::EndIntroPose()
{
	if (introPoseActive_)
	{
		worldTransform_.translation_ = introBasePosition_;
		worldTransform_.rotation_ = introBaseRotation_;
		worldTransform_.scale_ = introBaseScale_;
		UpdateIntroMatrix();
	}
	introPoseActive_ = false;
	introShockVisible_.fill(false);
}

void Boss::DrawIntroShockwaves()
{
	if (!introPoseActive_) { return; }
	Model::PreDraw(Model::CullingMode::kBack, Model::BlendMode::kNormal, Model::DepthTestMode::kReadOnly);
	for (int wave = 0; wave < 2; ++wave)
	{
		if (!introShockVisible_[wave]) { continue; }
		for (int i = 0; i < kShockSegments; ++i)
		{ bulletModel_->Draw(introShockTransforms_[wave * kShockSegments + i], *camera_, &introShockColors_[wave]); }
	}
	Model::PostDraw();
}

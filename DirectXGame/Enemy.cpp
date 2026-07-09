#include "Enemy.h"
#include "MapChipField.h"
#include "Math.h"
#include "UpData.h"
#include <algorithm>
#include <cassert>
#include <numbers>

using namespace KamataEngine;

void Enemy::Initialize(Model* model, Camera* camera, const Vector3& position) {

	assert(model);

	// 02_09 7枚目
	model_ = model;
	// 02_09 7枚目
	camera_ = camera;
	// 02_09 7枚目
	worldTransform_.Initialize();
	worldTransform_.translation_ = position;
	// 02_09 7枚目 角度調整
	worldTransform_.rotation_.y = std::numbers::pi_v<float> * 3.0f / 2.0f;

	// 02_09 16枚目
	velocity_ = {-kWalkSpeed, 0, 0};
	// 02_09 20枚目
	walkTimer = 0.0f;
}

// 02_09 スライド5枚目
void Enemy::UpDate() {

	// 02_09 16枚目 移動
	worldTransform_.translation_ += velocity_;

	// 02_09 20枚目
	walkTimer += 1.0f / 60.0f;

	// 02_09 23枚目 回転アニメーション
	// worldTransform_.rotation_.x = std::sin(std::numbers::pi_v<float> * 2.0f * walkTimer / kWalkMotionTime);

	float param = std::sin(std::numbers::pi_v<float> * 2.0f * walkTimer / kWalkMotionTime);

	float degree = kWalkMotionAngleStart + kWalkMotionAngleEnd * (param + 1.0f) / 2.0f;

	worldTransform_.rotation_.x = degree * (std::numbers::pi_v<float> / 180.0f);

	// 02_09 スライド8枚目 ワールド行列更新
	upData->WorldTransformUpData(worldTransform_);
}

// 02_09 スライド5枚目
void Enemy::Draw() {
	// 02_09 スライド9枚目  モデル描画
	model_->Draw(worldTransform_, *camera_);
}

// 02_10 スライド14枚目
AABB Enemy::GetAABB() {

	Vector3 worldPos = GetWorldPosition();

	AABB aabb;

	aabb.min = {worldPos.x - kWidth / 2.0f, worldPos.y - kHeight / 2.0f, worldPos.z - kWidth / 2.0f};
	aabb.max = {worldPos.x + kWidth / 2.0f, worldPos.y + kHeight / 2.0f, worldPos.z + kWidth / 2.0f};

	return aabb;
}

// 02_10 スライド14枚目
Vector3 Enemy::GetWorldPosition() {

	Vector3 worldPos;

	// ワールド行列の平行移動成分を取得（ワールド座標）
	worldPos.x = worldTransform_.matWorld_.m[3][0];
	worldPos.y = worldTransform_.matWorld_.m[3][1];
	worldPos.z = worldTransform_.matWorld_.m[3][2];

	return worldPos;
}

// 02_10 スライド20枚目
void Enemy::OnCollision(const Player* player) { (void)player; }
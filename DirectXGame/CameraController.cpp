#define NOMINMAX
#include "CameraController.h"
#include "Math.h"
#include "Player.h"
#include <algorithm>

using namespace KamataEngine;

void CameraController::Initialize(Camera* camera) { camera_ = camera; }

void CameraController::Updata() {
	// 追従対象のワールドトランスフォームを参照
	const WorldTransform& targetWorldTransform = target_->GetWorldTransform();

	const Vector3& targetVelocity = target_->GetVelocity();

	// 追従対象とオフセットからカメラの目標座標を計算
	targetCoordinates_ = targetWorldTransform.translation_ + targetOffset_ + targetVelocity * kVelocityBias;

	// 座標補間によりゆったり追従
	camera_->translation_ = Lerp(camera_->translation_, targetCoordinates_, kInterpolationRate);

	// 追従対象とオフセットからカメラの座標を計算
	// camera_->translation_ = operator+(targetWorldTransform.translation_, targetOffset_);

	// 追従対象が画面外に出ないように補正
	camera_->translation_.x = std::max(camera_->translation_.x, targetCoordinates_.x + targetMargin.left);
	camera_->translation_.x = std::min(camera_->translation_.x, targetCoordinates_.x + targetMargin.right);
	camera_->translation_.y = std::max(camera_->translation_.y, targetCoordinates_.y + targetMargin.bottom);
	camera_->translation_.y = std::min(camera_->translation_.y, targetCoordinates_.y + targetMargin.top);

	// 移動範囲制限
	camera_->translation_.x = std::max(camera_->translation_.x, movableArea_.left);
	camera_->translation_.x = std::min(camera_->translation_.x, movableArea_.right);
	camera_->translation_.y = std::min(camera_->translation_.y, movableArea_.bottom);
	camera_->translation_.y = std::max(camera_->translation_.y, movableArea_.top);

	// 行列を更新
	camera_->UpdateMatrix();
}

void CameraController::Reset() {

	const WorldTransform& targetWorldTransform = target_->GetWorldTransform();

	camera_->translation_ = operator+(targetWorldTransform.translation_, targetOffset_);
}

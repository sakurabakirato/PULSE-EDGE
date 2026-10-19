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
	camera_->translation_.y = (std::clamp)(camera_->translation_.y,
	    (std::min)(movableArea_.bottom, movableArea_.top),
	    (std::max)(movableArea_.bottom, movableArea_.top));

	// 行列を更新
	camera_->UpdateMatrix();
}

void CameraController::Reset() {

	const WorldTransform& targetWorldTransform = target_->GetWorldTransform();

	camera_->translation_ = operator+(targetWorldTransform.translation_, targetOffset_);
}

Vector3 CameraController::GetFollowPosition() const
{
	Vector3 p = target_->GetWorldTransform().translation_ + targetOffset_;
	p.x = (std::clamp)(p.x, movableArea_.left, movableArea_.right);
	const float lower = (std::min)(movableArea_.bottom, movableArea_.top);
	const float upper = (std::max)(movableArea_.bottom, movableArea_.top);
	p.y = (std::clamp)(p.y, lower, upper);
	return p;
}

void CameraController::FollowEntrance()
{
	camera_->translation_ = target_->GetWorldTransform().translation_ + targetOffset_;
	camera_->UpdateMatrix();
}

void CameraController::BeginIntroMove(const Vector3& destination, float duration)
{
	introMoveStart_ = camera_->translation_;
	introMoveEnd_ = destination;
	introMoveTime_ = 0.0f;
	introMoveDuration_ = (std::max)(duration, 0.001f);
}

bool CameraController::UpdateIntroMove()
{
	introMoveTime_ = (std::min)(introMoveTime_ + 1.0f / 60.0f, introMoveDuration_);
	const float t = introMoveTime_ / introMoveDuration_;
	const float eased = t * t * (3.0f - 2.0f * t);
	camera_->translation_ = Lerp(introMoveStart_, introMoveEnd_, eased);
	camera_->UpdateMatrix();
	return introMoveTime_ >= introMoveDuration_;
}

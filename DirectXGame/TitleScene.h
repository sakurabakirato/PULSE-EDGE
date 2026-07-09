#pragma once

#include "KamataEngine.h"
#include "UpData.h"
#include "Fade.h"

using namespace KamataEngine;

class TitleScene {
public:
	~TitleScene();

	void Initialize();

	void Update();

	void Draw();

	// 02_12 26枚目
	bool IsFinished() const { return finished_; }

	enum class Phase {
		kFadeIn,
		kMain,
		kFadeOut,
	};

private:
	static inline const float kTimeTitleMove = 2.0f;

	// ビュープロジェクション
	Camera camera_;
	WorldTransform worldTransformTitle_;
	WorldTransform worldTransformPlayer_;

	Model* modelPlayer_ = nullptr;
	Model* modelTitle_ = nullptr;

	UpData* upData = nullptr;

	// ワールド変換データ
	WorldTransform worldTransform_;

	float counter_ = 0.0f;
	// 02_12 26枚目
	bool finished_ = false;

	Fade* fade_ = nullptr;

	Phase phase_ = Phase::kFadeIn;
};
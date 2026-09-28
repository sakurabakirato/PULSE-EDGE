#pragma once
#include "Fade.h"
#include <KamataEngine.h>

using namespace KamataEngine;

class TitleScene
{
public:
	~TitleScene();

	void Initialize();
	void Update();
	void Draw();

	bool IsFinished() const { return finished_; }

	enum class Phase
	{
		kFadeIn,
		kMain,
		kFadeOut,
	};

private:
	
	// ビュープロジェクション
	Camera camera_;
	WorldTransform worldTransformTitle_;

	Model* modelTitle_ = nullptr;

	bool finished_ = false;
	Fade* fade_ = nullptr;
	Phase phase_ = Phase::kFadeIn;
};

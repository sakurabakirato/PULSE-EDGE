#include "TitleScene.h"

TitleScene::~TitleScene()
{
	delete modelTitle_;
	delete fade_;
}

void TitleScene::Initialize()
{
	finished_ = false;
	phase_ = Phase::kFadeIn;

	modelTitle_ = Model::CreateFromOBJ("titleFont", true);

	// カメラ初期化
	camera_.Initialize();

	const float kPlayerTitle = 2.0f;

	worldTransformTitle_.Initialize();
	worldTransformTitle_.scale_ = { kPlayerTitle, kPlayerTitle, kPlayerTitle };

	fade_ = new Fade();
	fade_->Initialize();
	fade_->Start(Fade::Status::FadeIn, 1.0f);
}

void TitleScene::Update()
{
	switch (phase_)
	{
	case Phase::kFadeIn:
		fade_->Update();
		if (fade_->IsFinished())
		{
			phase_ = Phase::kMain;
		}
		break;

	case Phase::kMain:
		// スペースキーを押した瞬間にゲーム開始
		if (Input::GetInstance()->TriggerKey(DIK_SPACE))
		{
			phase_ = Phase::kFadeOut;
			fade_->Start(Fade::Status::FadeOut, 1.0f);
		}
		break;

	case Phase::kFadeOut:
		fade_->Update();
		if (fade_->IsFinished())
		{
			finished_ = true;
		}
		break;
	}
}

void TitleScene::Draw()
{
	modelTitle_->Draw(worldTransformTitle_, camera_);

	fade_->Draw();
}

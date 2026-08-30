#include "TitleScene.h"

TitleScene::~TitleScene()
{
	delete titleSprite_;
	delete fade_;
}

void TitleScene::Initialize()
{
	finished_ = false;
	phase_ = Phase::kFadeIn;

	// 1280x720のタイトル画像を画面全体に表示
	titleTextureHandle_ = TextureManager::Load("title.png");
	titleSprite_ = Sprite::Create(
		titleTextureHandle_,
		{0.0f, 0.0f});
	titleSprite_->SetAnchorPoint({0.0f, 0.0f});
	titleSprite_->SetSize({1280.0f, 720.0f});

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
	Sprite::PreDraw(
		DirectXCommon::GetInstance()->GetCommandList());

	titleSprite_->Draw();

	Sprite::PostDraw();

	fade_->Draw();
}

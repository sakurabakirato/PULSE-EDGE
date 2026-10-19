#include "GameClear.h"
#include "Math.h"
#include <numbers>

GameClear::~GameClear() 
{
	delete modelSpace_;
	delete modelClear_;
	delete fade_;
}

void GameClear::Initialize() 
{
	camera_.Initialize();

	modelClear_ = Model::CreateFromOBJ("GameClear", true);

	skydome_ = new Skydome();
	modelSkydome_ = Model::CreateFromOBJ("skyDome", true);
	skydome_->Initialize(modelSkydome_, &camera_);

	// サウンドデータの読み込み
	BGMHandle = Audio::GetInstance()->LoadWave("sound/1123.mp3");
	voiceHandle = Audio::GetInstance()->PlayWave(BGMHandle, true, 0.1f);

	// カメラ初期化
	camera_.Initialize();

	const float kPlayerClear = 2.0f;

	worldTransformClear_.Initialize();

	worldTransformClear_.scale_ = {kPlayerClear, kPlayerClear, kPlayerClear};

	const float kSpaceScale = 3.0f;

	worldTransformSpace_.Initialize();

	worldTransformSpace_.scale_ = {kSpaceScale, kSpaceScale, kSpaceScale};

	finished_ = false;

	phase_ = Phase::kFadeIn;

	fade_ = new Fade();
	fade_->Initialize();

	// 閉じたシャッターを開いて画面を見せる
	fade_->Start(Fade::Status::FadeIn, 0.35f);
}


void GameClear::Update() 
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
		if (Input::GetInstance()->TriggerKey(DIK_SPACE)) 
		{
			Audio::GetInstance()->StopWave(voiceHandle);

			phase_ = Phase::kFadeOut;
			fade_->Start(Fade::Status::FadeOut, 0.25f);
		}
		break;

	case Phase::kFadeOut:
		fade_->Update();

		if (fade_->IsFinished()) 
		{
			// 完全に覆ってからタイトルへ戻る
			finished_ = true;
		}
		break;
	}
	counter_ += 1.0f / 60.0f;
	counter_ = std::fmod(counter_, kTimeClearMove);

	float angle = counter_ / kTimeClearMove * 2.0f * std::numbers::pi_v<float>;

	worldTransformClear_.translation_.y = std::sin(angle) + 10.0f;

	camera_.TransferMatrix();

	upDate_->WorldTransformUpDate(worldTransformClear_);

	// アフィン変換～DirectXに転送(タイトル座標)
	upDate_->WorldTransformUpDate(worldTransformSpace_);

	//  skydome生成
	skydome_->Update();
}

void GameClear::Draw() 
{
	Model::PreDraw();

	// 天球描画
	skydome_->Draw();

	modelClear_->Draw(worldTransformClear_, camera_);
	/*modelSpace_->Draw(worldTransformSpace_, camera_);*/

	Model::PostDraw();

	fade_->Draw();
}

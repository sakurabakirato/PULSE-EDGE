#include "GameClear.h"
#include "Math.h"
#include <numbers>

GameClear::~GameClear() 
{
	delete modelSpace_;
	delete modelClear_;
}

void GameClear::Initialize() 
{
	camera_.Initialize();

	modelClear_ = Model::CreateFromOBJ("GameClear", true);
	/*modelSpace_ = Model::CreateFromOBJ("space");*/

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

	// worldTransformPlayer_.rotation_.y = 0.95f * std::numbers::pi_v<float>;

	// worldTransformPlayer_.translation_.x = -2.0f;

	// worldTransformPlayer_.translation_.y = -10.0f;
}


void GameClear::Update() 
{

	if (Input::GetInstance()->TriggerKey(DIK_SPACE))
	{
		Audio::GetInstance()->StopWave(voiceHandle);
		finished_ = true;
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
}

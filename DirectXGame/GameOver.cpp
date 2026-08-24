#include "GameOver.h"
#include "Math.h"
#include <numbers>

GameOver::~GameOver() 
{
	delete modelSpace_;
	delete modelOver_;
}

void GameOver::Initialize() 
{

	modelOver_ = Model::CreateFromOBJ("GameOver", true);
	/*modelSpace_ = Model::CreateFromOBJ("space");*/

	skydome_ = new Skydome();
	modelSkydome_ = Model::CreateFromOBJ("skyDome", true);
	skydome_->Initialize(modelSkydome_, &camera_);

	// サウンドデータの読み込み
	BGMHandle = Audio::GetInstance()->LoadWave("sound/1123.mp3");
	voiceHandle = Audio::GetInstance()->PlayWave(BGMHandle, true, 0.1f);

	// カメラ初期化
	camera_.Initialize();

	const float kPlayerOver = 2.0f;

	worldTransformOver_.Initialize();
	worldTransformOver_.scale_ = { kPlayerOver, kPlayerOver, kPlayerOver };

	const float kSpaceScale = 3.0f;

	worldTransformSpace_.Initialize();
	worldTransformSpace_.scale_ = { kSpaceScale, kSpaceScale, kSpaceScale };
}

void GameOver::Update() 
{
	// 02_12 27枚目
	if (Input::GetInstance()->TriggerKey(DIK_SPACE) && Input::GetInstance()->PushKey(DIK_SPACE)) 
	{
		// Gameの音を止める
		Audio::GetInstance()->StopWave(voiceHandle);
		finished_ = true;
	}

	counter_ += 1.0f / 60.0f;
	counter_ = std::fmod(counter_, kTimeOverMove);

	float angle = counter_ / kTimeOverMove * 2.0f * std::numbers::pi_v<float>;

	worldTransformOver_.translation_.y = std::sin(angle) + 10.0f;

	camera_.TransferMatrix();

	upDate_->WorldTransformUpDate(worldTransformOver_);
	// アフィン変換～DirectXに転送(タイトル座標)
	upDate_->WorldTransformUpDate(worldTransformSpace_);
	//  skydome生成
	skydome_->Update();
}

void GameOver::Draw() 
{
	Model::PreDraw();

	// 天球描画
	skydome_->Draw();

	modelOver_->Draw(worldTransformOver_, camera_);
	
	/*modelSpace_->Draw(worldTransformSpace_, camera_);*/

	Model::PostDraw();
}
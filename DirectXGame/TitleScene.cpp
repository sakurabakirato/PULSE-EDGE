#include "TitleScene.h"
#include <cmath>
#include <algorithm>
#include <numbers>

TitleScene::~TitleScene()
{
	delete modelTitle_;
	delete sprite_;
	delete backgroundSprite_;
	delete fade_;
}

void TitleScene::Initialize()
{
	finished_ = false;
	phase_ = Phase::kFadeIn;

	//3Dモデルの生成
	modelTitle_ = Model::CreateFromOBJ("title", true);

	// 背景画像を読み込む
	backgroundTextureHandle_ = TextureManager::Load("titleBackground.png");

	//ファイル名を指定して2Dテクスチャを読み込む
	textureHandle_ = TextureManager::Load("PUSHSPACE.png");

	// スタートSEを読み込む
	startSEHandle_ = KamataEngine::Audio::GetInstance()->LoadWave("sound/accent.mp3");

	// 画面左上に配置
	backgroundSprite_ = Sprite::Create(backgroundTextureHandle_, { 0.0f, 0.0f });

	backgroundSprite_->SetAnchorPoint({ 0.0f, 0.0f });

	// 画面全体のサイズにする
	backgroundSprite_->SetSize({ 1280.0f, 720.0f });

	// 元の左上位置(450, 500)とサイズ(400, 70)から求めた中心
	sprite_ = Sprite::Create(textureHandle_, { 650.0f, 535.0f });

	// 中央を基準に拡大する
	sprite_->SetAnchorPoint({ 0.5f, 0.5f });

	// 通常サイズ
	sprite_->SetSize(pushSpaceBaseSize_);

	// 演出時間をリセット
	startElapsed_ = 0.0f;

	// カメラ初期化
	camera_.Initialize();
	//カメラを手前に近づけさせる
	camera_.translation_.z = -1.0f;

	const float kTitleSize = 2.0f;

	//ワールド変換の初期化
	worldTransformTitle_.Initialize();
	worldTransformTitle_.scale_ = { kTitleSize, kTitleSize, kTitleSize };


	//登場演出を最初から始める
	titleElapsed_ = 0.0f;

	//右の画面外に配置
	worldTransformTitle_.translation_.x = titleStartX_;
	

	fade_ = new Fade();
	fade_->Initialize();
	fade_->Start(Fade::Status::FadeIn, 1.0f);
}

void TitleScene::Update()
{
	// 固定60fps
	const float deltaTime = 1.0f / 60.0f;

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
		// SPACEを押した瞬間、リアクションを開始
		if (Input::GetInstance()->TriggerKey(DIK_SPACE))
		{
			// SEを1回再生する
			KamataEngine::Audio::GetInstance()->PlayWave(
				startSEHandle_,
				false
			);

			startElapsed_ = 0.0f;
			phase_ = Phase::kStartReaction;
		}
		break;

	case Phase::kStartReaction:
	{
		startElapsed_ += deltaTime;

		float t = std::clamp(
			startElapsed_ / startDuration_,
			0.0f,
			1.0f
		);

		// 1.0倍 → 1.15倍 → 1.0倍
		float scale =
			1.0f + 0.15f * std::sin(t * std::numbers::pi_v<float>);

		sprite_->SetSize({
			pushSpaceBaseSize_.x * scale,
			pushSpaceBaseSize_.y * scale
			});

		// リアクション終了後にフェードアウト
		if (t >= 1.0f)
		{
			sprite_->SetSize(pushSpaceBaseSize_);

			phase_ = Phase::kFadeOut;
			fade_->Start(Fade::Status::FadeOut, 1.0f);
		}
		break;
	}

	case Phase::kFadeOut:
		fade_->Update();

		if (fade_->IsFinished())
		{
			finished_ = true;
		}
		break;
	}

	// ① 時間を進める
	// 今回は固定60fpsを前提とする
	titleElapsed_ += deltaTime;

	// ② 進行率を0～1で求める
	float t = std::clamp(titleElapsed_ / titleDuration_, 0.0f, 1.0f);

	// ③ 少し行き過ぎて戻る動きに変換する（EaseOutBack）
	const float overshoot = 1.70158f;
	float u = t - 1.0f;

	float ease = 1.0f + (overshoot + 1.0f) * u * u * u + overshoot * u * u;

	// ④ 開始位置から終了位置への移動に反映する
	worldTransformTitle_.translation_.x = titleStartX_ + (titleEndX_ - titleStartX_) * ease;


	counter_ += 1.0f / 60.0f;
	counter_ = std::fmod(counter_, kTimeTitleMove);

	float angle = counter_ / kTimeTitleMove * 2.0f * std::numbers::pi_v<float>;

	worldTransformTitle_.translation_.y = std::sin(angle) + 10.0f;

	// 行列を計算して、描画側へ転送する
	WorldTransformUpdate(worldTransformTitle_);


}

void TitleScene::Draw()
{

	// ① 背景
	Sprite::PreDraw();

	backgroundSprite_->Draw();

	Sprite::PostDraw();

	//深度バッファだけをクリアする
	DirectXCommon::GetInstance()->ClearDepthBuffer();

	// 3Dモデルの描画準備
	Model::PreDraw();

	modelTitle_->Draw(worldTransformTitle_, camera_);

	// 3Dモデルの描画終了
	Model::PostDraw();

	//スプライト描画前処理
	Sprite::PreDraw();

	sprite_->Draw();

	//スプライト描画後処理
	Sprite::PostDraw();

	fade_->Draw();
}

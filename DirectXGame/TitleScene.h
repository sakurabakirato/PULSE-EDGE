#pragma once
#include "Fade.h"
#include "Update.h"
#include "Math.h"
#include <KamataEngine.h>

using namespace KamataEngine;
/// <summary>
/// タイトルシーン
/// </summary>
class TitleScene
{
public:

	//デストラクタ
	~TitleScene();
	//初期化
	void Initialize();
	//更新
	void Update();
	//描画
	void Draw();
	//デスフラグのgetter
	bool IsFinished() const { return finished_; }

	enum class Phase
	{
		kFadeIn,
		kMain,
		kStartReaction, //SPACEを押した後の演出
		kFadeOut,
	};

private:

	//PUSHSPACE用テクスチャ
	uint32_t textureHandle_ = 0;

	//スプライト
	Sprite* sprite_ = nullptr;

	// ビュープロジェクション
	Camera camera_;

	// ワールド変換データ
	WorldTransform worldTransformTitle_;

	//3Dモデル
	Model* modelTitle_ = nullptr;

	// 登場演出の経過時間
	float titleElapsed_ = 0.0f;

	// 登場にかける時間（秒）
	float titleDuration_ = 0.8f;

	// タイトルの開始位置と終了位置
	float titleStartX_ = 40.0f;
	float titleEndX_ = 0.0f;

	static inline const float kTimeTitleMove = 2.0f;
	float counter_ = 0.0f;

	// SPACEを押した後の演出時間
	float startElapsed_ = 0.0f;
	float startDuration_ = 0.3f;

	// PUSH SPACEの通常サイズ
	KamataEngine::Vector2 pushSpaceBaseSize_ = { 400.0f, 70.0f };

	//終了フラグ
	bool finished_ = false;

	Fade* fade_ = nullptr;
	Phase phase_ = Phase::kFadeIn;


};

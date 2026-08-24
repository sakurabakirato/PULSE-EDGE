#pragma once
#include "Skydome.h"


#include "KamataEngine.h"
#include "UpDate.h"

using namespace KamataEngine;


class GameClear {

public:
	~GameClear();

	void Initialize();

	void Update();

	void Draw();

	// 02_12 26枚目
	bool IsFinished() const { return finished_; }
	

private:
	static inline const float kTimeClearMove = 2.0f;

	// ビュープロジェクション
	Camera camera_;
	WorldTransform worldTransformClear_;
	WorldTransform worldTransformSpace_;

	// Model* modelPlayer_ = nullptr;
	Model* modelClear_ = nullptr;
	Model* modelSpace_ = nullptr;

	Skydome* skydome_ = nullptr;
	UpDate* upDate_ = nullptr;

	Model* modelSkydome_ = nullptr;

	// ワールド変換データ
	WorldTransform worldTransform_;

	float counter_ = 0.0f;
	// 02_12 26枚目
	bool finished_ = false;

	//BGM
	uint32_t BGMHandle = 0;
	uint32_t voiceHandle = 0;

};

#pragma once
#include "Skydome.h"

#include "KamataEngine.h"
#include "UpDate.h"

using namespace KamataEngine;

class GameOver 
{
public:
	~GameOver();

	void Initialize();

	void Update();

	void Draw();

	// 02_12 26枚目
	bool IsFinished() const { return finished_; }
	Skydome* skydome_ = nullptr;
private:
	static inline const float kTimeOverMove = 2.0f;

	// ビュープロジェクション
	Camera camera_;
	WorldTransform worldTransformOver_;
	WorldTransform worldTransformSpace_;
	

	Model* modelSpace_ = nullptr;
	Model* modelOver_ = nullptr;

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
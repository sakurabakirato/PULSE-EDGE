#pragma once
#include "CameraController.h"
#include "DeathParticles.h"
#include "Enemy.h"
#include "MapChipField.h"
#include "Player.h"
#include "Skydome.h"
#include "Fade.h"
#include <KamataEngine.h>

using namespace KamataEngine;

// ゲームシーン
class GameScene {
public:
	// 初期化
	void Initialize();

	// 更新
	void Update();

	// 描画
	void Draw();

	~GameScene();

	void GenerateBlocks();

	// 02_10 16枚目 衝突判定と応答
	void CheckAllCollisions();

	// 02_12 9枚目
	void ChangePhase();

	// 02_12 26枚目	デスフラグのgetter
	bool IsFinished() const { return finished_; }

private:
	enum class Phase {
		kFadeIn,
		kPlay, // ゲームプレイ
		kDeath, // デス演出
		kFadeOut,
	};

	// 02_12 4枚目 ゲームの現在フェーズ（変数）
	Phase phase_;

	////テクスチャーハンドル
	uint32_t textureHandle_ = 0;

	Sprite* sprite_ = nullptr;

	//////3Dモデル
	Model* model_ = nullptr;

	// ブロックの3Dモデル
	Model* blockModel_ = nullptr;

	WorldTransform worldTransform_;

	////カメラ
	Camera camera_;

	std::vector<std::vector<WorldTransform*>> worldTransformBlocks_;

	DebugCamera* debugCamera_ = nullptr;

	// 自キャラ
	Player* player_ = nullptr;

	// 02_09 10枚目 エネミークラス
	Enemy* enemy_ = nullptr;

	// Math* math_ = nullptr;

	// デバッグカメラ有効
	bool isDebugCameraActive_ = false;

	Skydome* skydome_ = nullptr;

	Model* modelSkydome_ = nullptr;

	Model* modelPlayer_ = nullptr;

	Model* enemy_model_ = nullptr;

	MapChipField* mapChipField_;

	CameraController* CController_ = nullptr;

	std::list<Enemy*> enemies_;

	DeathParticles* deathParticles_ = nullptr;

	// 02_11 16枚目
	Model* deathParticle_model_ = nullptr;

	// 02_12 26枚目
	bool finished_ = false;

	Fade* fade_ = nullptr;
};
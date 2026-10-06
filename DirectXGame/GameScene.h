#pragma once
#include "CameraController.h"
#include "DeathParticles.h"
#include "MapChipField.h"
#include "Player.h"
#include "Boss.h"
#include "Skydome.h"
#include "Fade.h"
#include <KamataEngine.h>

using namespace KamataEngine;

// ゲームシーン
class GameScene 
{
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

	// ボスのHPが0ならゲームクリア
	bool IsFinished() const { return boss_->IsDead(); }
	bool IsDead() const { return player_->IsDead(); }

private:
	enum class Phase 
	{
		kFadeIn,
		kPlay, // ゲームプレイ
		kDeath, // デス演出
		kFadeOut,
	};

	// 02_12 4枚目 ゲームの現在フェーズ（変数）
	Phase phase_;

	// HPバー用テクスチャ
	uint32_t HPtextureHandle_ = 0;

	// 現在HP
	Sprite* sprite_ = nullptr;

	// HPバーの背景
	Sprite* hpBarBack_ = nullptr;

	// プレイヤーHPバー
	Sprite* playerHpBar_ = nullptr;
	Sprite* playerHpBarBack_ = nullptr;

	///////////////////////////3Dモデル////////////////////////
	Model* model_ = nullptr;
	Model* blockModel_ = nullptr;
	Model* modelSkydome_ = nullptr;
	Model* modelPlayer_ = nullptr;
	Model* modelBoss_ = nullptr;
	Model* modelBossBullet_ = nullptr;
	Model* deathParticle_model_ = nullptr;

	WorldTransform worldTransform_;

	////カメラ
	Camera camera_;

	std::vector<std::vector<WorldTransform*>> worldTransformBlocks_;

	DebugCamera* debugCamera_ = nullptr;
	Player* player_ = nullptr;
	Boss* boss_ = nullptr;
	Skydome* skydome_ = nullptr;
	CameraController* CController_ = nullptr;
	DeathParticles* deathParticles_ = nullptr;
	Fade* fade_ = nullptr;

	// デバッグカメラ有効
	bool isDebugCameraActive_ = false;

	MapChipField* mapChipField_;

	// 02_12 26枚目
	bool finished_ = false;

	//BGM
	uint32_t BGMHandle = 0;
	uint32_t voiceHandle = 0;

};

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

	//デストラクタ
	~GameScene();

	// 演出完了後にゲームクリアへ進む
    // 同時に死亡した場合はゲームオーバーを優先
	bool IsFinished() const 
	{
		return finished_ && boss_->IsDead() && !player_->IsDead();
	}

	// 演出完了後にゲームオーバーへ進む
	bool IsDead() const 
	{
		return finished_ && player_->IsDead();
	}

	// タイトルへ戻る要求をシーン管理側へ通知する
	bool IsReturnToTitleRequested() const { return returnToTitleRequested_; }

private:
	bool returnToTitleRequested_ = false;
	bool gameOverSpaceReady_ = false;
	enum class IntroPhase { kPlayerEntry, kCameraToBoss, kBossPose, kCameraReturn };
	IntroPhase introPhase_ = IntroPhase::kPlayerEntry;
	static constexpr float kEntryWalkDistance = 6.0f;
	float introTargetX_ = 0.0f;
	float introEntryTime_ = 0.0f;
	float introStuckTime_ = 0.0f;

	void InitializaHpBars();
	void InitializeGameOverUI();

	void UpdateIntro();
	void UpdatePlay();    //プレイヤー更新、ボス更新、当たり判定、勝敗確認
	void UpdateDeath();   //死亡パーティクルの更新と終了確認
	void UpdateGameOver();//暗くする処理、リスタート入力の確認
	void UpdateHpBars();  //プレイヤーとボスのHPバー更新
	void UpdateCamera();  //通常カメラとデバックカメラの切り替え

	void BeginGameOver(); //ゲームオーバーに入る状態

	void DrawPlay3D();    //ステージ、キャラクターなどの3D描画
	void DrawUI();        //HPバーなどの通常UI描画

	void StartBossCamera();

	void GenerateBlocks();
	//衝突判定と応答
	void CheckAllCollisions();
	void ChangePhase();

	enum class Phase 
	{
		kFadeIn,
		kIntro, // 開始演出
		kPlay, // ゲームプレイ
		kDeath, // デス演出
		kGameOver,
		kFadeOut,
	};

	// ゲームの現在フェーズ（変数）
	Phase phase_;

	// HPバー用テクスチャ
	uint32_t HPtextureHandle_ = 0;

	// ボスHPバー、背景
	Sprite* bossHpBar_ = nullptr;
	Sprite* bossHpBarBack_ = nullptr;

	// プレイヤーHPバー、背景
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

	bool finished_ = false;

	//BGM
	uint32_t BGMHandle = 0;
	uint32_t voiceHandle = 0;

	float gameOverAlpha_ = 0.0f; // 暗幕の透明度
	Sprite* gameOverOverlay_ = nullptr; // 画面を覆う暗幕

	Model* modelGameOver_ = nullptr;
	WorldTransform worldTransformGameOver_;
	Camera gameOverCamera_;

};

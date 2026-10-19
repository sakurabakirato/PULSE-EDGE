#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "GameScene.h"
#include <cmath>

using namespace KamataEngine;

GameScene::~GameScene() 
{
	// ゲームシーンを抜けたら戦闘BGMを停止
	if (voiceHandle != 0)
	{
		Audio::GetInstance()->StopWave(voiceHandle);
		voiceHandle = 0;
	}

	delete sprite_;
	delete hpBarBack_;
	delete playerHpBar_;
	delete playerHpBarBack_;
	delete debugCamera_;
	delete model_;
	delete blockModel_;
	delete skydome_;
	delete modelPlayer_;
	delete player_;
	delete boss_;
	delete modelBoss_;
	delete modelBossBullet_;

	for (std::vector<WorldTransform*>& worldTransformBlockLine : worldTransformBlocks_) 
	{
		for (WorldTransform*& worldTransformBlock : worldTransformBlockLine) 
		{
			delete worldTransformBlock;
		}
	}
	worldTransformBlocks_.clear();

	delete mapChipField_;
	delete deathParticles_;
	delete deathParticle_model_;
	delete fade_;
	delete CController_;
	delete modelSkydome_;
}

void GameScene::Initialize() 
{
	// ゲームシーン中だけ戦闘BGMをループ再生
	BGMHandle = Audio::GetInstance()->LoadWave("sound/Battle-Everlasting.wav");

	voiceHandle = 0; // BGMはカメラが戻って戦闘開始するときに再生

	blockModel_ = Model::CreateFromOBJ("block");

	debugCamera_ = new DebugCamera(1280, 720);

	mapChipField_ = new MapChipField;
	mapChipField_->LoadMapChipCsv("Resources/blocks_csv/blocks.csv");

	//-----------------自キャラ------------------//
	// 自キャラの生成
	player_ = new Player();

	// 座標をマップチップ番号で指定
	Vector3 playerPosition = mapChipField_->GetMapChipPositionByIndex(1, 18);

	modelPlayer_ = Model::CreateFromOBJ("player", true);
	// 自キャラの初期化
	player_->Initialize(modelPlayer_, &camera_, playerPosition);
	// マップチップフィールドの生成と初期化
    // 自キャラの生成と初期化
	player_->SetMapChipField(mapChipField_);

	worldTransform_.Initialize();

	// カメラの初期化
	camera_.Initialize();
	camera_.farZ = 1000.0f;

	// skydome生成
	skydome_ = new Skydome();
	// 初期化
	modelSkydome_ = Model::CreateFromOBJ("skyDome", true);
	skydome_->Initialize(modelSkydome_, &camera_);


	GenerateBlocks();

	CController_ = new CameraController(); // 生成
	CController_->Initialize(&camera_); // 初期化
	CController_->SetTarget(player_); // 追従対象セット
	CController_->Reset(); // リセット

	CameraController::Rect cameraArea = {12.0f, 100 - 12.0f, 6.0f, 6.0f};
	CController_->SetMovableArea(cameraArea);


	//---------------ボスクラス--------------//
	boss_ = new Boss();
	//ボスモデル
	modelBoss_ = Model::CreateFromOBJ("boss");
	// 仮の弾モデル
	modelBossBullet_ = Model::CreateSphere(8, 8);
	//ボスの位置決めてボスクラス初期化
	Vector3 bossPosition = mapChipField_->GetMapChipPositionByIndex(50, 13);// 中央足場の上から出現
	boss_->Initialize(modelBoss_, modelBossBullet_, &camera_, mapChipField_, player_, bossPosition);

	//----------------ボスHPバー----------------//

	// 1×1の白い画像を読み込む
	HPtextureHandle_ = TextureManager::Load("white1x1.png");

	// HPバーの背景
	hpBarBack_ = Sprite::Create(HPtextureHandle_, { 390.0f, 40.0f });
	hpBarBack_->SetAnchorPoint({ 0.0f, 0.0f });
	hpBarBack_->SetSize({ 500.0f, 30.0f });
	hpBarBack_->SetColor({
		0.1f,
		0.1f,
		0.1f,
		1.0f
		});

	// 現在HPを表す赤いバー
	sprite_ = Sprite::Create(HPtextureHandle_, { 390.0f, 40.0f });

	sprite_->SetAnchorPoint({ 0.0f, 0.0f });
	sprite_->SetSize({ 500.0f, 30.0f });
	sprite_->SetColor({
		1.0f,
		0.0f,
		0.0f,
		1.0f
		});

	//----------------プレイヤーHPバー----------------//

	playerHpBarBack_ = Sprite::Create(
		HPtextureHandle_,
		{40.0f, 660.0f });

	playerHpBarBack_->SetAnchorPoint({0.0f, 0.0f});
	playerHpBarBack_->SetSize({250.0f, 20.0f});
	playerHpBarBack_->SetColor({
		0.1f,
		0.1f,
		0.1f,
		1.0f
		});

	playerHpBar_ = Sprite::Create(
		HPtextureHandle_,
		{40.0f, 660.0f });

	playerHpBar_->SetAnchorPoint({0.0f, 0.0f});
	playerHpBar_->SetSize({250.0f, 20.0f});
	playerHpBar_->SetColor({
		0.0f,
		0.8f,
		0.2f,
		1.0f
		});

	//-------------デスパーティクル-----------//
	//モデル読み込み
	deathParticle_model_ = Model::CreateFromOBJ("deathParticle");
	deathParticles_ = nullptr; // 死亡したときだけ生成

	//ゲームプレイフェーズから開始
	phase_ = Phase::kFadeIn;

	//---------------fade------------------//
	fade_ = new Fade();
	fade_->Initialize();
	fade_->Start(Fade::Status::FadeIn, 0.35f);

	finished_ = false;
	introPhase_ = IntroPhase::kPlayerEntry;
	introTargetX_ = playerPosition.x + kEntryWalkDistance;
	introEntryTime_ = 0.0f;
	introStuckTime_ = 0.0f;
	CController_->FollowEntrance();
}

void GameScene::GenerateBlocks() 
{
	// 要素数
	uint32_t numBlockVirtical = mapChipField_->GetNumBlockVirtical();
	uint32_t numBlockHorizontal = mapChipField_->GetNumBlockHorizontal();

	// 要素数を変更する
	worldTransformBlocks_.resize(numBlockVirtical);

	// キューブの生成
	for (uint32_t i = 0; i < numBlockVirtical; ++i) 
	{
		worldTransformBlocks_[i].resize(numBlockHorizontal);
	}
	// ブロックの生成
	for (uint32_t i = 0; i < numBlockVirtical; ++i) 
	{
		for (uint32_t j = 0; j < numBlockHorizontal; ++j) 
		{
			if (mapChipField_->GetMapChipTypeByIndex(j, i) == MapChipType::kBlock) 
			{
				WorldTransform* worldTransform = new WorldTransform();
				worldTransform->Initialize();
				worldTransformBlocks_[i][j] = worldTransform;
				worldTransformBlocks_[i][j]->translation_ = mapChipField_->GetMapChipPositionByIndex(j, i);
			}
		}
	}
}

void GameScene::Update() 
{
	const Phase phaseAtStart = phase_;
	switch (phase_) 
	{
	case Phase::kIntro:
		UpdateIntro();
		break;
	case Phase::kPlay:

		break;
	case Phase::kDeath:
		if (deathParticles_) 
		{
			deathParticles_->Update();

			if (deathParticles_->IsFinished()) 
			{
				phase_ = Phase::kFadeOut;

				fade_->Start(
					Fade::Status::FadeOut,
					0.25f
				);
			}
		}
		break;
	case Phase::kFadeIn:
		fade_->Update();
		if (fade_->IsFinished()) 
		{
			phase_ = Phase::kIntro;
		}
		break;
	case Phase::kFadeOut:
		fade_->Update();

		if (fade_->IsFinished()) 
		{
			// シャッターが完全に閉じた
			finished_ = true;
		}
		break;
	}

	// 天球更新
	skydome_->Update();

	// 通常プレイ中だけキャラクターと当たり判定を更新する
	if (phaseAtStart == Phase::kPlay && phase_ == Phase::kPlay) 
	{
		player_->Update();
		boss_->Update();

		CheckAllCollisions();
		ChangePhase();
	}

	//----------------ボスHPバー更新----------------//
	float hpRatio = static_cast<float>(boss_->GetHp()) / static_cast<float>(boss_->GetMaxHp());

	if (hpRatio < 0.0f)
	{
		hpRatio = 0.0f;
	}

	sprite_->SetSize({ 500.0f * hpRatio, 30.0f });

	float playerHpRatio = static_cast<float>(player_->GetHp()) / 3.0f;

	if (playerHpRatio < 0.0f)
	{
		playerHpRatio = 0.0f;
	}

	playerHpBar_->SetSize({ 250.0f * playerHpRatio, 20.0f });

	// ゲームオーバーまたはボス撃破時に戦闘BGMを停止
	// シーンが切り替わる前の更新で確実に止める
	if ((player_->IsDead() || boss_->IsDead()) && voiceHandle != 0)
	{
		Audio::GetInstance()->StopWave(voiceHandle);
		voiceHandle = 0;
	}
	

	// 演出中はUpdateIntroがカメラを管理。追従で上書きしない。
	if (phase_ == Phase::kPlay)
	{
		if (isDebugCameraActive_)
		{
			debugCamera_->Update();
			camera_.matView = debugCamera_->GetCamera().matView;
			camera_.matProjection = debugCamera_->GetCamera().matProjection;
			camera_.TransferMatrix();
		}
		else { CController_->Updata(); }
	}

	for (std::vector<WorldTransform*>& worldTransformBlockLine : worldTransformBlocks_) 
	{
		for (WorldTransform*& worldTransformBlock : worldTransformBlockLine) 
		{
			if (!worldTransformBlock)
				continue;
			// アフィン変換行列の生成
			worldTransformBlock->matWorld_ = MakeAffineMatrix(worldTransformBlock->scale_, worldTransformBlock->rotation_, worldTransformBlock->translation_);

			// 定数バッファに転送する
			worldTransformBlock->TransferMatrix();
		}
	}



}

void GameScene::Draw() 
{

	Model::PreDraw();

	//ブロックの描画
	for (std::vector<WorldTransform*>& worldTransformBlockLine : worldTransformBlocks_) 
	{
		for (WorldTransform*& worldTransformBlock : worldTransformBlockLine) 
		{
			if (!worldTransformBlock)
				continue;
			blockModel_->Draw(*worldTransformBlock, camera_);
		}
	}
	//自キャラ描画
	if (phase_ == Phase::kPlay || phase_ == Phase::kFadeIn || phase_ == Phase::kIntro) 
	{
		player_->Draw();
	}

	//天球描画
	skydome_->Draw();

	boss_->Draw();

	//デスパーティクルあれば描画
	if (deathParticles_) 
	{
		deathParticles_->Draw();
	}

	Model::PostDraw();

	// 追加：残像・斬撃・ビームを描画
	if (phase_ == Phase::kPlay) { player_->DrawAttackEffects(); }
	boss_->DrawIntroShockwaves();

	//----------------ボスHPバー描画----------------//

	Sprite::PreDraw(DirectXCommon::GetInstance()->GetCommandList());

	if (phase_ != Phase::kIntro && phase_ != Phase::kFadeIn)
	{
		hpBarBack_->Draw();
		sprite_->Draw();
		playerHpBarBack_->Draw();
		playerHpBar_->Draw();
	}

	Sprite::PostDraw();

	fade_->Draw();
}

//自キャラとボスの当たり判定
void GameScene::CheckAllCollisions() 
{
#pragma region 自キャラとボスの当たり判定
	// ボスが死亡している場合は判定しない
	if (boss_->IsDead())
	{
		return;
	}

	// プレイヤー本体とボス本体の当たり判定
	AABB playerAABB = player_->GetAABB();
	AABB bossAABB = boss_->GetAABB();

	if (IsCollision(playerAABB, bossAABB))
	{
		player_->OnCollision(boss_);
	}

	// 命中判定と、一回の攻撃につき一度だけのダメージ処理
	player_->TryHitBoss(boss_);
#pragma endregion
}

void GameScene::ChangePhase() 
{

	switch (phase_) 
	{
	case Phase::kPlay:

		// 同時に死亡した場合はプレイヤー死亡を優先
		if (player_->IsDead()) 
		{
			// 死亡パーティクルを開始
			phase_ = Phase::kDeath;

			const Vector3& position =
				player_->GetWorldPosition();

			delete deathParticles_;
			deathParticles_ = new DeathParticles();
			deathParticles_->Initialize(
				deathParticle_model_,
				&camera_,
				position
			);
		} 
		else if (boss_->IsDead()) 
		{
			// ボス撃破時はシャッターを閉じる
			phase_ = Phase::kFadeOut;
			fade_->Start(Fade::Status::FadeOut, 0.25f);
		}

		break;
	default:
		break;
	}
}

void GameScene::StartBossCamera()
{
	player_->StopEntrance();
	Vector3 destination = boss_->GetWorldPosition();
	destination.y += 0.4f;
	destination.z -= 20.0f;
	CController_->BeginIntroMove(destination, 1.2f);
	introPhase_ = IntroPhase::kCameraToBoss;
}

void GameScene::UpdateIntro()
{
	constexpr float dt = 1.0f / 60.0f;
	switch (introPhase_)
	{
	case IntroPhase::kPlayerEntry:
	{
		const float previousX = player_->GetWorldTransform().translation_.x;
		const bool arrived = player_->UpdateEntrance(introTargetX_);
		CController_->FollowEntrance();
		introEntryTime_ += dt;
		if (std::abs(player_->GetWorldTransform().translation_.x - previousX) < 0.001f)
		{ 
			introStuckTime_ += dt; 
		}
		else 
		{ 
			introStuckTime_ = 0.0f; 
		}
		// CSV未確認のため、障害物で止まっても開始不能にならない保険。
		// 通常は指定Xへ到達して次へ進む。停止位置はマップに合わせて調整する。
		if (arrived || introStuckTime_ >= 0.7f || introEntryTime_ >= 4.0f)
		{ StartBossCamera(); }
		break;
	}
	case IntroPhase::kCameraToBoss:
		if (CController_->UpdateIntroMove())
		{
			boss_->BeginIntroPose();
			introPhase_ = IntroPhase::kBossPose;
		}
		break;
	case IntroPhase::kBossPose:
		if (boss_->UpdateIntroPose())
		{
			CController_->BeginIntroMove(CController_->GetFollowPosition(), 1.0f);
			introPhase_ = IntroPhase::kCameraReturn;
		}
		break;
	case IntroPhase::kCameraReturn:
		if (CController_->UpdateIntroMove())
		{
			// この更新では戦闘を進めず、次の更新から両者を同時に解放。
			player_->StopEntrance();
			phase_ = Phase::kPlay;
			voiceHandle = Audio::GetInstance()->PlayWave(BGMHandle, true, 0.15f);
		}
		break;
	}
}

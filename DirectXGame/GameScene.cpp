#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "GameScene.h"
#include <cmath>
#include <algorithm>

using namespace KamataEngine;

GameScene::~GameScene() 
{
	// ゲームシーンを抜けたら戦闘BGMを停止
	if (voiceHandle != 0)
	{
		Audio::GetInstance()->StopWave(voiceHandle);
		voiceHandle = 0;
	}
	// SEの途中でタイトルへ戻った場合も、音が残らないようにする。
	if (bossDefeatVoiceHandle_ != 0)
	{
		Audio::GetInstance()->StopWave(bossDefeatVoiceHandle_);
		bossDefeatVoiceHandle_ = 0;
	}

	if (gameClearVoiceHandle_ != 0)
	{
		Audio::GetInstance()->StopWave(gameClearVoiceHandle_);
		gameClearVoiceHandle_ = 0;
	}

	delete bossHpBar_;
	delete bossHpBarBack_;
	delete playerHpBar_;
	delete playerHpBarBack_;
	delete debugCamera_;
	delete model_;
	delete shardModel_;
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
	delete modelGameOver_;
	delete gameOverOverlay_;
	delete modelVictory_;
}

void GameScene::Initialize() 
{
	// ゲームシーン中だけ戦闘BGMをループ再生
	BGMHandle = Audio::GetInstance()->LoadWave("sound/Battle-Everlasting.wav");

	voiceHandle = 0; // BGMはカメラが戻って戦闘開始するときに再生

	// 音声データは、シーン開始時に一度だけ読み込む。
	// パスの書き方は既存のBGMに合わせる。
	bossDefeatSeHandle_ = Audio::GetInstance()->LoadWave("sound/boss_defeat.mp3");

	gameClearSeHandle_ = Audio::GetInstance()->LoadWave("sound/game_clear.mp3");

	// まだSEを再生していない状態にする。
	bossDefeatVoiceHandle_ = 0;
	gameClearVoiceHandle_ = 0;
	gameClearSePlayed_ = false;


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
	player_->SetBoss(boss_);
    shardModel_=Model::CreateFromOBJ("pulseShard");
    for(auto* fx:{&dodgeFx_,&impactFx_,&phaseFx_}) fx->Prepare(shardModel_,&camera_);
    player_->SetCombatParticles(&dodgeFx_,&impactFx_);
    boss_->SetPhaseParticles(&phaseFx_);

	InitializaHpBars();
	InitializeGameOverUI();
	InitializeVictory();

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

	returnToTitleRequested_ = false;
	gameOverSpaceReady_ = false;
	introPhase_ = IntroPhase::kPlayerEntry;
	introTargetX_ = playerPosition.x + kEntryWalkDistance;
	introEntryTime_ = 0.0f;
	introStuckTime_ = 0.0f;
	CController_->FollowEntrance();

}

void GameScene::InitializeVictory()
{
	// VICTORYモデルを用意したら、ここを変更する
	modelVictory_ = Model::CreateFromOBJ("GameClear", true);

	victoryCamera_.Initialize();
	victoryCamera_.TransferMatrix();

	worldTransformVictory_.Initialize();
	worldTransformVictory_.scale_ = { 2.0f, 2.0f, 2.0f };
	worldTransformVictory_.translation_ = { 0.0f, 10.0f, 0.0f };

	worldTransformVictory_.matWorld_ = MakeAffineMatrix(
		worldTransformVictory_.scale_,
		worldTransformVictory_.rotation_,
		worldTransformVictory_.translation_);

	worldTransformVictory_.TransferMatrix();

	clearTimer_ = 0.0f;
	clearSpaceReady_ = false;
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
    for(auto* fx:{&dodgeFx_,&impactFx_,&phaseFx_}) fx->Update();

	switch (phase_) 
	{
	case Phase::kIntro:
		UpdateIntro();
		break;
	case Phase::kPlay:
		UpdatePlay();
		UpdateHpBars();
		break;
	case Phase::kBossDefeat:
	case Phase::kVictory:
	case Phase::kClearFadeOut:
		UpdateClear();
		break;
	case Phase::kDeath:
		UpdateDeath();
		break;
	case Phase::kGameOver:
		UpdateGameOver();
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

	UpdateCamera();

	// ゲームオーバーまたはボス撃破時に戦闘BGMを停止
	// シーンが切り替わる前の更新で確実に止める
	if ((player_->IsDead() || boss_->IsDead()) && voiceHandle != 0)
	{
		Audio::GetInstance()->StopWave(voiceHandle);
		voiceHandle = 0;
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

	DrawPlay3D();

	Model::PostDraw();

	// 残像・斬撃・ビームを描画
	if (phase_ == Phase::kPlay)
	{
		player_->DrawAttackEffects();
	}
	boss_->DrawIntroShockwaves();
    Model::PreDraw(Model::CullingMode::kBack,Model::BlendMode::kNormal,Model::DepthTestMode::kReadOnly);
    for(auto* fx:{&dodgeFx_,&impactFx_,&phaseFx_}) fx->Draw();
    Model::PostDraw();

	Sprite::PreDraw(DirectXCommon::GetInstance()->GetCommandList());

	DrawUI();

	if (phase_ == Phase::kGameOver)
	{
		gameOverOverlay_->Draw();
	}

	Sprite::PostDraw();

	// 暗幕の上に3DのGAME OVERを描画
	if (phase_ == Phase::kGameOver)
	{
		Model::PreDraw(
			Model::CullingMode::kBack,
			Model::BlendMode::kNormal,
			Model::DepthTestMode::kOff);

		modelGameOver_->Draw(worldTransformGameOver_, gameOverCamera_);

		Model::PostDraw();
	}

	if ((phase_ == Phase::kVictory || phase_ == Phase::kClearFadeOut) && clearTimer_ >= 1.5f)
	{
		Model::PreDraw(
			Model::CullingMode::kBack,
			Model::BlendMode::kNormal,
			Model::DepthTestMode::kOff);

		modelVictory_->Draw(worldTransformVictory_, victoryCamera_);

		Model::PostDraw();
	}

	fade_->Draw();
}

void GameScene::DrawPlay3D()
{

	// 天球描画
	skydome_->Draw();

	// ブロックの描画
	for (std::vector<WorldTransform*>& worldTransformBlockLine : worldTransformBlocks_)
	{
		for (WorldTransform*& worldTransformBlock : worldTransformBlockLine)
		{
			if (!worldTransformBlock)
				continue;
			blockModel_->Draw(*worldTransformBlock, camera_);
		}
	}
	// 自キャラ描画
	if (phase_ == Phase::kPlay ||
		phase_ == Phase::kFadeIn ||
		phase_ == Phase::kIntro ||
		phase_ == Phase::kBossDefeat ||
		phase_ == Phase::kVictory ||
		phase_ == Phase::kClearFadeOut)
	{
		player_->Draw();
	}

	boss_->Draw();

	// デスパーティクルあれば描画
	if (deathParticles_)
	{
		deathParticles_->Draw();
	}
}

void GameScene::DrawUI()
{
	//クリア演出中はHPバーを隠すため、先頭に追加
	if (phase_ == Phase::kBossDefeat ||
		phase_ == Phase::kVictory ||
		phase_ == Phase::kClearFadeOut)
	{
		return;
	}

	if (phase_ != Phase::kIntro && phase_ != Phase::kFadeIn)
	{
		bossHpBarBack_->Draw();
		bossHpBar_->Draw();
		playerHpBarBack_->Draw();
		playerHpBar_->Draw();
	}
}

//自キャラとボスの当たり判定
void GameScene::CheckAllCollisions()
{
	if (!boss_ || !player_ || boss_->IsDead() || player_->IsDead())
	{
		return;
	}

	const AABB playerAABB = player_->GetAABB();

	// 連続攻撃：ボス本体ではなく、前方の攻撃判定を使う
	if (boss_->IsComboAttacking())
	{
		if (boss_->CanResolveComboContact() && IsCollision(playerAABB, boss_->GetComboAttackAABB()))
		{
			// この一撃は判定済みにする
			boss_->ResolveComboContact();

			// 既存のダメージ・無敵・ジャスト回避処理を利用
			player_->OnBulletCollision();
		}
	}
	// 突進：ボス本体を攻撃判定として使う
	else if (boss_->IsAttack() && IsCollision(playerAABB, boss_->GetAABB()))
	{
		player_->OnCollision(boss_);
	}

	// プレイヤーからボスへの攻撃
	if (!player_->IsDead())
	{
		player_->TryHitBoss(boss_);
	}
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
			/*phase_ = Phase::kDeath;*/

			const Vector3& position =
				player_->GetWorldPosition();

			delete deathParticles_;
			deathParticles_ = new DeathParticles();
			deathParticles_->Initialize(
				deathParticle_model_,
				&camera_,
				position
			);
			BeginGameOver();
		} 
		else if (boss_->IsDead()) 
		{
			BeginBossDefeat();
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

/////////////////////////プレイヤー、ボスのHPバー生成/////////////////////////
void GameScene::InitializaHpBars()
{
	//----------------ボスHPバー----------------//

	// 1×1の白い画像を読み込む
	HPtextureHandle_ = TextureManager::Load("white1x1.png");

	// HPバーの背景
	bossHpBarBack_ = Sprite::Create(HPtextureHandle_, { 390.0f, 40.0f });
	bossHpBarBack_->SetAnchorPoint({ 0.0f, 0.0f });
	bossHpBarBack_->SetSize({ 500.0f, 30.0f });
	bossHpBarBack_->SetColor({
		0.1f,
		0.1f,
		0.1f,
		1.0f
		});

	// 現在HPを表す赤いバー
	bossHpBar_ = Sprite::Create(HPtextureHandle_, { 390.0f, 40.0f });

	bossHpBar_->SetAnchorPoint({ 0.0f, 0.0f });
	bossHpBar_->SetSize({ 500.0f, 30.0f });
	bossHpBar_->SetColor({
		1.0f,
		0.0f,
		0.0f,
		1.0f
		});

	//----------------プレイヤーHPバー----------------//

	playerHpBarBack_ = Sprite::Create(
		HPtextureHandle_,
		{ 40.0f, 660.0f });

	playerHpBarBack_->SetAnchorPoint({ 0.0f, 0.0f });
	playerHpBarBack_->SetSize({ 250.0f, 20.0f });
	playerHpBarBack_->SetColor({
		0.1f,
		0.1f,
		0.1f,
		1.0f
		});

	playerHpBar_ = Sprite::Create(
		HPtextureHandle_,
		{ 40.0f, 660.0f });

	playerHpBar_->SetAnchorPoint({ 0.0f, 0.0f });
	playerHpBar_->SetSize({ 250.0f, 20.0f });
	playerHpBar_->SetColor({
		0.0f,
		0.8f,
		0.2f,
		1.0f
		});
}

void GameScene::InitializeGameOverUI()
{
	gameOverAlpha_ = 0.0f;

	// HPバーと同じ白い画像を使って、別のスプライトを作る
	gameOverOverlay_ = Sprite::Create(HPtextureHandle_, { 0.0f, 0.0f });

	gameOverOverlay_->SetAnchorPoint({ 0.0f, 0.0f });
	gameOverOverlay_->SetSize({ 1280.0f, 720.0f });
	gameOverOverlay_->SetColor({ 0.0f, 0.0f, 0.0f, gameOverAlpha_ });

	modelGameOver_ = Model::CreateFromOBJ("GameOver", true);

	gameOverCamera_.Initialize();

	worldTransformGameOver_.Initialize();
	worldTransformGameOver_.scale_ = { 2.0f, 2.0f, 2.0f };
	worldTransformGameOver_.translation_ = { 0.0f, 10.0f, 0.0f };

	worldTransformGameOver_.matWorld_ = MakeAffineMatrix(
		worldTransformGameOver_.scale_,
		worldTransformGameOver_.rotation_,
		worldTransformGameOver_.translation_);

	worldTransformGameOver_.TransferMatrix();
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
		{ 
			StartBossCamera(); 
		}
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

///////////////////プレイヤー更新、ボス更新、当たり判定、勝敗確認//////////////
void GameScene::UpdatePlay()
{

	const Phase phaseAtStart = phase_;

	// 通常プレイ中だけキャラクターと当たり判定を更新する
	if (phaseAtStart == Phase::kPlay && phase_ == Phase::kPlay)
	{
		player_->Update();
		boss_->Update();

		CheckAllCollisions();
		ChangePhase();
	}
}
////////////////////死亡パーティクルの更新と終了確認///////////////////
void GameScene::UpdateDeath()
{
	if (deathParticles_)
	{
		deathParticles_->Update();

		if (deathParticles_->IsFinished())
		{
			phase_ = Phase::kGameOver;

			fade_->Start(
				Fade::Status::FadeOut,
				0.25f
			);
		}
	}
}

void GameScene::UpdateGameOver()
{
	// 背景のボスと弾は動かし続ける
	boss_->Update();

	// 死亡パーティクルも最後まで動かす
	if (deathParticles_)
	{
		deathParticles_->Update();

		if (deathParticles_->IsFinished())
		{
			delete deathParticles_;
			deathParticles_ = nullptr;
		}
	}

	// 約0.4秒で、透明度0.65まで暗くする
	constexpr float dt = 1.0f / 60.0f;
	constexpr float targetAlpha = 0.65f;
	constexpr float fadeDuration = 0.4f;

	gameOverAlpha_ += (targetAlpha / fadeDuration) * dt;

	if (gameOverAlpha_ > targetAlpha)
	{
		gameOverAlpha_ = targetAlpha;
	}

	// 計算した透明度を見た目へ反映する
	gameOverOverlay_->SetColor(
		{ 0.0f, 0.0f, 0.0f, gameOverAlpha_ });
	// 暗幕が表示された後、新しくSPACEを押したらタイトルへ戻る
	if (!Input::GetInstance()->PushKey(DIK_SPACE))
	{
		gameOverSpaceReady_ = true;
	}
	if (gameOverSpaceReady_ && gameOverAlpha_ >= targetAlpha && Input::GetInstance()->TriggerKey(DIK_SPACE))
	{
		returnToTitleRequested_ = true;
	}
}
/////////////////////プレイヤーとボスのHPバー更新/////////////////////
void GameScene::UpdateHpBars()
{
	//----------------ボスHPバー更新----------------//
	float hpRatio = static_cast<float>(boss_->GetHp()) / static_cast<float>(boss_->GetMaxHp());

	if (hpRatio < 0.0f)
	{
		hpRatio = 0.0f;
	}

	bossHpBar_->SetSize({ 500.0f * hpRatio, 30.0f });

	float playerHpRatio = static_cast<float>(player_->GetHp()) / 3.0f;

	if (playerHpRatio < 0.0f)
	{
		playerHpRatio = 0.0f;
	}

	playerHpBar_->SetSize({ 250.0f * playerHpRatio, 20.0f });
}

void GameScene::UpdateCamera()
{
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
		else 
		{ 
			CController_->Updata(); 
		}
	}
}

void GameScene::BeginBossDefeat()
{
	phase_ = Phase::kBossDefeat;
	clearTimer_ = 0.0f;
	clearSpaceReady_ = false;
	returnToTitleRequested_ = false;

	delete deathParticles_;
	deathParticles_ = nullptr;

	boss_->BeginDefeat();

	// 次のクリア演出に備えて、文字のSEの再生状態をリセット。
	gameClearSePlayed_ = false;

	// HPが0になり、撃破演出へ入った瞬間に鳴らす。
	// falseは「ループしない」、0.7fは音量。
	bossDefeatVoiceHandle_ = Audio::GetInstance()->PlayWave(bossDefeatSeHandle_, false, 0.7f);
}

void GameScene::UpdateClear()
{
	constexpr float dt = 1.0f / 60.0f;
	constexpr float burstTime = 0.7f;
	constexpr float textTime = 1.5f;

	clearTimer_ += dt;

	// 通常のplayer_->Update()、boss_->Update()、
	// 当たり判定は呼ばず、演出だけを進める
	if (phase_ == Phase::kBossDefeat)
	{
		boss_->UpdateDefeat(clearTimer_);

		if (clearTimer_ >= burstTime)
		{
			// ボスが消えるタイミングで、一度だけ生成する。
			deathParticles_ = new DeathParticles();

			deathParticles_->Initialize(
				deathParticle_model_,     // 粒として表示するモデル
				&camera_,                 // 戦闘画面と同じカメラ
				boss_->GetWorldPosition(),// ボスの位置から発生
				true                      // ボス撃破用の設定を使う
			);

			phase_ = Phase::kVictory;
		}
	}

	if (deathParticles_)
	{
		deathParticles_->Update();

		if (deathParticles_->IsFinished())
		{
			delete deathParticles_;
			deathParticles_ = nullptr;
		}
	}

	if (clearTimer_ >= textTime)
	{
		// このifには、文字の表示開始後も毎フレーム入る。
	    // そのため、まだ鳴らしていない場合だけ再生する。
		if (!gameClearSePlayed_)
		{
			gameClearVoiceHandle_ = Audio::GetInstance()->PlayWave(gameClearSeHandle_, false, 0.2f);

			// 再生したことを記録し、次のフレームからは鳴らさない。
			gameClearSePlayed_ = true;
		}


		float t = (clearTimer_ - textTime) / 0.35f;
		t = (std::clamp)(t, 0.0f, 1.0f);

		// 大きく登場して、少し行き過ぎてから収まる
		const float u = t - 1.0f;
		const float ease =
			1.0f + 2.70158f * u * u * u + 1.70158f * u * u;

		const float scale = 2.0f + 0.8f * (1.0f - ease);

		worldTransformVictory_.scale_ = { scale, scale, scale };
		worldTransformVictory_.translation_ = 
		{
			0.0f,
			10.0f + 1.5f * (1.0f - ease),
			0.0f
		};

		worldTransformVictory_.matWorld_ = MakeAffineMatrix(
			worldTransformVictory_.scale_,
			worldTransformVictory_.rotation_,
			worldTransformVictory_.translation_);

		worldTransformVictory_.TransferMatrix();
	}

	if (phase_ == Phase::kClearFadeOut)
	{
		fade_->Update();

		if (fade_->IsFinished())
		{
			returnToTitleRequested_ = true;
		}

		return;
	}

	// 演出終了後、一度SPACEを離してから押す
	if (phase_ == Phase::kVictory && clearTimer_ >= 3.0f)
	{
		if (!Input::GetInstance()->PushKey(DIK_SPACE))
		{
			clearSpaceReady_ = true;
		}

		if (clearSpaceReady_ &&
			Input::GetInstance()->TriggerKey(DIK_SPACE))
		{
			phase_ = Phase::kClearFadeOut;
			fade_->Start(Fade::Status::FadeOut, 0.25f);
		}
	}
}

void GameScene::BeginGameOver()
{
	returnToTitleRequested_ = false;
	// ジャンプで押していたSPACEを一度離してから受け付ける
	gameOverSpaceReady_ = !Input::GetInstance()->PushKey(DIK_SPACE);
	phase_ = Phase::kGameOver;
	gameOverAlpha_ = 0.0f;

	gameOverOverlay_->SetColor({ 0.0f, 0.0f, 0.0f, gameOverAlpha_ });

}



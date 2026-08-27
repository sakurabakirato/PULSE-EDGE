#include "GameScene.h"

using namespace KamataEngine;

GameScene::~GameScene() 
{
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

	// 02_11_17枚目
	delete deathParticles_;
	delete deathParticle_model_;
	delete fade_;
}

void GameScene::Initialize() 
{

	BGMHandle = Audio::GetInstance()->LoadWave("sound/1123.mp3");

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

	// 02_03天球
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
	textureHandle_ =
		TextureManager::Load("white1x1.png");

	// HPバーの背景
	hpBarBack_ = Sprite::Create(
		textureHandle_,
		{ 390.0f, 40.0f });

	hpBarBack_->SetAnchorPoint({ 0.0f, 0.0f });
	hpBarBack_->SetSize({ 500.0f, 30.0f });
	hpBarBack_->SetColor({
		0.1f,
		0.1f,
		0.1f,
		1.0f
		});

	// 現在HPを表す赤いバー
	sprite_ = Sprite::Create(
		textureHandle_,
		{ 390.0f, 40.0f });

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
		textureHandle_,
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
		textureHandle_,
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
	deathParticles_ = new DeathParticles;
	deathParticles_->Initialize(deathParticle_model_, &camera_, playerPosition);

	// 02_12 4枚目 ゲームプレイフェーズから開始
	phase_ = Phase::kFadeIn;

	//---------------fade------------------//
	fade_ = new Fade();
	fade_->Initialize();
	fade_->Start(Fade::Status::FadeIn, 1.0f);
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
	switch (phase_) 
	{
	case Phase::kPlay:

		break;
	case Phase::kDeath:
		// デス演出フェーズ
		// 02_11 18枚目 デスパーティクルあれば更新
		deathParticles_->Update();

		if (deathParticles_ && deathParticles_->IsFinished()) 
		{
			phase_ = Phase::kFadeOut;
			fade_->Start(Fade::Status::FadeOut, 1.0f);
		}

		break;
	case Phase::kFadeIn:
		fade_->Update();
		if (fade_->IsFinished()) 
		{
			phase_ = Phase::kPlay;
		}
		break;
	case Phase::kFadeOut:
		fade_->Update();
		if (fade_->IsFinished()) 
		{
			finished_ = true;
		}
	}

	// 天球生成
	skydome_->Update();

	// 自キャラの更新
	player_->Update();

	// ボス更新
	boss_->Update();

	//----------------ボスHPバー更新----------------//

	float hpRatio =
		static_cast<float>(boss_->GetHp()) /
		static_cast<float>(boss_->GetMaxHp());

	if (hpRatio < 0.0f)
	{
		hpRatio = 0.0f;
	}

	sprite_->SetSize({
		500.0f * hpRatio,
		30.0f
		});

	float playerHpRatio =
		static_cast<float>(player_->GetHp()) / 3.0f;

	if (playerHpRatio < 0.0f)
	{
		playerHpRatio = 0.0f;
	}

	playerHpBar_->SetSize({
		250.0f * playerHpRatio,
		20.0f
		});

	ChangePhase();

	CheckAllCollisions();
	

	// カメラの処理
	if (isDebugCameraActive_) 
	{
		debugCamera_->Update();
		camera_.matView = debugCamera_->GetCamera().matView;
		camera_.matProjection = debugCamera_->GetCamera().matProjection;
		// ビュープロジェクション行列の転送AL3_02_02*/
		camera_.TransferMatrix();
	}
	else 
	{
		// ビュープロジェクション行列の更新と転送AL3_02_02*/

		camera_.UpdateMatrix();
	}
	CController_->Updata();

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
	if (phase_ == Phase::kPlay || phase_ == Phase::kFadeIn) 
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

	//----------------ボスHPバー描画----------------//

	Sprite::PreDraw(
		DirectXCommon::GetInstance()->GetCommandList());

	hpBarBack_->Draw();
	sprite_->Draw();
	playerHpBarBack_->Draw();
	playerHpBar_->Draw();

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

	// Jキーを押した瞬間だけ攻撃判定
	if (player_->IsAttack())
	{
		AABB attackAABB = player_->GetAttackAABB();

		if (IsCollision(attackAABB, bossAABB))
		{
			// ボスへ1ダメージ
			boss_->OnDamage(1);
		}
	}
#pragma endregion
}

void GameScene::ChangePhase() 
{

	switch (phase_) 
	{
	case Phase::kPlay:
		// 02_12 13枚目 if文から中身まで全部実装
		// Initialize関数のいきなりパーティクル発生処理は消す
		if (player_->IsDead()) 
		{
			// 死亡演出
			phase_ = Phase::kDeath;

			const Vector3& deathParticlesPosition = player_->GetWorldPosition();

			deathParticles_ = new DeathParticles;
			deathParticles_->Initialize(deathParticle_model_, &camera_, deathParticlesPosition);
		}
		break;
	case Phase::kDeath:

		break;
	}
}

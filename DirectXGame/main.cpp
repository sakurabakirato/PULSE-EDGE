#include "GameScene.h"
#include "KamataEngine.h"
#include "TitleScene.h"
#include "GameClear.h"
#include "GameOver.h"
#include <Windows.h>

using namespace KamataEngine;

TitleScene* titleScene = nullptr;
GameScene* gameScene = nullptr;
GameClear* gameClear = nullptr;
GameOver* gameOver = nullptr;

enum class Scene 
{
	kUnknown = 0,
	kTitle,
	kGame,
	kClear,
	kOver,
};

// 現在シーン（型）
Scene scene = Scene::kUnknown;

void ChangeScene() 
{

	switch (scene)
	{
	case Scene::kTitle:
		if (titleScene->IsFinished())
		{
			// シーン変更
			scene = Scene::kGame;
			delete titleScene;
			titleScene = nullptr;
			gameScene = new GameScene;
			gameScene->Initialize();
		}
		break;

	case Scene::kGame:
		if (gameScene->IsReturnToTitleRequested())
		{
			delete gameScene;
			gameScene = nullptr;

			titleScene = new TitleScene();
			titleScene->Initialize();
			scene = Scene::kTitle;
			break;
		}

		// ゲームをクリアした
		if (gameScene->IsFinished())
		{
			// ゲームクリアへ変更
			scene = Scene::kClear;

			gameClear = new GameClear();
			gameClear->Initialize();

			delete gameScene;
			gameScene = nullptr;
		}
		break;
	case Scene::kClear:
		if (gameClear && gameClear->IsFinished())
		{
			delete gameClear;
			gameClear = nullptr;

			titleScene = new TitleScene();
			titleScene->Initialize();

			scene = Scene::kTitle;
		}
		break;
	case Scene::kOver:
		if (gameOver && gameOver->IsFinished())
		{
			delete gameOver;
			gameOver = nullptr;

			titleScene = new TitleScene();
			titleScene->Initialize();

			scene = Scene::kTitle;
		}
		break;
	}
}

void UpdataScene() 
{
	switch (scene) 
	{
	case Scene::kTitle:
		titleScene->Update();
		break;
	case Scene::kGame:
		gameScene->Update();
		break;
	case Scene::kClear:
		gameClear->Update();
		break;
	case Scene::kOver:
		gameOver->Update();
		break;
	}
}

void DrawScene() 
{
	switch (scene) 
	{
	case Scene::kTitle:
		titleScene->Draw();
		break;
	case Scene::kGame:
		gameScene->Draw();
		break;
	case Scene::kClear:
		gameClear->Draw();
		break;
	case Scene::kOver:
		gameOver->Draw();
		break;
	}
}

// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) 
{

	KamataEngine::Initialize();

	DirectXCommon* dxCommon = DirectXCommon::GetInstance();

	scene = Scene::kTitle;
	titleScene = new TitleScene;
	titleScene->Initialize(false);

	// メインループ
	while (true) 
	{

		// エンジンの更新
		if (KamataEngine::Update()) 
		{
			break;
		}

		// シーン切り替え
		ChangeScene();

		// 現在シーン更新
		UpdataScene();

		// 描画更新
		dxCommon->PreDraw();

		// ゲームシーンの描画
		DrawScene();

		// 描画終了
		dxCommon->PostDraw();
	}

	// 各種解放
	delete titleScene;
	delete gameScene;
	delete gameClear;
	delete gameOver;

	// nullptrの代入
	gameScene = nullptr;

	KamataEngine::Finalize();

	return 0;
}
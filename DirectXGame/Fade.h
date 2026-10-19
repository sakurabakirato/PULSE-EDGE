#pragma once
#include "KamataEngine.h"

using namespace KamataEngine;

class Fade 
{
public:
    enum class Status 
    {
        None,
        FadeIn,   // シャッターを開く
        FadeOut,  // シャッターを閉じる
    };

    ~Fade();

    void Initialize();
    void Update();
    void Draw();

    void Start(Status status, float duration);
    void Stop();

    bool IsFinished() const;

private:
    // パネルとラインの配置を更新する
    void UpdateLayout();

    Sprite* topPanel_ = nullptr;
    Sprite* bottomPanel_ = nullptr;

    Sprite* topLine_ = nullptr;
    Sprite* bottomLine_ = nullptr;

    Status status_ = Status::None;

    float duration_ = 0.0f;
    float counter_ = 0.0f;

    // 0：完全に開いている
    // 1：完全に閉じている
    float coverage_ = 0.0f;
};
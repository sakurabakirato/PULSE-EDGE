#include "Fade.h"
#include <algorithm>

Fade::~Fade() 
{
    delete topPanel_;
    delete bottomPanel_;
    delete topLine_;
    delete bottomLine_;
}

void Fade::Initialize() 
{
    uint32_t textureHandle = TextureManager::Load("white1x1.png");

    topPanel_ = Sprite::Create(textureHandle, { 0.0f, 0.0f });
    bottomPanel_ = Sprite::Create(textureHandle, { 0.0f, 0.0f });

    topLine_ = Sprite::Create(textureHandle, { 0.0f, 0.0f });
    bottomLine_ = Sprite::Create(textureHandle, { 0.0f, 0.0f });

    // 左上を基準に配置する
    topPanel_->SetAnchorPoint({ 0.0f, 0.0f });
    bottomPanel_->SetAnchorPoint({ 0.0f, 0.0f });
    topLine_->SetAnchorPoint({ 0.0f, 0.0f });
    bottomLine_->SetAnchorPoint({ 0.0f, 0.0f });

    // 暗い紺色のパネル。不透明にする
    topPanel_->SetColor({ 0.02f, 0.03f, 0.06f, 1.0f });
    bottomPanel_->SetColor({ 0.02f, 0.03f, 0.06f, 1.0f });

    // シアンのライン
    topLine_->SetColor({ 0.0f, 0.9f, 1.0f, 1.0f });
    bottomLine_->SetColor({ 0.0f, 0.9f, 1.0f, 1.0f });

    status_ = Status::None;
    coverage_ = 0.0f;

    UpdateLayout();
}

void Fade::Start(Status status, float duration) 
{
    status_ = status;

    // 0秒による割り算を防ぐ
    duration_ = (std::max)(duration, 0.001f);
    counter_ = 0.0f;

    // 開くときは閉じた状態から開始する
    // 閉じるときは開いた状態から開始する
    coverage_ = (status == Status::FadeIn) ? 1.0f : 0.0f;

    // 最初の描画から正しい位置になるようにする
    UpdateLayout();
}

void Fade::Update() 
{
    if (status_ == Status::None) 
    {
        return;
    }

    // 既存コードと同じ固定60fpsを前提にする
    counter_ = (std::min)(counter_ + 1.0f / 60.0f, duration_);

    float t = std::clamp(
        counter_ / duration_,
        0.0f,
        1.0f
    );

    // 動き始めと終わりをなめらかにする
    float ease = t * t * (3.0f - 2.0f * t);

    if (status_ == Status::FadeOut) 
    {
        coverage_ = ease;
    } 
    else 
    {
        coverage_ = 1.0f - ease;
    }

    UpdateLayout();
}

void Fade::UpdateLayout() 
{
    const float width =
        static_cast<float>(WinApp::kWindowWidth);

    const float height =
        static_cast<float>(WinApp::kWindowHeight);

    // 上下それぞれ画面の半分まで覆う
    float panelHeight = height * 0.5f * coverage_;

    topPanel_->SetPosition({ 0.0f, 0.0f });
    topPanel_->SetSize({ width, panelHeight });

    bottomPanel_->SetPosition({
        0.0f,
        height - panelHeight
        });
    bottomPanel_->SetSize({ width, panelHeight });

    // パネルが小さい間はラインも細くする
    float lineHeight = (std::min)(3.0f, panelHeight);

    // 上パネルの下端
    topLine_->SetPosition({
        0.0f,
        panelHeight - lineHeight
        });
    topLine_->SetSize({ width, lineHeight });

    // 下パネルの上端
    bottomLine_->SetPosition({
        0.0f,
        height - panelHeight
        });
    bottomLine_->SetSize({ width, lineHeight });
}

void Fade::Draw() 
{
    if (status_ == Status::None || coverage_ <= 0.0f) 
    {
        return;
    }

    Sprite::PreDraw(
        DirectXCommon::GetInstance()->GetCommandList()
    );

    topPanel_->Draw();
    bottomPanel_->Draw();

    // ラインはパネルの上に描く
    topLine_->Draw();
    bottomLine_->Draw();

    Sprite::PostDraw();
}

void Fade::Stop() 
{
    status_ = Status::None;
}

bool Fade::IsFinished() const 
{
    return status_ == Status::None ||
        counter_ >= duration_;
}
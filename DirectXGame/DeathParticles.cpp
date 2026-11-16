#include "DeathParticles.h"

#include <algorithm>
#include <cmath>
#include <numbers>

void DeathParticles::Initialize(
    Model* model,
    Camera* camera,
    const Vector3& position,
    bool forBossClear)
{
    // GameSceneが持っているモデルとカメラを使う。
    model_ = model;
    camera_ = camera;

    forBossClear_ = forBossClear;

    // 演出を最初から開始する。
    elapsed_ = 0.0f;
    isFinished_ = false;

    if (forBossClear_)
    {
        // ボス撃破用は、粒を増やして短時間で広げる。
        particleCount_ = 32;
        duration_ = 1.2f;

        // 金色にする。最後の値は透明度。
        color_ = { 1.0f, 0.8f, 0.25f, 1.0f };
    } 
    else
    {
        // 通常の死亡演出は、元の設定を基本にする。
        particleCount_ = 8;
        duration_ = 2.0f;

        color_ = { 1.0f, 1.0f, 1.0f, 1.0f };
    }

    objectColor_.Initialize();
    objectColor_.SetColor(color_);

    // 円1周を粒の数で割る。
    // 8個なら、45度ずつ違う方向へ飛ぶ。
    const float angleStep =
        2.0f * std::numbers::pi_v<float> /
        static_cast<float>(particleCount_);

    for (uint32_t i = 0; i < particleCount_; ++i)
    {
        WorldTransform& transform = worldTransforms_[i];

        transform.Initialize();

        // 全粒を同じ位置から発生させる。
        transform.translation_ = position;

        // 粒の番号によって、飛ぶ方向を変える。
        const float angle = angleStep * static_cast<float>(i);

        float speed = 3.0f;

        if (forBossClear_)
        {
            // 10、12、14、16、18の5種類の速さを繰り返す。
            // 全粒が同じ距離に並ぶのを避けるため。
            speed =
                10.0f + static_cast<float>(i % 5) * 2.0f;

            // ボス撃破用は、小さな破片として表示する。
            transform.scale_ = { 0.18f, 0.18f, 0.18f };

            // 見た目の向きも少しずつ変える。
            transform.rotation_.z = angle;
        } 
        else
        {
            transform.scale_ = { 1.0f, 1.0f, 1.0f };
        }

        // cosで横方向、sinで縦方向の成分を求める。
        // これでXY平面上に放射状に飛ばせる。
        //
        // 速度の単位は「1秒あたりに進む距離」。
        velocities_[i] = {
            std::cos(angle) * speed,
            std::sin(angle) * speed,
            0.0f
        };
    }

    // Updateより先にDrawが呼ばれても正しい位置で描けるように、
    // 初期化時にも行列を反映する。
    UpdateMatrices();
}

void DeathParticles::Update()
{
    if (isFinished_)
    {
        return;
    }

    // 既存コードに合わせ、60FPSを前提とする。
    constexpr float dt = 1.0f / 60.0f;

    elapsed_ += dt;

    if (elapsed_ >= duration_)
    {
        isFinished_ = true;
        return;
    }

    // 演出がどれくらい進んだかを、0～1で表す。
    //
    // 開始時       : 0
    // 時間の半分   : 0.5
    // 終了時       : 1
    const float progress =
        std::clamp(elapsed_ / duration_, 0.0f, 1.0f);

    for (uint32_t i = 0; i < particleCount_; ++i)
    {
        WorldTransform& transform = worldTransforms_[i];
        Vector3& velocity = velocities_[i];

        // 移動距離 = 1秒あたりの速度 × 経過した秒数。
        transform.translation_.x += velocity.x * dt;
        transform.translation_.y += velocity.y * dt;
        transform.translation_.z += velocity.z * dt;

        if (forBossClear_)
        {
            // 速度を毎フレーム94%にする。
            // 最初は速く、徐々に減速する動きになる。
            velocity.x *= 0.94f;
            velocity.y *= 0.94f;
            velocity.z *= 0.94f;

            // 破片が回転しながら飛ぶようにする。
            transform.rotation_.z += 4.0f * dt;

            // 時間が進むほど小さくする。
            const float scale = 0.18f * (1.0f - progress);

            transform.scale_ = { scale, scale, scale };
        }
    }

    // 透明度も1から0へ下げる。
    // 実際の透過の見え方は、モデルの描画設定にも依存する。
    color_.w = 1.0f - progress;
    objectColor_.SetColor(color_);

    // 変更した位置・回転・大きさを描画へ反映する。
    UpdateMatrices();
}

void DeathParticles::UpdateMatrices()
{
    for (uint32_t i = 0; i < particleCount_; ++i)
    {
        WorldTransform& transform = worldTransforms_[i];

        // 大きさ・回転・位置を、1つの変換行列にまとめる。
        transform.matWorld_ = MakeAffineMatrix(
            transform.scale_,
            transform.rotation_,
            transform.translation_);

        // 計算結果を描画で使えるように転送する。
        transform.TransferMatrix();
    }
}

void DeathParticles::Draw()
{
    if (isFinished_)
    {
        return;
    }

    // Drawでは時間や座標を進めない。
    // Updateで決めた状態を、そのまま描画する。
    for (uint32_t i = 0; i < particleCount_; ++i)
    {
        model_->Draw(
            worldTransforms_[i],
            *camera_,
            &objectColor_);
    }
}
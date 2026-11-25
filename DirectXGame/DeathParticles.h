#pragma once

#include "KamataEngine.h"
#include "Math.h"

#include <array>

using namespace KamataEngine;

class DeathParticles
{
public:
    // forBossClear:
    // false = 通常の死亡演出
    // true  = ボス撃破用の、勢いよくはじける演出
    //
    // 「= false」があるので、既存の3引数の呼び出しも使える。
    void Initialize(
        Model* model,
        Camera* camera,
        const Vector3& position,
        bool forBossClear = false);

    enum class Effect { Dodge, Impact, Gather, Release };
    // Prepare once, then reuse the same transforms and color buffer.
    void Prepare(Model* shard, Camera* camera);
    void Emit(Effect effect, Vector3 origin, float direction=1.f);
    void Update();
    void Draw();

    bool IsFinished() const
    {
        return isFinished_;
    }

private:
    bool combat_ = false;
    Effect effect_ = Effect::Dodge;
    Vector3 origin_{};
    float direction_=1.f;
    void PoseCombat();
    // 最大32個分を用意しておく。
    // 実際に何個使うかは、particleCount_で決める。
    static constexpr uint32_t kMaxParticles = 32;

    // 各粒の位置・回転・大きさ。
    std::array<WorldTransform, kMaxParticles> worldTransforms_;

    // 各粒の速度。
    // Initializeで方向と速さを決め、Updateで減速させる。
    std::array<Vector3, kMaxParticles> velocities_{};

    // モデルとカメラはGameSceneから借りる。
    // このクラスではnewしていないため、deleteもしない。
    Model* model_ = nullptr;
    Camera* camera_ = nullptr;

    // 今回の演出で使う粒の数。
    uint32_t particleCount_ = 8;

    bool forBossClear_ = false;
    bool isFinished_ = false;

    // 秒単位で管理する。
    float elapsed_ = 0.0f;
    float duration_ = 2.0f;

    // 今回は全粒で同じ色と透明度を使う。
    ObjectColor objectColor_;
    Vector4 color_ = { 1.0f, 1.0f, 1.0f, 1.0f };

    // 座標・回転・大きさを、描画に使う行列へ反映する。
    void UpdateMatrices();
};
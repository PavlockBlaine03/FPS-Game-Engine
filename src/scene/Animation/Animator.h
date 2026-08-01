#pragma once

#include <glm/glm.hpp>

#include <string>
#include <unordered_set>
#include <vector>

class SkeletalModel;
struct AnimationClip;

class Animator
{
public:
    explicit Animator(const SkeletalModel& model);

    bool play(const std::string& clipName, bool loop = true, float blendSeconds = 0.2F);
    void ignoreNodeAnimation(const std::string& nodeName);
    void update(float deltaTime);

    [[nodiscard]] const std::vector<glm::mat4>& skinMatrices() const { return m_skinMatrices; }
    [[nodiscard]] const std::vector<glm::mat4>& globalPose() const { return m_globalPose; }

private:
    struct Playback
    {
        const AnimationClip* clip = nullptr;
        float time = 0.0F;
        bool loop = true;
    };

    void advance(Playback& playback, float deltaTime) const;
    void evaluate();

    const SkeletalModel& m_model;
    Playback m_current;
    Playback m_previous;
    float m_blendDuration = 0.0F;
    float m_blendElapsed = 0.0F;
    std::vector<glm::mat4> m_globalPose;
    std::vector<glm::mat4> m_skinMatrices;
    std::unordered_set<int> m_ignoredAnimationNodes;
};

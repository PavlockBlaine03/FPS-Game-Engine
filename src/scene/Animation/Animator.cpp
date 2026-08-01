#include "scene/Animation/Animator.h"

#include "rendering/SkeletalModel.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <algorithm>
#include <cmath>

namespace
{
    template<typename T, typename Interpolator>
    T sampleKeys(
        const std::vector<AnimationKey<T>>& keys,
        const float time,
        const T& fallback,
        Interpolator interpolate)
    {
        if (keys.empty()) { return fallback; }
        if (keys.size() == 1 || time <= keys.front().time) { return keys.front().value; }
        if (time >= keys.back().time) { return keys.back().value; }

        const auto upper = std::upper_bound(
            keys.begin(), keys.end(), time,
            [](const float value, const AnimationKey<T>& key) { return value < key.time; });
        const auto lower = upper - 1;
        const float span = upper->time - lower->time;
        const float factor = span > 0.0F ? (time - lower->time) / span : 0.0F;
        return interpolate(lower->value, upper->value, factor);
    }

    struct NodePose
    {
        glm::vec3 translation{ 0.0F };
        glm::quat rotation{ 1.0F, 0.0F, 0.0F, 0.0F };
        glm::vec3 scale{ 1.0F };
    };

    NodePose sampleNode(
        const SkeletonNode& node,
        const AnimationClip* clip,
        const float time,
        const int nodeIndex)
    {
        NodePose pose{ node.bindTranslation, node.bindRotation, node.bindScale };
        if (clip == nullptr) { return pose; }
        const auto found = clip->channelByNode.find(nodeIndex);
        if (found == clip->channelByNode.end()) { return pose; }

        const AnimationChannel& channel = clip->channels[found->second];
        pose.translation = sampleKeys(channel.translations, time, pose.translation,
            [](const glm::vec3& a, const glm::vec3& b, const float t) { return glm::mix(a, b, t); });
        pose.rotation = sampleKeys(channel.rotations, time, pose.rotation,
            [](const glm::quat& a, const glm::quat& b, const float t) { return glm::normalize(glm::slerp(a, b, t)); });
        pose.scale = sampleKeys(channel.scales, time, pose.scale,
            [](const glm::vec3& a, const glm::vec3& b, const float t) { return glm::mix(a, b, t); });
        return pose;
    }

    glm::mat4 poseMatrix(const NodePose& pose)
    {
        glm::mat4 result = glm::translate(glm::mat4(1.0F), pose.translation);
        result *= glm::mat4_cast(pose.rotation);
        return glm::scale(result, pose.scale);
    }
}

Animator::Animator(const SkeletalModel& model)
    : m_model(model)
    , m_globalPose(model.nodes().size(), glm::mat4(1.0F))
    , m_skinMatrices(model.bones().size(), glm::mat4(1.0F))
{
    evaluate();
}

bool Animator::play(const std::string& clipName, const bool loop, const float blendSeconds)
{
    const AnimationClip* clip = m_model.findClip(clipName);
    if (clip == nullptr) { return false; }
    if (m_current.clip == clip)
    {
        m_current.loop = loop;
        return true;
    }

    m_previous = m_current;
    m_current = Playback{ clip, 0.0F, loop };
    m_blendDuration = (m_previous.clip != nullptr) ? std::max(0.0F, blendSeconds) : 0.0F;
    m_blendElapsed = 0.0F;
    evaluate();
    return true;
}

void Animator::ignoreNodeAnimation(const std::string& nodeName)
{
    const auto& nodes = m_model.nodes();
    for (std::size_t i = 0; i < nodes.size(); ++i)
    {
        if (nodes[i].name == nodeName)
        {
            m_ignoredAnimationNodes.insert(static_cast<int>(i));
            evaluate();
            return;
        }
    }
}

void Animator::advance(Playback& playback, const float deltaTime) const
{
    if (playback.clip == nullptr) { return; }
    playback.time += std::max(0.0F, deltaTime);
    if (playback.loop && playback.clip->durationSeconds > 0.0F)
    {
        playback.time = std::fmod(playback.time, playback.clip->durationSeconds);
    }
    else
    {
        playback.time = std::min(playback.time, playback.clip->durationSeconds);
    }
}

void Animator::update(const float deltaTime)
{
    advance(m_current, deltaTime);
    advance(m_previous, deltaTime);
    if (m_blendElapsed < m_blendDuration)
    {
        m_blendElapsed = std::min(m_blendElapsed + std::max(0.0F, deltaTime), m_blendDuration);
    }
    evaluate();
}

void Animator::evaluate()
{
    const auto& nodes = m_model.nodes();
    const float blend = m_blendDuration > 0.0F
        ? std::clamp(m_blendElapsed / m_blendDuration, 0.0F, 1.0F)
        : 1.0F;

    for (std::size_t i = 0; i < nodes.size(); ++i)
    {
        const bool ignoreAnimation = m_ignoredAnimationNodes.contains(static_cast<int>(i));
        NodePose pose = sampleNode(
            nodes[i], ignoreAnimation ? nullptr : m_current.clip,
            m_current.time, static_cast<int>(i));
        if (blend < 1.0F && m_previous.clip != nullptr)
        {
            const NodePose oldPose = sampleNode(
                nodes[i], ignoreAnimation ? nullptr : m_previous.clip,
                m_previous.time, static_cast<int>(i));
            pose.translation = glm::mix(oldPose.translation, pose.translation, blend);
            pose.rotation = glm::normalize(glm::slerp(oldPose.rotation, pose.rotation, blend));
            pose.scale = glm::mix(oldPose.scale, pose.scale, blend);
        }

        const glm::mat4 local = poseMatrix(pose);
        m_globalPose[i] = nodes[i].parent >= 0
            ? m_globalPose[static_cast<std::size_t>(nodes[i].parent)] * local
            : local;
    }

    const auto& bones = m_model.bones();
    for (std::size_t i = 0; i < bones.size(); ++i)
    {
        m_skinMatrices[i] = m_model.inverseRoot()
            * m_globalPose[static_cast<std::size_t>(bones[i].nodeIndex)]
            * bones[i].inverseBind;
    }
}

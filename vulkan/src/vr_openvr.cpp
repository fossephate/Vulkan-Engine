#include "vr_common.hpp"
#include <openvr.h>

namespace openvr {
    template<typename F>
    void for_each_eye(F f) {
        f(vr::Hmd_Eye::Eye_Left);
        f(vr::Hmd_Eye::Eye_Right);
    }

    inline mat4 toGlm(const vr::HmdMatrix44_t& m) {
        return glm::transpose(glm::make_mat4(&m.m[0][0]));
    }

    inline vec3 toGlm(const vr::HmdVector3_t& v) {
        return vec3(v.v[0], v.v[1], v.v[2]);
    }

    inline mat4 toGlm(const vr::HmdMatrix34_t& m) {
        mat4 result = mat4(
            m.m[0][0], m.m[1][0], m.m[2][0], 0.0,
            m.m[0][1], m.m[1][1], m.m[2][1], 0.0,
            m.m[0][2], m.m[1][2], m.m[2][2], 0.0,
            m.m[0][3], m.m[1][3], m.m[2][3], 1.0f);
        return result;
    }

    inline vr::HmdMatrix34_t toOpenVr(const mat4& m) {
        vr::HmdMatrix34_t result;
        for (uint8_t i = 0; i < 3; ++i) {
            for (uint8_t j = 0; j < 4; ++j) {
                result.m[i][j] = m[j][i];
            }
        }
        return result;
    }
}

class OpenVrExample : public VrExampleBase {
    using Parent = VrExampleBase;
public:
    std::array<glm::mat4, 2> eyeOffsets;
    vr::IVRSystem* vrSystem{ nullptr };
    vr::IVRCompositor* vrCompositor{ nullptr };

    void submitVrFrame() override {
        // Flip y-axis since GL UV coords are backwards.
        static vr::VRTextureBounds_t leftBounds{ 0, 0, 0.5f, 1 };
        static vr::VRTextureBounds_t rightBounds{ 0.5f, 0, 1, 1 };
        vr::Texture_t texture{ (void*)_colorBuffer, vr::API_OpenGL, vr::ColorSpace_Auto };
        vrCompositor->Submit(vr::Eye_Left, &texture, &leftBounds);
        vrCompositor->Submit(vr::Eye_Right, &texture, &rightBounds);
    }

    virtual void renderMirror() override {
        gl::nv::vk::DrawVkImage(vulkanRenderer.framebuffer.colors[0].image, 0, vec2(0), size, 0, glm::vec2(0), glm::vec2(1));
    }

    void update(float delta) override {
        vr::TrackedDevicePose_t currentTrackedDevicePose[vr::k_unMaxTrackedDeviceCount];
        vrCompositor->WaitGetPoses(currentTrackedDevicePose, vr::k_unMaxTrackedDeviceCount, nullptr, 0);
        vr::TrackedDevicePose_t _trackedDevicePose[vr::k_unMaxTrackedDeviceCount];
        float displayFrequency = vrSystem->GetFloatTrackedDeviceProperty(vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_DisplayFrequency_Float);
        float frameDuration = 1.f / displayFrequency;
        float vsyncToPhotons = vrSystem->GetFloatTrackedDeviceProperty(vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_SecondsFromVsyncToPhotons_Float);
        float predictedDisplayTime = frameDuration + vsyncToPhotons;
        vrSystem->GetDeviceToAbsoluteTrackingPose(vr::TrackingUniverseStanding, (float)predictedDisplayTime, _trackedDevicePose, vr::k_unMaxTrackedDeviceCount);
        auto basePose = openvr::toGlm(_trackedDevicePose[vr::k_unTrackedDeviceIndex_Hmd].mDeviceToAbsoluteTracking);
        eyeViews = std::array<glm::mat4, 2>{ glm::inverse(basePose * eyeOffsets[0]), glm::inverse(basePose * eyeOffsets[1]) };
        Parent::update(delta);
    }

    void prepare() {
        vr::EVRInitError eError;
        vrSystem = vr::VR_Init(&eError, vr::VRApplication_Scene);
        vrSystem->GetRecommendedRenderTargetSize(&renderTargetSize.x, &renderTargetSize.y);
        vrCompositor = vr::VRCompositor();
        // Recommended render target size is per-eye, so double the X size for 
        // left + right eyes
        renderTargetSize.x *= 2;

        openvr::for_each_eye([&](vr::Hmd_Eye eye) {
            eyeOffsets[eye] = openvr::toGlm(vrSystem->GetEyeToHeadTransform(eye));
            eyeProjections[eye] = openvr::toGlm(vrSystem->GetProjectionMatrix(eye, 0.1f, 256.0f, vr::API_OpenGL));
        });
        Parent::prepare();
    }

    std::string getWindowTitle() {
        std::string device(vulkanContext.deviceProperties.deviceName);
        return "OpenGL Interop - " + device + " - " + std::to_string(frameCounter) + " fps";
    }
};

RUN_EXAMPLE(OpenVrExample)

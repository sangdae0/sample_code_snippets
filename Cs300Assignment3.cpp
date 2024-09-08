#include "Cs300Assignment3.h"
#include "../Common/shader.hpp"
#include "imgui.h"
#include "../OBJLoader.h"
#include "../ShaderManager.h"
#include <iostream>
#include <string>


#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/ext.hpp>

#include <unordered_map>
#include <vector>
#include <cmath>

Cs300Assignment3::Cs300Assignment3(int winWidth, int winHeight)
    : Scene(winWidth, winHeight) {
}

Cs300Assignment3::~Cs300Assignment3() {
    CleanUp();
}

int Cs300Assignment3::Init() {
    CleanUp();

    if (!mSceneCmp_Lightmanager) {
        mSceneCmp_Lightmanager = new SceneCmp_LightManager();
        mSceneCmp_Lightmanager->Init(SceneCmp_LightManager::ScenarioType::Scenario_Cs300A1);
    }
    if (!mSceneCmp_CameraManager) {
        mSceneCmp_CameraManager = new SceneCmp_CameraManager();
        mSceneCmp_CameraManager->Init();
    }

    if (!mSceneCmp_ModelLoader) {
        mSceneCmp_ModelLoader = new SceneCmp_ModelLoader();
        mSceneCmp_ModelLoader->Init(false);
    }

    if (!mSceneCmp_CollisionManager) {
        mSceneCmp_CollisionManager = new SceneCmp_CollisionManager();
        mSceneCmp_CollisionManager->Init();
    }

    glGenFramebuffers(6, mDynamicEnvFbos);
    glGenRenderbuffers(6, mDynamicEnvRbos);
    glGenTextures(6, mDynamicEnvTxos);

    for (int i = 0; i < 6; ++ i ) {
        glBindTexture(GL_TEXTURE_2D, mDynamicEnvTxos[i]);
        glTexImage2D(GL_TEXTURE_2D,
                        0,
                        GL_RGBA,
                        mDynamicEnvMapTexSize,
                        mDynamicEnvMapTexSize,
                        0,
                        GL_RGBA,
                        GL_UNSIGNED_BYTE,
                        NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }

    if (mSkyBox == nullptr) {
        mSkyBox = new SkyBox();
    }


    return Scene::Init();
}

void Cs300Assignment3::CleanUpModel() {
    if (mSceneCmp_ModelLoader)
        mSceneCmp_ModelLoader->CleanUpModel();
    if (mSceneCmp_CollisionManager)
        mSceneCmp_CollisionManager->CleanUp();
}

void Cs300Assignment3::CleanUp() {
    if (mDrawAxis) {
        mDrawAxis->CleanUp();
        delete mDrawAxis;
        mDrawAxis = nullptr;
    }

    if (mSceneCmp_ModelLoader) {
        mSceneCmp_ModelLoader->Shutdown();
        delete mSceneCmp_ModelLoader;
        mSceneCmp_ModelLoader = nullptr;
    }

    if (mSceneCmp_Lightmanager) {
        mSceneCmp_Lightmanager->Shutdown();
        delete mSceneCmp_Lightmanager;
        mSceneCmp_Lightmanager = nullptr;
    }
    if (mSceneCmp_CameraManager) {
        mSceneCmp_CameraManager->Shutdown();
        delete mSceneCmp_CameraManager;
        mSceneCmp_CameraManager = nullptr;
    }
    if (mSceneCmp_CollisionManager) {
        mSceneCmp_CollisionManager->Shutdown();
        delete mSceneCmp_CollisionManager;
        mSceneCmp_CollisionManager = nullptr;
    }

    if (mSkyBox) {
        delete mSkyBox;
        mSkyBox = nullptr;
    }

    glDeleteFramebuffers(6, mDynamicEnvFbos);
    glDeleteRenderbuffers(6, mDynamicEnvRbos);
    glDeleteTextures(6, mDynamicEnvTxos);
}

void Cs300Assignment3::UpdateGUI() {
    ImGui::Text("Scene: Cs300Assignment3");
    const ImGuiTabBarFlags tabBarFlags = ImGuiTabBarFlags_None;
   
    if (mSceneCmp_ModelLoader)
        mSceneCmp_ModelLoader->GUI();
    if (mSceneCmp_CameraManager)
        mSceneCmp_CameraManager->UpdateGUI();
    if (mSceneCmp_Lightmanager)
        mSceneCmp_Lightmanager->UpdateGUI();
    if (mSceneCmp_CollisionManager)
        mSceneCmp_CollisionManager->GUI(mSceneCmp_ModelLoader->mModel);

    if (ImGui::BeginTabBar("Model", tabBarFlags)) {
        if (ImGui::BeginTabItem("Render Option")) {
            ImGui::Checkbox("Render Axis",&mEnableDrawAxis);


            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Shader")) {
            ImGui::Text("Model Shader");
            ImGui::BeginChild("Shaders",
                              ImVec2(200.0f, 100.0f),
                              true);
            std::vector<std::string> shaderNames;
            shaderNames.push_back("EnvironmentMapping");
            shaderNames.push_back("PhongShadingWithEnv");
            shaderNames.push_back("PhongFrag");
            {
                int currentIdx = 0;
                for (auto shaderName : shaderNames) {
                    if (ImGui::Selectable(shaderName.c_str(), guiShaderSelectedIdx == currentIdx))
                        guiShaderSelectedIdx = currentIdx;
                    ++currentIdx;
                }
                if (guiShaderSelectedIdx != -1) {
                    if (prevGuiShaderSelectedIdx != guiShaderSelectedIdx) {
                        prevGuiShaderSelectedIdx = guiShaderSelectedIdx;
                        ChangeShader((ModelShaderType)guiShaderSelectedIdx);
                    }
                }
            }
            ImGui::EndChild();

            ImGui::Text("Env Mapping mode");
            ImGui::BeginChild("Env Mapping mode",
                              ImVec2(200.0f, 100.0f),
                              true);
            std::vector<std::string> envModeNames;
            envModeNames.push_back("NotSelected");
            envModeNames.push_back("Only Reflection");
            envModeNames.push_back("Only Refraction");
            envModeNames.push_back("Fresnel Approx");
            {
                int currentIdx = 0;
                for (auto envModeName : envModeNames) {
                    if (ImGui::Selectable(envModeName.c_str(), mGuiEnvMapModeSelectedIdx == currentIdx))
                        mGuiEnvMapModeSelectedIdx = currentIdx;
                    ++currentIdx;
                }
                if (mGuiEnvMapModeSelectedIdx != -1) {
                    if (mPrevGuiEnvMapModeSelectedIdx != mGuiEnvMapModeSelectedIdx) {
                        mPrevGuiEnvMapModeSelectedIdx = mGuiEnvMapModeSelectedIdx;
                        mCurrentEnvMapModeIdx = mGuiEnvMapModeSelectedIdx;
                    }
                }
            }
            ImGui::EndChild();

            if (modelShaderType.has_value()
                && (modelShaderType.value() == ShaderManager::ShaderType::PhongShadingWithEnv)) {
                ImGui::InputFloat("Current nt", &mCurrentNt);
                ImGui::DragFloat("Current phong interpolation factor",
                                    &mCurrentPhongEnvInterpolationFactor,
                                    0.01f,
                                    0.0f,
                                    1.0f);
            }


            if (modelShaderType.has_value()
                && (modelShaderType.value() == ShaderManager::ShaderType::EnvironmentMapping
                    || modelShaderType.value() == ShaderManager::ShaderType::PhongShadingWithEnv)) {
                ImGui::Text("Select Refractive Index");
                ImGui::InputFloat("Current Refractive Ratio", &mCurrentRefractiveIndexRatio);
                ImGui::InputFloat("Current WaveLength Offset R-G", &mCurrentWaveLengthOffsetRG);
                ImGui::InputFloat("Current WaveLength Offset G-B", &mCurrentWaveLengthOffsetGB);
                ImGui::BeginChild("Refractive Index List",
                                  ImVec2(200.0f, 100.0f),
                                  true);
                unordered_map<string, float> refractiveIndices;
                refractiveIndices.insert({"Air", 1.0f / 1.000293f});
                refractiveIndices.insert({"Hydrogen", 1.0f / 1.000132f});
                refractiveIndices.insert({"Water", 1.0f / 1.333f});
                refractiveIndices.insert({"Olive Oil", 1.0f / 1.47f});
                refractiveIndices.insert({"Ice", 1.0f / 1.31f});
                refractiveIndices.insert({"Quartz", 1.0f / 1.46f});
                refractiveIndices.insert({"Diamond", 1.0f / 2.42f});
                refractiveIndices.insert({"Acrylic", 1.0f / 1.49f});
                {
                    int currentIdx = 0;
                    float val = 1.0f;
                    for (auto pair : refractiveIndices) {
                        if (ImGui::Selectable(pair.first.c_str(), mGuiRefractiveIndex == currentIdx)) {
                            mGuiRefractiveIndex = currentIdx;
                            val = pair.second;
                        }
                        ++currentIdx;
                    }
                    if (mGuiRefractiveIndex != -1) {
                        if (mPrevGuiRefractiveIndex != mGuiRefractiveIndex) {
                            mPrevGuiRefractiveIndex = mGuiRefractiveIndex;
                            mCurrentRefractiveIndexRatio = val;
                        }
                    }
                }
                ImGui::EndChild();
            }
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
} 

void Cs300Assignment3::ChangeShader(ModelShaderType modelShaderType_) {
    switch (modelShaderType_) {
    case ModelShaderType::EnvironmentMapping:
        modelShaderType = ShaderManager::ShaderType::EnvironmentMapping;
        break;
    case ModelShaderType::PhongShadingWithEnv:
        modelShaderType = ShaderManager::ShaderType::PhongShadingWithEnv;
        break;
    case ModelShaderType::PhongFrag:
        modelShaderType = ShaderManager::ShaderType::PhongFrag;
        break;
    default:
    break;
    }
}

void Cs300Assignment3::Update() {
    auto camera = mSceneCmp_CameraManager->GetCamera();
    if (camera == nullptr) return;

    if (mSceneCmp_CameraManager)
        mSceneCmp_CameraManager->Update(_windowWidth,
                                        _windowHeight,
                                        mSceneCmp_Lightmanager->GetZNear(),
                                        mSceneCmp_Lightmanager->GetZFar());
    if (mSceneCmp_ModelLoader) {
        mSceneCmp_ModelLoader->Update();
        auto model = mSceneCmp_ModelLoader->mModel;
        if (model && mSceneCmp_Lightmanager)
            mSceneCmp_Lightmanager->Update(model->GetCentroidInWorldSpace(), camera->viewMat);
    }
}

void Cs300Assignment3::RenderDebugObjects(float dt_,
                                            mat4 viewMat_,
                                            mat4 projMat_) {
    auto camera = mSceneCmp_CameraManager->GetCamera();
    if (camera == nullptr) return;

    if (mEnableDrawAxis)
        if (mDrawAxis)
            mDrawAxis->Draw(viewMat_, projMat_);

    if (mSceneCmp_ModelLoader && mSceneCmp_ModelLoader->mModel) {
        mSceneCmp_ModelLoader->mModel->DebugDraw(viewMat_,
                                                 projMat_);
        mSceneCmp_Lightmanager->RenderAllSpheres(viewMat_,
                                                          projMat_,
                                                          glm::vec3(0.0f),
                                                          dt_);
    }

    if (mSceneCmp_CollisionManager)
        mSceneCmp_CollisionManager->DebugDraw();
    if (mSceneCmp_CollisionManager) {
        auto camera = mSceneCmp_CameraManager->GetCamera();
        if (camera != nullptr) {
            mSceneCmp_CollisionManager->Update(glm::identity<mat4>(),
                                               viewMat_,
                                               projMat_);
        }
    }
}

int Cs300Assignment3::Render(float dt_) {
    if (!ShaderManager::get_inst()->mCompiled)
        return 0;
    if (!mSceneCmp_Lightmanager
        || !mSceneCmp_Lightmanager->GetInit()
        || !mSceneCmp_CameraManager
        || !mSceneCmp_CameraManager->GetInit()
        || !mSceneCmp_ModelLoader
        || !mSceneCmp_ModelLoader->GetInit())
        return 0;

    auto camera = mSceneCmp_CameraManager->GetCamera();
    auto model = mSceneCmp_ModelLoader->mModel;



    if (camera == nullptr)
        return 0;

    Update();

    if (model == nullptr)
        return 0;

    glFrontFace(GL_CCW);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

	glDepthMask(GL_TRUE);
	glEnable(GL_DEPTH_TEST);


    if (modelShaderType.has_value()
        &&
        (modelShaderType == ShaderManager::ShaderType::EnvironmentMapping
            || modelShaderType == ShaderManager::ShaderType::PhongShadingWithEnv)) {
        model->mUseEnvMapping = false;
        model->mRefractiveIndexRatio = mCurrentRefractiveIndexRatio;
        model->mRefractiveWaveLengthOffsetRG = mCurrentWaveLengthOffsetRG;
        model->mRefractiveWaveLengthOffsetGB = mCurrentWaveLengthOffsetGB;
        model->mEnvModeIdx = mCurrentEnvMapModeIdx;
        model->mNt = mCurrentNt;
        model->mPhongEnvInterpolationFactor = mCurrentPhongEnvInterpolationFactor;

        GLenum drawBuffers[1] = {
            GL_COLOR_ATTACHMENT0
        };

        /*
        vec3 captureEyes[6] = {
            vec3(5.0f, 0.0f, 0.0f),
            vec3(-5.0f, 0.0f, 0.0f),
            vec3(0.0f, 5.0f, 0.0f),
            vec3(0.0f, -5.0f, 0.0f),
            vec3(0.0f, 0.0f, 5.0f),
            vec3(0.0f, 0.0f, -5.0f)
        };
        */

        vec3 captureEyes[6] = {
            vec3(0.0f, 0.0f, 0.0f),
            vec3(0.0f, 0.0f, 0.0f),
            vec3(0.0f, 0.0f, 0.0f),
            vec3(0.0f, 0.0f, 0.0f),
            vec3(0.0f, 0.0f, 0.0f),
            vec3(0.0f, 0.0f, 0.0f)
        };

        vec3 captureTargets[6] = {
            vec3(-1.0f, 0.0f, 0.0f),
            vec3(1.0f, 0.0f, 0.0f),
            vec3(0.0f, -1.0f, 0.0f),
            vec3(0.0f, 1.0f, 0.0f),
            vec3(0.0f, 0.0f, -1.0f),
            vec3(0.0f, 0.0f, 1.0f)
        };

        vec3 captureUps[6] = {
            vec3(0.0f, 1.0f, 0.0f),
            vec3(0.0f, 1.0f, 0.0f),
            vec3(0.0f, 0.0f, 1.0f),
            vec3(0.0f, 0.0f, -1.0f),
            vec3(0.0f, 1.0f, 0.0f),
            vec3(0.0f, 1.0f, 0.0f)
        };

        mSceneCmp_Lightmanager->UpdateAllSpheres(camera->viewMat,
                                                    mSceneCmp_CameraManager->mProjMat,
                                                    glm::vec3(0.0f),
                                                    dt_);
       
        glViewport(0, 0, mDynamicEnvMapTexSize, mDynamicEnvMapTexSize);
        for (unsigned i = 0; i < 6; ++ i) {
            glBindFramebuffer(GL_FRAMEBUFFER, mDynamicEnvFbos[i]);
            glDrawBuffers(1, drawBuffers);
            glBindRenderbuffer(GL_RENDERBUFFER, mDynamicEnvRbos[i]);
            glRenderbufferStorage(GL_RENDERBUFFER,
                                    GL_DEPTH_COMPONENT,
                                    mDynamicEnvMapTexSize,
                                    mDynamicEnvMapTexSize);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER,
                                        GL_DEPTH_ATTACHMENT,
                                        GL_RENDERBUFFER,
                                        mDynamicEnvRbos[i]);
            

            glBindTexture(GL_TEXTURE_2D, mDynamicEnvTxos[i]);
            glFramebufferTexture2D(GL_FRAMEBUFFER,
                                    GL_COLOR_ATTACHMENT0,
                                    GL_TEXTURE_2D,
                                    mDynamicEnvTxos[i],
                                    0);

            glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


            auto cVMat = glm::lookAt(captureEyes[i],
                                    captureTargets[i],
                                    captureUps[i]);

            auto captureViewMat = cVMat;
            auto captureProjMat = glm::perspective(glm::radians(90.0f),
                                                    1.0f,
                                                    mSceneCmp_Lightmanager->GetZNear(),
                                                    mSceneCmp_Lightmanager->GetZFar());
            
            if (mSkyBox)
                mSkyBox->Draw(captureViewMat, captureProjMat);


            if (mEnableDrawAxis)
                if (mDrawAxis)
                    mDrawAxis->Draw(captureViewMat, captureProjMat);

            if (mSceneCmp_ModelLoader && mSceneCmp_ModelLoader->mModel) {
                mSceneCmp_ModelLoader->mModel->DebugDraw(captureViewMat,
                                                         captureProjMat);
                mSceneCmp_Lightmanager->RenderAllSpheres(captureViewMat,
                                                            captureProjMat,
                                                            glm::vec3(0.0f),
                                                            dt_);
            }

            if (mSceneCmp_CollisionManager)
                mSceneCmp_CollisionManager->DebugDraw();

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }
    }

    glViewport(0, 0, 1600, 900);

    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    glClearDepth(1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


    if (mSkyBox)
        mSkyBox->Draw(camera->viewMat, mSceneCmp_CameraManager->mProjMat);

    RenderDebugObjects(dt_,
                        camera->viewMat,
                        mSceneCmp_CameraManager->mProjMat);
    if (mSceneCmp_CollisionManager) {
        auto camera = mSceneCmp_CameraManager->GetCamera();
        if (camera != nullptr) {
            mSceneCmp_CollisionManager->Update(glm::identity<mat4>(),
                                                camera->viewMat,
                                                mSceneCmp_CameraManager->mProjMat);
        }
    }

    if (modelShaderType.has_value()) {
        switch (modelShaderType.value()) {
        case ShaderManager::ShaderType::EnvironmentMapping:
        case ShaderManager::ShaderType::PhongShadingWithEnv:
            model->mUseEnvMapping = true;
            break;
        case ShaderManager::ShaderType::PhongFrag:
            model->mUseEnvMapping = false;
            break;
        default:
            break;
        }
    }
    model->Draw(camera->viewMat,
                mSceneCmp_CameraManager->mProjMat,
                camera->eyePos,
                ShaderManager::get_inst()->UseShader(modelShaderType.value()),
                mSceneCmp_Lightmanager->phongProps.useFaceNormal,
                mDynamicEnvTxos);
    return 0;
}

int Cs300Assignment3::postRender() {
    return 0;
}



#include "Cs350Assignment1.h"
#include "../Common/shader.hpp"
#include "imgui.h"
#include "../OBJLoader.h"
#include "../ShaderManager.h"
#include "../Model.h"
#include "../DrawTet.h"
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>


#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/ext.hpp>

#include <unordered_map>
#include <vector>
#include <cmath>

Cs350Assignment1::Cs350Assignment1(int winWidth, int winHeight)
    : Scene(winWidth, winHeight) {
}

Cs350Assignment1::~Cs350Assignment1() {
    CleanUp();
}

int Cs350Assignment1::Init() {
    CleanUp();

    if (!mSceneCmp_Lightmanager) {
        mSceneCmp_Lightmanager = new SceneCmp_LightManager();
        mSceneCmp_Lightmanager->Init(SceneCmp_LightManager::ScenarioType::Scenario1_DirectionalLight);
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

    mGbufferFboId = 0;
    DeferredSetUpGBuffer();

    modelShaderType = ShaderManager::ShaderType::DeferredPhongFrag;
    mDrawQuad = new DrawQuad();
    mDrawAxis = new DrawAxis();
    return Scene::Init();
}

void Cs350Assignment1::DeferredSetUpGBuffer() {
    // refer from cs350 slide and OpenGL SuperBible
    glGenFramebuffers(1, &mGbufferFboId);
    glBindFramebuffer(GL_FRAMEBUFFER, mGbufferFboId);

    /*
        // attach 0
        - vec3 ambient -> 4byte * 3 = 12 bytes + EMPTY(4bytes) -> GL_RGBA32F

        // attach 1
        - vec3 diffuse -> 4byte * 3 = 12 bytes + EMPTY(4bytes) -> GL_RGBA32F

        // attach 2
        - vec3 specular -> 4byte * 3 = 12 bytes + char shiness(1byte) -> GL_RGBA32F

        // attach 3
        - vec3 emissive -> 4byte * 3 = 12 bytes + EMPTY(4bytes) -> GL_RGBA32F

        // attach 4
        - vec3 vertNormal -> 4byte * 3 = 12 bytes + EMPTY(4bytes) -> GL_RGBA32F

        // attach 5
        - vec3 vertPos -> 4byte * 3 = 12 bytes + EMPTY(4bytes) -> GL_RGBA32F

        // depth attach
        - float depthValue -> 4 bytes -> GL_DEPTH_COMPONENT32F

        // => 100 bytes per frag

    */
    glGenTextures(NumOfDeferredRenderTargets, mGbufferTexIds);

    // 0 - ambient
    glBindTexture(GL_TEXTURE_2D, mGbufferTexIds[0]);
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_RGBA32F,
                 _windowWidth,
                 _windowHeight,
                 0,
                 GL_RGBA,
                 GL_FLOAT,
                 NULL);
    glTexParameteri(GL_TEXTURE_2D,
                    GL_TEXTURE_MIN_FILTER,
                    GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,
                    GL_TEXTURE_MAG_FILTER,
                    GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER,
                           GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D,
                           mGbufferTexIds[0],
                           0);

    // 1 - diffuse
    glBindTexture(GL_TEXTURE_2D, mGbufferTexIds[1]);
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_RGBA32F,
                 _windowWidth,
                 _windowHeight,
                 0,
                 GL_RGBA,
                 GL_FLOAT,
                 NULL);
    glTexParameteri(GL_TEXTURE_2D,
                    GL_TEXTURE_MIN_FILTER,
                    GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,
                    GL_TEXTURE_MAG_FILTER,
                    GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER,
                           GL_COLOR_ATTACHMENT1,
                           GL_TEXTURE_2D,
                           mGbufferTexIds[1],
                           0);

    // 2 - specular + shiness
    glBindTexture(GL_TEXTURE_2D, mGbufferTexIds[2]);
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_RGBA32F,
                 _windowWidth,
                 _windowHeight,
                 0,
                 GL_RGBA,
                 GL_FLOAT,
                 NULL);
    glTexParameteri(GL_TEXTURE_2D,
                    GL_TEXTURE_MIN_FILTER,
                    GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,
                    GL_TEXTURE_MAG_FILTER,
                    GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER,
                           GL_COLOR_ATTACHMENT2,
                           GL_TEXTURE_2D,
                           mGbufferTexIds[2],
                           0);

    // 3 - emissive
    glBindTexture(GL_TEXTURE_2D, mGbufferTexIds[3]);
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_RGBA32F,
                 _windowWidth,
                 _windowHeight,
                 0,
                 GL_RGBA,
                 GL_FLOAT,
                 NULL);
    glTexParameteri(GL_TEXTURE_2D,
                    GL_TEXTURE_MIN_FILTER,
                    GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,
                    GL_TEXTURE_MAG_FILTER,
                    GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER,
                           GL_COLOR_ATTACHMENT3,
                           GL_TEXTURE_2D,
                           mGbufferTexIds[3],
                           0);

    // 4 - vertNormal
    glBindTexture(GL_TEXTURE_2D, mGbufferTexIds[4]);
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_RGBA32F,
                 _windowWidth,
                 _windowHeight,
                 0,
                 GL_RGBA,
                 GL_FLOAT,
                 NULL);
    glTexParameteri(GL_TEXTURE_2D,
                    GL_TEXTURE_MIN_FILTER,
                    GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,
                    GL_TEXTURE_MAG_FILTER,
                    GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER,
                           GL_COLOR_ATTACHMENT4,
                           GL_TEXTURE_2D,
                           mGbufferTexIds[4],
                           0);

    // 5 - vertPos
    glBindTexture(GL_TEXTURE_2D, mGbufferTexIds[5]);
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_RGBA32F,
                 _windowWidth,
                 _windowHeight,
                 0,
                 GL_RGBA,
                 GL_FLOAT,
                 NULL);
    glTexParameteri(GL_TEXTURE_2D,
                    GL_TEXTURE_MIN_FILTER,
                    GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,
                    GL_TEXTURE_MAG_FILTER,
                    GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER,
                           GL_COLOR_ATTACHMENT5,
                           GL_TEXTURE_2D,
                           mGbufferTexIds[5],
                           0);
    
    //// depth attach
    //glGenRenderbuffers(1, &mGbufferDepthBufferId);
    //glBindRenderbuffer(GL_RENDERBUFFER, mGbufferDepthBufferId);
    //glRenderbufferStorage(GL_RENDERBUFFER,
    //                      GL_DEPTH_COMPONENT,
    //                      mWinWidth,
    //                      mWinHeight);
    //glFramebufferRenderbuffer(GL_FRAMEBUFFER,
    //                          GL_DEPTH_ATTACHMENT,
    //                          GL_RENDERBUFFER,
    //                          mGbufferDepthBufferId);
    GLenum drawBuffers[] = {
        GL_COLOR_ATTACHMENT0,
        GL_COLOR_ATTACHMENT1,
        GL_COLOR_ATTACHMENT2,
        GL_COLOR_ATTACHMENT3,
        GL_COLOR_ATTACHMENT4,
        GL_COLOR_ATTACHMENT5
    };

    glDrawBuffers(6, drawBuffers);

    // depth attach 
    glGenRenderbuffers(1, &mGbufferDepthBufferId);
    glBindRenderbuffer(GL_RENDERBUFFER, mGbufferDepthBufferId);
    glRenderbufferStorage(GL_RENDERBUFFER,
                          GL_DEPTH_COMPONENT,
                          _windowWidth,
                          _windowHeight);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER,
                              GL_DEPTH_ATTACHMENT,
                              GL_RENDERBUFFER,
                              mGbufferDepthBufferId);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        cout << "G buffer program failed to compile" << endl;
        return;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Cs350Assignment1::CleanUpModel() {
    if (mSceneCmp_ModelLoader)
        mSceneCmp_ModelLoader->CleanUpModel();
    if (mSceneCmp_CollisionManager)
        mSceneCmp_CollisionManager->CleanUp();

    if (mDrawAxis) {
        mDrawAxis->CleanUp();
        delete mDrawAxis;
        mDrawAxis = nullptr;
    }
}

void Cs350Assignment1::CleanUp() {
    CleanUpModel();

    if (mSceneCmp_ModelLoader) {
        mSceneCmp_ModelLoader->Shutdown();
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

    if (mDrawQuad) {
        mDrawQuad->CleanUp();
        delete mDrawQuad;
        mDrawQuad = nullptr;
    }

    if (mDrawAxis) {
        mDrawAxis->CleanUp();
        delete mDrawAxis;
        mDrawAxis = nullptr;
    }

    if (mDrawSphere) {
        mDrawSphere->CleanUp();
        delete mDrawSphere;
        mDrawSphere = nullptr;
    }
}

void Cs350Assignment1::ChangeShader(ModelShaderType modelShaderType_) {
    switch (modelShaderType_) {
    case ModelShaderType::PhongVert:
        modelShaderType = ShaderManager::ShaderType::PhongVert;
        break;
    case ModelShaderType::PhongFrag:
        modelShaderType = ShaderManager::ShaderType::PhongFrag;
        break;
    case ModelShaderType::DeferredPhongFrag:
        modelShaderType = ShaderManager::ShaderType::DeferredPhongFrag;
    break;
    default:
    break;
    }
}

void Cs350Assignment1::UpdateGUI() {
    ImGui::Text("Scene: Cs300Assignment1");
    ImGuiTabBarFlags tabBarFlags = ImGuiTabBarFlags_None;
    
    ImGui::Checkbox("Enable GJK debug draw", &mGJKDebugDrawEnabled);
    ImGui::Checkbox("Enable GJK debug also if not colliding", &mDebugEvenNotCollidedOption);
    if (ImGui::BeginTabBar("Model", tabBarFlags)) {
        if (ImGui::BeginTabItem("Render Option")) {
            ImGui::Checkbox("Render Axis",&mEnableDrawAxis);
            //ImGui::Checkbox("Render plane", &enableRenderPlane);

            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Shader")) {
            ImGui::Text("Model Shader");
            ImGui::BeginChild("Shaders",
                              ImVec2(200.0f, 100.0f),
                              true);
            std::vector<std::string> shaderNames;
            shaderNames.push_back("PhongVert");
            shaderNames.push_back("PhongFrag");
            shaderNames.push_back("DeferredPhongFrag");
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

            if (modelShaderType == ShaderManager::ShaderType::DeferredPhongFrag) {
                ImGui::Checkbox("Copy Depth", &mCopyDepthInfo);

                ImGui::Text("Dump GBuffer Type");
                ImGui::BeginChild("Dump GBuffer Type",
                                  ImVec2(200.0f, 100.0f),
                                  true);
                std::vector<std::string> dumpTypeNames;
                dumpTypeNames.push_back("None");
                dumpTypeNames.push_back("Ambient");
                dumpTypeNames.push_back("Diffuse");
                dumpTypeNames.push_back("Specular");
                dumpTypeNames.push_back("Emissive");
                dumpTypeNames.push_back("Normal");
                dumpTypeNames.push_back("Pos");

                int currentDumpSelectedIdx = 0;
                for (auto dumpTypeName : dumpTypeNames) {
                    if (ImGui::Selectable(dumpTypeName.c_str(), mDumpGuiSelectedIdx == currentDumpSelectedIdx))
                        mDumpGuiSelectedIdx = currentDumpSelectedIdx;
                    ++currentDumpSelectedIdx;
                }
                if (mDumpGuiSelectedIdx != -1) {
                    if (mPrevDumpSelectedIdx != mDumpGuiSelectedIdx) {
                        mPrevDumpSelectedIdx = mDumpGuiSelectedIdx;
                        mDumpBufferType = static_cast<DumpBufferType>(mDumpGuiSelectedIdx);
                    }
                }
                ImGui::EndChild();
            }
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    if (mSceneCmp_ModelLoader)
        mSceneCmp_ModelLoader->GUI();
    if (mSceneCmp_CameraManager)
        mSceneCmp_CameraManager->UpdateGUI();
    if (mSceneCmp_Lightmanager)
        mSceneCmp_Lightmanager->UpdateGUI();
    if (mSceneCmp_CollisionManager)
        mSceneCmp_CollisionManager->GUI(mSceneCmp_ModelLoader->mModel);
}

void Cs350Assignment1::Update() {
    auto camera = mSceneCmp_CameraManager->GetCamera();
    if (camera == nullptr) return;

    if (mSceneCmp_CameraManager) {
        mSceneCmp_CameraManager->Update(_windowWidth,
                                        _windowHeight,
                                        mSceneCmp_Lightmanager->GetZNear(),
                                        mSceneCmp_Lightmanager->GetZFar());
    }
    if (!mIsInGJKDebuggingProcess) {
        if (mSceneCmp_ModelLoader) {
            mSceneCmp_ModelLoader->Update();
            auto model = mSceneCmp_ModelLoader->mModel;
            if (model && mSceneCmp_Lightmanager)
                mSceneCmp_Lightmanager->Update(model->GetCentroidInWorldSpace(), camera->viewMat);
        }
    }
}

void Cs350Assignment1::RenderDeferredObjects() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    auto deferredShader = ShaderManager::get_inst()->UseShader(ShaderManager::ShaderType::DeferredPhongFrag);

    vector<string>  gBufferTexNames;
    gBufferTexNames.push_back("GBufferAmbient");
    gBufferTexNames.push_back("GBufferDiffuse");
    gBufferTexNames.push_back("GBufferSpecular");
    gBufferTexNames.push_back("GBufferEmissive");
    gBufferTexNames.push_back("GBufferVertNormal");
    gBufferTexNames.push_back("GBufferVertPos");
    for (unsigned i = 0; i < NumOfDeferredRenderTargets; ++ i) {
        GLint texLoc = glGetUniformLocation(deferredShader, gBufferTexNames[i].c_str());
        if (texLoc == -1) {
            cout << "Wrong tex loc on deferred rendereng" << endl;
            return;
        }
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, mGbufferTexIds[i]);
        glUniform1i(texLoc, i);
    }

    //glEnable(GL_CULL_FACE);
    //glCullFace(GL_BACK);
    mDrawQuad->Draw();
}

void Cs350Assignment1::RenderDebugObjects(float dt_) {
    auto camera = mSceneCmp_CameraManager->GetCamera();
    if (camera == nullptr) return;
    auto projMat = mSceneCmp_CameraManager->mProjMat;

    if (mEnableDrawAxis)
        if (mDrawAxis)
            mDrawAxis->Draw(camera->viewMat, projMat);

    if (mSceneCmp_ModelLoader && mSceneCmp_ModelLoader->mModel
        && !mIsInGJKDebuggingProcess) {
        mSceneCmp_ModelLoader->mModel->DebugDraw(camera->viewMat,
                                                 projMat);
        mSceneCmp_Lightmanager->RenderAndUpdateAllSpheres(camera->viewMat,
                                                          projMat,
                                                          mSceneCmp_ModelLoader->mModel->GetCentroidInWorldSpace(),
                                                          dt_);
    }

    if (mSceneCmp_CollisionManager && !mIsInGJKDebuggingProcess) {
        if (mSceneCmp_CollisionManager) {
            auto camera = mSceneCmp_CameraManager->GetCamera();
            if (camera != nullptr) {
                mSceneCmp_CollisionManager->Update(glm::identity<mat4>(),
                                                   camera->viewMat,
                                                   mSceneCmp_CameraManager->mProjMat);
            }
        }
        mSceneCmp_CollisionManager->DebugDraw();
    }

    if (camera
        && mSceneCmp_ModelLoader->mModel
        && mSceneCmp_CollisionManager
        && mSceneCmp_CollisionManager->mOctree) {
        if (camera->mIsSpaceTriggered) {
            if (!mIsSphereInShooting
                || mShootingElpased >= 10.0f) {
                if (mDrawSphere != nullptr) {
                    delete mDrawSphere;
                    mDrawSphere = nullptr;
                }
                mDrawSphere = new DrawSphere(glm::vec3(0.0f, 1.0f, 0.0f),
                                                camera->eyePos,
                                                0.1f,
                                                10,
                                                10,
                                                false,
                                                true);
            
                mShootingElpased = 0.0f;
                mDrawSphere->mWorldPos = camera->eyePos;
                mSphereMoveDir = glm::normalize(camera->targetPos - camera->eyePos);
                mIsSphereInShooting = true;
                mReqDebugEvenNotCollidedOption = false;
            }
        }

        bool isCollided = false;
        if (mIsSphereInShooting) {
            if (!mIsInGJKDebuggingProcess) {
                const float sphereSpeed = 0.05f;
                mDrawSphere->SetPosition(mDrawSphere->GetPosition() + mSphereMoveDir * sphereSpeed);

                mDebugSimplexes.clear();
                isCollided = DetectCollision_BroadPhase(mDrawSphere,
                                                        mSceneCmp_CollisionManager->mOctree->mRootNode,
                                                        camera->viewMat,
                                                        projMat);
                if (isCollided || mReqDebugEvenNotCollidedOption) {
                    mIsInGJKDebuggingProcess = true;
                    mGJKDebugIterationCount = -1;
                    mReqIncrementDebugGJKCount = true;
                    mDrawSphere->mDrawOnlyAABB = true;
                }
                if (!isCollided) {
                    mShootingElpased += 0.1f;
                }
            }
        }

        if (mIsInGJKDebuggingProcess) {
            if (Camera::mIsSpaceTriggered) {
                mReqIncrementDebugGJKCount = true;
            }

            if (mReqIncrementDebugGJKCount) {
                mReqIncrementDebugGJKCount = false;
                if (mCurrentDrawSimplex) {
                    delete mCurrentDrawSimplex;
                    mCurrentDrawSimplex = nullptr;
                }
                if ((mGJKDebugIterationCount + 1) < mDebugSimplexes.size()) {
                    ++ mGJKDebugIterationCount;
                    auto debugSimplex = mDebugSimplexes[mGJKDebugIterationCount];
                    auto color = mReqDebugEvenNotCollidedOption ? glm::vec3(1.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.0f, 0.0f);
                    switch (debugSimplex.size()) {
                    case 1:
                        mCurrentDrawSimplex = new DrawPoint(color, debugSimplex[0]);
                        break;
                    case 2:
                        mCurrentDrawSimplex = new DrawPolygon(color,
                                                               debugSimplex);
                        break;
                    case 3:
                        mCurrentDrawSimplex = new DrawTriangle(color,
                                                               debugSimplex[0],
                                                               debugSimplex[1],
                                                               debugSimplex[2]);
                        break;
                    case 4:
                        mCurrentDrawSimplex = new DrawTet(color,
                                                          debugSimplex[0],
                                                          debugSimplex[1],
                                                          debugSimplex[2],
                                                          debugSimplex[3]);
                        break;
                    default:
                        assert(false);
                        break;
                    }
                } else {
                    mIsInGJKDebuggingProcess = false;
                    if (mDrawSphere) {
                        delete mDrawSphere;
                        mDrawSphere = nullptr;
                        mIsSphereInShooting = false;
                        mReqDebugEvenNotCollidedOption = false;
                    }
                }
            }
            if (mCurrentDrawSimplex) {
                mCurrentDrawSimplex->Draw(glm::identity<mat4>(),
                                          camera->viewMat,
                                          projMat);
                if (mIntersectingBV) {
                    mIntersectingBV->Draw(glm::identity<mat4>(),
                                              camera->viewMat,
                                              projMat);
                }
            }
        }

        if (mDrawSphere) {
            mDrawSphere->Draw(glm::identity<mat4>(),
                              camera->viewMat,
                              projMat);
        }
    }
}

bool Cs350Assignment1::DetectCollision_BroadPhase(DrawSphere* sphere_,
                                                  OctTree::TreeNode* rootNode_,
                                                  glm::mat4 viewMat_,
                                                  glm::mat4 projMat_) {
    if (sphere_ == nullptr || sphere_->mDrawAABB == nullptr || rootNode_ == nullptr)
        return false;

    // The broad phase part of collision detection
    AABB_2 worldBox = rootNode_->mDrawAABB->mDrawAABB;

    // standard box-box intersection
    if (!Collision::Check(&worldBox, &sphere_->mDrawAABB->mDrawAABB))
        return false;

    // boxes collide, move to mid-phase
    // In mid-phase, we check for intersection of S(sphere) with internal nodes
    if (rootNode_->mChildren.size() > 0) {
        for (unsigned i = 0; i < rootNode_->mChildren.size(); ++ i) {
            auto childNode = rootNode_->mChildren[i];
            // For BVH and Octree: box-box collision
            if (DetectCollision_MidPhase(sphere_, childNode, viewMat_, projMat_))
                return true;
        }
    } else if (DetectCollision_MidPhase(sphere_, rootNode_, viewMat_, projMat_))
            return true;

    // no intersection with the scene
    return false;
}

bool Cs350Assignment1::DetectCollision_MidPhase(DrawSphere* sphere_,
                                                OctTree::TreeNode* treeNode_,
                                                glm::mat4 viewMat_,
                                                glm::mat4 projMat_) {
    if (sphere_ == nullptr || treeNode_ == nullptr)
        return false;

    // if treeNode == LEAF, then perform GJK
    // Narrow-phase calculations here
    if (treeNode_->IsLeafNode()) {
        // Potential approaches for GJK with S(sphere)
        // a. (pre-)Calculate the convex hull of treeNode geometry – most precise calculation
        // b. use BBox of treeNode geometry – approximate solution
        // c. Level-of-detail based pseudo-geometry for dynamic objects
        // We will use approach b. for our assignment
        if (DetectCollision_GJK(sphere_->mDrawAABB->mDrawAABB, treeNode_, viewMat_, projMat_)) {
            // Render polygons of treeNode & sphere in RED color
            return true;
        }
        return false;
    }

    AABB_2 nodeBox = treeNode_->mDrawAABB->mDrawAABB;
       
    // standard box-box intersection
    // if (!AABB_AABB(nodeBox, sphereBox)
    if(!Collision::Check(&nodeBox, &sphere_->mDrawAABB->mDrawAABB))
        return false;

    // Now recurse through the child nodes
    for (unsigned i = 0; i < treeNode_->mChildren.size(); ++ i) {
        auto childNode = treeNode_->mChildren[i];
        // For BVH and Octree: box-box collision
        if (DetectCollision_MidPhase(sphere_, childNode, viewMat_, projMat_))
            return true;
    }
    
    // no intersection with the scene
    return false;
}

bool Cs350Assignment1::DetectCollision_GJK(AABB_2 sphereBox_,
                                           OctTree::TreeNode* treeNode_,
                                           glm::mat4 viewMat_,
                                           glm::mat4 projMat_) {
    if (treeNode_ == nullptr)
        return false;

    // first check node's aabb collision by standard box-box collision
    AABB_2 nodeBox = treeNode_->mDrawAABB->mDrawAABB;

    // standard box-box intersection
    // if (!AABB_AABB(nodeBox, sphereBox)
    if(!Collision::Check(&nodeBox, &sphereBox_))
        return false;

    // check object's aabb
    for (unsigned i = 0;  i < treeNode_->mObjects.size(); ++ i) {
        auto object = treeNode_->mObjects[i];
        mIntersectingBV = object->mBV->mDrawAABB;
        mDebugSimplexes.clear();
        auto dist = CheckGJK(sphereBox_, mIntersectingBV->mDrawAABB, viewMat_, projMat_);
        if (dist.has_value() && dist.value() <= glm::epsilon<float>())
            return true;
    }
    
    return false;
}

optional<float> Cs350Assignment1::CheckGJK(AABB_2 aabbA_,
                                 AABB_2 aabbB_,
                                 glm::mat4 viewMat_,
                                 glm::mat4 projMat_) {
    vector<vec3> aVerts;
    aVerts.push_back(vec3(aabbA_.mHalfExtents.x,
                     aabbA_.mHalfExtents.y,
                     -aabbA_.mHalfExtents.z)); // A-0
    aVerts.push_back(vec3(aabbA_.mHalfExtents.x,
                     aabbA_.mHalfExtents.y,
                     aabbA_.mHalfExtents.z)); // B-1
    aVerts.push_back(vec3(aabbA_.mHalfExtents.x,
                     -aabbA_.mHalfExtents.y,
                     aabbA_.mHalfExtents.z)); // C-2
    aVerts.push_back(vec3(aabbA_.mHalfExtents.x,
                     -aabbA_.mHalfExtents.y,
                     -aabbA_.mHalfExtents.z)); // D-3
    aVerts.push_back(vec3(-aabbA_.mHalfExtents.x,
                     aabbA_.mHalfExtents.y,
                     aabbA_.mHalfExtents.z)); // E-4
    aVerts.push_back(vec3(-aabbA_.mHalfExtents.x,
                     -aabbA_.mHalfExtents.y,
                     aabbA_.mHalfExtents.z)); // F-5
    aVerts.push_back(vec3(-aabbA_.mHalfExtents.x,
                     aabbA_.mHalfExtents.y,
                     -aabbA_.mHalfExtents.z)); // G-6
    aVerts.push_back(vec3(-aabbA_.mHalfExtents.x,
                     -aabbA_.mHalfExtents.y,
                     -aabbA_.mHalfExtents.z)); // H-7

    vector<vec3> bVerts;
    bVerts.push_back(vec3(aabbB_.mHalfExtents.x,
                        aabbB_.mHalfExtents.y,
                        -aabbB_.mHalfExtents.z)); // A-0
    bVerts.push_back(vec3(aabbB_.mHalfExtents.x,
                        aabbB_.mHalfExtents.y,
                        aabbB_.mHalfExtents.z)); // B-1
    bVerts.push_back(vec3(aabbB_.mHalfExtents.x,
                        -aabbB_.mHalfExtents.y,
                        aabbB_.mHalfExtents.z)); // C-2
    bVerts.push_back(vec3(aabbB_.mHalfExtents.x,
                        -aabbB_.mHalfExtents.y,
                        -aabbB_.mHalfExtents.z)); // D-3
    bVerts.push_back(vec3(-aabbB_.mHalfExtents.x,
                        aabbB_.mHalfExtents.y,
                        aabbB_.mHalfExtents.z)); // E-4
    bVerts.push_back(vec3(-aabbB_.mHalfExtents.x,
                        -aabbB_.mHalfExtents.y,
                        aabbB_.mHalfExtents.z)); // F-5
    bVerts.push_back(vec3(-aabbB_.mHalfExtents.x,
                        aabbB_.mHalfExtents.y,
                        -aabbB_.mHalfExtents.z)); // G-6
    bVerts.push_back(vec3(-aabbB_.mHalfExtents.x,
                        -aabbB_.mHalfExtents.y,
                        -aabbB_.mHalfExtents.z)); // H-7
                                                
    vector<vec3> simplexSet;
    simplexSet.push_back(bVerts[0]);
    simplexSet.push_back(bVerts[1]);
    simplexSet.push_back(bVerts[2]);
    simplexSet.push_back(bVerts[3]);

    if (mGJKDebugDrawEnabled) {
        if (mDebugEvenNotCollidedOption)
            mReqDebugEvenNotCollidedOption = true;
        mDebugSimplexes.push_back(simplexSet);
    }

    while (true) {
        vec3 p = vec3(0.0f);
        optional<unsigned> idxToReduce;
        switch (simplexSet.size()) {
        case 1:
            p = Collision::ClosestPointOnPoint(glm::vec3(0.0f),
                                               simplexSet[0]);
            break;
        case 2:
            p = Collision::ClosestPointOnLineSegment(glm::vec3(0.0f),
                                                     simplexSet[0],
                                                     simplexSet[1],
                                                     idxToReduce);
            break;
        case 3:
            p = Collision::ClosestPointOnTriangle(glm::vec3(0.0f),
                                                  simplexSet[0],
                                                  simplexSet[1],
                                                  simplexSet[2],
                                                  idxToReduce);
            break;
        case 4:
            p = Collision::ClosestPointOnTetrahedron(glm::vec3(0.0f),
                                                     simplexSet[0],
                                                     simplexSet[1],
                                                     simplexSet[2],
                                                     simplexSet[3],
                                                     idxToReduce);
            break;
        default:
            assert(false);
        }

        // if P is origin
        if (glm::length(p) <= glm::epsilon<float>())
            return 0.0f;

        // reduce Q to smaller subset
        if (idxToReduce.has_value()) {
            auto erasedVert = simplexSet[idxToReduce.value()];
            simplexSet.erase(simplexSet.begin() + idxToReduce.value());

            if (mGJKDebugDrawEnabled) {
                mDebugSimplexes.push_back(simplexSet);
            }
        } /*else
            assert(false);*/

        // find supporting point
        auto findingDir = -p;
        vec3 vA = aVerts[Collision::FindSupportingPointIdx(findingDir, aVerts)];
        auto foundIdx = Collision::FindSupportingPointIdx(findingDir, bVerts);
        vec3 vB = bVerts[foundIdx];

        auto v = vA - vB;
        // check no more extreme points
        for (unsigned i = 0; i < simplexSet.size(); ++ i)
            if (glm::length(simplexSet[i] - v) <= numeric_limits<float>::epsilon())
                return glm::length(p);
        simplexSet.push_back(v);

        if (mGJKDebugDrawEnabled) {
            mDebugSimplexes.push_back(simplexSet);
        }
    }

    return nullopt;
}

void Cs350Assignment1::DeferredWriteToGBuffer() {
    // write to GBuffer
    glBindFramebuffer(GL_FRAMEBUFFER, mGbufferFboId);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    const auto writeGBufferProgram = ShaderManager::get_inst()->UseShader(ShaderManager::ShaderType::WriteGBufferPhong);
    if (mSceneCmp_ModelLoader && mSceneCmp_ModelLoader->mModel && !mIsInGJKDebuggingProcess) {
        mSceneCmp_ModelLoader->mModel->Draw(mSceneCmp_CameraManager->GetCamera()->viewMat,
                                            mSceneCmp_CameraManager->mProjMat,
                                            mSceneCmp_CameraManager->GetCamera()->eyePos,
                                            writeGBufferProgram);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Cs350Assignment1::DeferredDumpGBuffer() {
    auto dumpGBufferShader = ShaderManager::get_inst()->UseShader(ShaderManager::ShaderType::DumpGBuffer);
    GLint inputTexLoc = glGetUniformLocation(dumpGBufferShader, "inputTex");
    if (inputTexLoc == -1)
        return;

    if (mDumpBufferType == DumpBufferType::Ambient) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, mGbufferTexIds[0]);
        glUniform1i(inputTexLoc, 0);
    } else if (mDumpBufferType == DumpBufferType::Diffuse) {
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, mGbufferTexIds[1]);
        glUniform1i(inputTexLoc, 1);
    } else if (mDumpBufferType == DumpBufferType::Specular) {
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, mGbufferTexIds[2]);
        glUniform1i(inputTexLoc, 2);
    } else if (mDumpBufferType == DumpBufferType::Emissive) {
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, mGbufferTexIds[3]);
        glUniform1i(inputTexLoc, 3);
    } else if (mDumpBufferType == DumpBufferType::Normal) {
        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_2D, mGbufferTexIds[4]);
        glUniform1i(inputTexLoc, 4);
    } else if (mDumpBufferType == DumpBufferType::Pos) {
        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_2D, mGbufferTexIds[5]);
        glUniform1i(inputTexLoc, 5);
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    mDrawQuad->Draw();
}

int Cs350Assignment1::Render(float dt_) {
    if (!ShaderManager::get_inst()->mCompiled)
        return 0;
    if (!mSceneCmp_Lightmanager
        || !mSceneCmp_Lightmanager->GetInit()
        || !mSceneCmp_CameraManager
        || !mSceneCmp_CameraManager->GetInit()
        || !mSceneCmp_ModelLoader
        || !mSceneCmp_ModelLoader->GetInit())
        return 0;

    Update();

    auto camera = mSceneCmp_CameraManager->GetCamera();
    auto model = mSceneCmp_ModelLoader->mModel;

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (mDrawQuad == nullptr
        || camera == nullptr)
        return 0;

    switch (modelShaderType.value()) {
    case ShaderManager::ShaderType::PhongVert:
    case ShaderManager::ShaderType::PhongFrag:
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        if (model != nullptr && !mIsInGJKDebuggingProcess) {
            model->Draw(camera->viewMat,
                        mSceneCmp_CameraManager->mProjMat,
                        camera->eyePos,
                        ShaderManager::get_inst()->UseShader(modelShaderType.value()));
        }
        RenderDebugObjects(dt_);
        break;
    case ShaderManager::ShaderType::DeferredPhongFrag:
        DeferredWriteToGBuffer();
        if (mDumpBufferType != DumpBufferType::None) {
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glEnable(GL_DEPTH_TEST);
            DeferredDumpGBuffer();
        } else {
            RenderDeferredObjects();
            if (mCopyDepthInfo) {
                glBindFramebuffer(GL_READ_FRAMEBUFFER, mGbufferFboId);
                glBlitFramebuffer(0, 0, _windowWidth, _windowHeight, // src
                                                               // bottom left
                                  0, 0, // dst_min
                                  _windowWidth, _windowHeight, // dst_max
                                  GL_DEPTH_BUFFER_BIT,
                                  GL_NEAREST); // GL_LINEAR
                glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
            }
            RenderDebugObjects(dt_);
        }
        break;
    default:
        break;
    }
    return 0;
}

int Cs350Assignment1::postRender() {
    return 0;
}



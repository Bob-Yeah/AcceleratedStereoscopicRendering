/*
  Copyrighted(c) 2020, TH Köln.All rights reserved. Redistribution and use in source and binary forms, with or without modification,
  are permitted provided that the following conditions are met :

  * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
  * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer
    in the documentation and/or other materials provided with the distribution.
  * Neither the name of TH Köln nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.IN NO EVENT SHALL THE COPYRIGHT OWNER
  OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES(INCLUDING, BUT NOT LIMITED TO,
  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
  EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

  Authors: Niko Wissmann
 */

#include "StereoCameraController.h"
#include "Utils/Math/FalcorMath.h"
#include "glm/gtx/string_cast.hpp"


bool StereoCameraController::onMouseEvent(const MouseEvent & mouseEvent)
{
    bool handled = false;
    switch (mouseEvent.type)
    {
    case MouseEvent::Type::LeftButtonDown:
        mLastMousePos = mouseEvent.pos;
        mIsLeftButtonDown = true;
        handled = true;
        break;
    case MouseEvent::Type::LeftButtonUp:
        handled = mIsLeftButtonDown;
        mIsLeftButtonDown = false;
        break;
    case MouseEvent::Type::RightButtonDown:
        mLastMousePos = mouseEvent.pos;
        mIsRightButtonDown = true;
        handled = true;
        break;
    case MouseEvent::Type::RightButtonUp:
        handled = mIsRightButtonDown;
        mIsRightButtonDown = false;
        break;
    case MouseEvent::Type::Move:
        if (mIsLeftButtonDown || mIsRightButtonDown)
        {
            mMouseDelta = mouseEvent.pos - mLastMousePos;
            mLastMousePos = mouseEvent.pos;
            mShouldRotate = true;
            handled = true;
        }
        break;
    default:
        break;
    }

    return handled;
}

bool StereoCameraController::onKeyEvent(const KeyboardEvent & keyboardEvent)
{
    bool handled = false;
    bool keyPressed = (keyboardEvent.type == KeyboardEvent::Type::KeyPressed);

    switch (keyboardEvent.key)
    {
    case KeyboardEvent::Key::W:
        mMovement[Direction::Forward] = keyPressed;
        handled = true;
        break;
    case KeyboardEvent::Key::S:
        mMovement[Direction::Backward] = keyPressed;
        handled = true;
        break;
    case KeyboardEvent::Key::A:
        mMovement[Direction::Right] = keyPressed;
        handled = true;
        break;
    case KeyboardEvent::Key::D:
        mMovement[Direction::Left] = keyPressed;
        handled = true;
        break;
    case KeyboardEvent::Key::Q:
        mMovement[Direction::Down] = keyPressed;
        handled = true;
        break;
    case KeyboardEvent::Key::E:
        mMovement[Direction::Up] = keyPressed;
        handled = true;
        break;
    default:
        break;
    }

    mSpeedModifier = 1.0f;
    if (keyboardEvent.mods.isCtrlDown) mSpeedModifier = 0.25f;
    else if (keyboardEvent.mods.isShiftDown) mSpeedModifier = 10.0f;

    return handled;
}

bool StereoCameraController::update()
{
    mTimer.update();

    bool dirty = false;
    if (mpMonoCam)
    {
        if (mShouldRotate)
        {
            glm::vec3 camPos = mpMonoCam->getPosition();
            glm::vec3 camTarget = mpMonoCam->getTarget();
            glm::vec3 camUp = glm::vec3(0, 1, 0);;

            glm::vec3 viewDir = glm::normalize(camTarget - camPos);
            if (mIsLeftButtonDown)
            {
                glm::vec3 sideway = glm::cross(viewDir, normalize(camUp));

                // Rotate around x-axis
                glm::quat qy = glm::angleAxis(mMouseDelta.y * mSpeedModifier, sideway);
                glm::mat3 rotY(qy);
                viewDir = viewDir * rotY;
                camUp = camUp * rotY;

                // Rotate around y-axis
                glm::quat qx = glm::angleAxis(mMouseDelta.x * mSpeedModifier, camUp);
                glm::mat3 rotX(qx);
                viewDir = viewDir * rotX;

                mpMonoCam->setTarget(camPos + viewDir);
                mpMonoCam->setUpVector(camUp);
                dirty = true;
            }

            mShouldRotate = false;
        }

        if (mMovement.any())
        {
            glm::vec3 movement(0, 0, 0);
            movement.z += mMovement.test(Direction::Forward) ? 1 : 0;
            movement.z += mMovement.test(Direction::Backward) ? -1 : 0;
            movement.x += mMovement.test(Direction::Left) ? 1 : 0;
            movement.x += mMovement.test(Direction::Right) ? -1 : 0;
            movement.y += mMovement.test(Direction::Up) ? 1 : 0;
            movement.y += mMovement.test(Direction::Down) ? -1 : 0;

            glm::vec3 camPos = mpMonoCam->getPosition();
            glm::vec3 camTarget = mpMonoCam->getTarget();
            glm::vec3 camUp = mpMonoCam->getUpVector();

            glm::vec3 viewDir = normalize(camTarget - camPos);
            glm::vec3 sideway = glm::cross(viewDir, normalize(camUp));

            float elapsedTime = mTimer.getElapsedTime();

            float curMove = mSpeedModifier * mSpeed * elapsedTime;
            camPos += movement.z * curMove * viewDir;
            camPos += movement.x * curMove * sideway;
            camPos += movement.y * curMove * camUp;

            camTarget = camPos + viewDir;

            mpMonoCam->setPosition(camPos);
            mpMonoCam->setTarget(camTarget);
            dirty = true;
        }

        mpCamera->setRightEyeMatrices(mpCamera->getViewMatrix(), mpCamera->getProjMatrix());
        calcStereoParams();
    }

    return dirty;
}

void StereoCameraController::attachCamera(const Camera::SharedPtr & pCamera)
{
    mpMonoCam = pCamera;
    if(mpCamera == nullptr)
        mpCamera = Camera::create();
    mpCamera->setName("Stereo Cam");
}

Camera::SharedPtr StereoCameraController::getStereoCamera()
{
    return mpCamera;
}

inline void StereoCameraController::calcStereoParams()
{
    fovrad = focalLengthToFovY(mpMonoCam->getFocalLength(), mpMonoCam->getFrameHeight());
    
    top = mpMonoCam->getNearPlane() * glm::tan(fovrad / 2.f);
#ifdef _DEBUG_LOG
    logWarning("top:" + std::to_string(top));
#endif
    bottom = -top;
    right = mpMonoCam->getAspectRatio() * top;

    left = -right;
#ifdef _DEBUG_LOG
    logWarning("left:" + std::to_string(left));
#endif
    offset = ipd / 2.f*(mpMonoCam->getNearPlane() / z0);
#ifdef _DEBUG_LOG
    logWarning("offset:" + std::to_string(offset));
#endif
    //offset = ipd / 2.f;
    // 
    projectionMatLeft = glm::frustum(left + offset, right + offset, bottom, top, mpMonoCam->getNearPlane(), mpMonoCam->getFarPlane());
#ifdef _DEBUG_LOG
    logWarning("projectionMatLeft:" + glm::to_string(projectionMatLeft));
#endif
    // -0.105545, 0.102445
    projectionMatRight = glm::frustum(left - offset, right - offset, bottom, top, mpMonoCam->getNearPlane(), mpMonoCam->getFarPlane());
#ifdef _DEBUG_LOG
    logWarning("projectionMatRight:" + glm::to_string(projectionMatRight));
#endif
    viewMatLeft = glm::translate(glm::mat4(), glm::vec3(ipd / 2.f, 0, 0)) * mpMonoCam->getViewMatrix();
#ifdef _DEBUG_LOG
    logWarning("getViewMatrix:" + glm::to_string(mpMonoCam->getViewMatrix()));
    logWarning("viewMatLeft:" + glm::to_string(viewMatLeft));
#endif
    viewMatRight = glm::translate(glm::mat4(), glm::vec3(-ipd / 2.f, 0, 0)) * mpMonoCam->getViewMatrix();

    mpCamera->setProjectionMatrix(projectionMatLeft);
    mpCamera->setViewMatrix(viewMatLeft);
    mpCamera->setRightEyeMatrices(viewMatRight, projectionMatRight);
    mpCamera->setPosition(mpMonoCam->getPosition());
    mpCamera->setDepthRange(mpMonoCam->getNearPlane(), mpMonoCam->getFarPlane());
    mpCamera->setTarget(mpMonoCam->getTarget());
    mpCamera->setUpVector(mpMonoCam->getUpVector());
}

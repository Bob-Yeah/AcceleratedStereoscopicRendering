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

#pragma once
#include "Falcor.h"

using namespace Falcor;

// Camera controller for side-by-side view. Calculates asymmetric view frustum parameters
class StereoCameraController : public CameraController
{
public:

    bool onMouseEvent(const MouseEvent& mouseEvent) override;
    bool onKeyEvent(const KeyboardEvent& keyboardEvent) override;
    bool update() override;
    void attachCamera(const Camera::SharedPtr& pCamera) override;

    Camera::SharedPtr getStereoCamera();

    float ipd = 0.062f;
    float z0 = 2.f;

private:
    bool mIsLeftButtonDown = false;
    bool mIsRightButtonDown = false;
    bool mShouldRotate = false;

    Camera::SharedPtr mpMonoCam;

    // Stereo Camera Parameter
    float fovrad;
    float top;
    float bottom;
    float right;
    float left;
    float offset;
    float4x4 projectionMatLeft;
    float4x4 projectionMatRight;
    float4x4 viewMatLeft;
    float4x4 viewMatRight;

    void setIPD(float ipd) { this->ipd = ipd; }
    void setZ0(float z0) { this->z0 = z0; }
    inline void calcStereoParams();

    glm::vec2 mLastMousePos;
    glm::vec2 mMouseDelta;

    CpuTimer mTimer;

    enum Direction
    {
        Forward,
        Backward,
        Right,
        Left,
        Up,
        Down,
        Count
    };

    std::bitset<Direction::Count> mMovement;

    float mSpeedModifier = 1.0f;
};


/*
 *  cam_opencv.h
 *  PHD Guiding
 *
 *  Created by Craig Stark.
 *  Copyright (c) 2013 Craig Stark.
 *  Ported to PHD2 by Bret McKee.
 *  Copyright (c) 2013 Bret McKee.
 *  All rights reserved.
 *
 *  This source code is distributed under the following "BSD" license
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *    Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *    Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *    Neither the name of Craig Stark, Stark Labs nor the names of its
 *     contributors may be used to endorse or promote products derived from
 *     this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef CAM_V4L2_H_INCLUDED
#define CAM_V4L2_H_INCLUDED

#include <stdio.h>

#define IMG_WIDTH 648
#define IMG_HEIGHT 480

#define CLEAR(x) memset(&(x), 0, sizeof(x))

typedef struct {
    void   *start;
    size_t  length;
} membuf_t;

class CameraV4L2 : public GuideCamera
{
    long long total_time = 0;
    long total_frame = 0;

    membuf_t membuf[3];
    int fd = -1;
    const char dev_name[12] = "/dev/video5";
    bool streamming = false;
private:
    int open_device(void);
    int init_device(void);
    int xioctl(int fh, int request, void *arg);
    int start_capturing(void);
    int init_mmap(void);

public:
    CameraV4L2();
    ~CameraV4L2();

    bool    Capture(int duration, usImage& img, int options, const wxRect& subframe) override;
    bool    Connect(const wxString& camId) override;
    bool    Disconnect() override;
    bool HasNonGuiCapture() override { return true; }
    wxByte  BitsPerPixel() override;
};

#endif

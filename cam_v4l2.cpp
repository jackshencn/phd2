
#include "phd.h"
#include "camera.h"

#include "cam_v4l2.h"
#include <sys/syscall.h> 
#include <unistd.h>
#include <sys/time.h>

#include <linux/videodev2.h>

#include <sys/ioctl.h>

#ifdef V4L2_CAMERA

#define IMX334_W 648
#define IMX334_H 480

const char V4L2_LAUNCH[64] = "stdbuf -o0 v4l2-ctl -d /dev/video5 --stream-mmap --stream-to=-";
const char V4L2_SET_EXP[64] = "v4l2-ctl -d /dev/video5 --set-ctrl=exposure=";

CameraV4L2::CameraV4L2()
{
    Name=_T("V4L2 Camera");
    FullSize = wxSize(IMX334_W, IMX334_H);
    m_hasGuideOutput = false;
    v4l2_pipe = NULL;
}

CameraV4L2::~CameraV4L2(void)
{
}

wxByte CameraV4L2::BitsPerPixel()
{
    // YUV422 Y only is 8-bit
    return 8;
}

bool CameraV4L2::Connect(const wxString& camId) {
    Connected = true;
    return false;;
}

bool CameraV4L2::Disconnect() {
    if (v4l2_pipe) {
        pclose(v4l2_pipe);
        v4l2_pipe = NULL;
    }
    Connected = false;
    return false;
}

bool CameraV4L2::Capture(int duration, usImage& img, int options, const wxRect& subframe) {
    if (!v4l2_pipe) {
        v4l2_pipe = popen(V4L2_LAUNCH, "r");
printf("TID %i \n", syscall(SYS_gettid));
    }

    struct timeval cur_time;
    gettimeofday(&cur_time, NULL);
    long long milliseconds = cur_time.tv_sec*1000LL + cur_time.tv_usec/1000;
    printf("Triggered %ims\n", milliseconds - prev_time);
    prev_time = milliseconds;

    if (v4l2_pipe) {
        if (img.Init(IMX334_W, IMX334_H)) {
            pFrame->Alert(_("Memory allocation error"));
            throw ERROR_INFO("img.Init failed");
        }

        fread(img.ImageData, sizeof(unsigned short), IMX334_W * IMX334_H, v4l2_pipe);

        for (int i = 0; i < img.NPixels; i++) {
            // Capture only Y channel
            img.ImageData[i] &= 0xFF;
        }
    } else {
        return true;
    }

    return false;
}

#endif

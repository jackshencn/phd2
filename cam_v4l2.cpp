
#include "phd.h"
#include "camera.h"

#include "cam_v4l2.h"
#include <sys/syscall.h> 
#include <unistd.h>
#include <sys/time.h>

#include <linux/videodev2.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#ifdef V4L2_CAMERA


CameraV4L2::CameraV4L2()
{
    Name=_T("V4L2 Camera");
    FullSize = wxSize(IMG_WIDTH, IMG_HEIGHT);
    m_hasGuideOutput = false;
}

CameraV4L2::~CameraV4L2(void)
{
}

int CameraV4L2::xioctl(int fh, int request, void *arg) {
    int r;

    do {
        r = ioctl(fh, request, arg);
    } while (-1 == r && EINTR == errno);

    return r;
}

int CameraV4L2::start_capturing(void) {
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    if (-1 == xioctl(fd, VIDIOC_STREAMON, &type)) {
        printf("error VIDIOC_STREAMON");
        return 1;
    } else {
        streamming = true;
        return 0;
    }
}

int CameraV4L2::init_mmap(void) {
    struct v4l2_requestbuffers req;
    unsigned num_planes = 1;

    CLEAR(req);

    req.count = MEMBUF_COUNT;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    req.memory = V4L2_MEMORY_MMAP;

    if (-1 == xioctl(fd, VIDIOC_REQBUFS, &req)) {
        if (EINVAL == errno) {
            fprintf(stderr, "%s does not support "
                 "memory mapping", dev_name);
        } else {
            printf("errno VIDIOC_REQBUFS");
        }
        return 1;
    }

    if (req.count != MEMBUF_COUNT) {
        fprintf(stderr, "Insufficient buffer memory on %s\\n",
             dev_name);
        return 1;
    }

    CLEAR(membuf);

    for (int buf_idx = 0; buf_idx < req.count; ++buf_idx) {
        struct v4l2_plane planes[VIDEO_MAX_PLANES]={0};
        struct v4l2_buffer buf;

        CLEAR(buf);

        buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        buf.memory      = V4L2_MEMORY_MMAP;
        buf.index       = buf_idx;
        buf.m.planes = planes;
        buf.length = VIDEO_MAX_PLANES;

        if (-1 == xioctl(fd, VIDIOC_QUERYBUF, &buf))
            printf("VIDIOC_QUERYBUF");

        num_planes = buf.length;
        for(int i = 0 ; i < num_planes; i++) {
            membuf[buf_idx * num_planes + i].length = planes[i].length;
            membuf[buf_idx * num_planes + i].start = mmap(NULL,
                        planes[i].length,
                        PROT_READ | PROT_WRITE, MAP_SHARED,
                        fd,planes[i].m.mem_offset
                );
            printf("map buf %i plane %i 0x%x @ 0x%lx\n", buf_idx, i,
                (uint32_t) membuf[buf_idx * num_planes + i].length,
                (uint64_t) membuf[buf_idx * num_planes + i].start);
            if (membuf[buf_idx * num_planes + i].start == MAP_FAILED) {
                fprintf(stderr, "mmap failed\n");
                return 1;
            }
        }
    }

    return 0;
}

int CameraV4L2::init_device(void) {
    struct v4l2_capability cap;
    struct v4l2_cropcap cropcap;
    struct v4l2_crop crop;
    struct v4l2_format fmt;
    unsigned int min;

    if (-1 == xioctl(fd, VIDIOC_QUERYCAP, &cap)) {
        if (EINVAL == errno) {
            printf("%s is no V4L2 device\\n", dev_name);
            return 1;
        } else {
            printf("errno VIDIOC_QUERYCAP");
            return 1;
        }
    }

    if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
        fprintf(stderr, "%s does not support streaming i/o\\n",
                dev_name);
        return 1;
    }

    /* Select video input, video standard and tune here. */

    CLEAR(cropcap);

    crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;

    if (0 == xioctl(fd, VIDIOC_G_CROP, &crop)) {
        crop.c.height = IMG_HEIGHT;
        crop.c.width = IMG_WIDTH;
        crop.c.left = 0;
        crop.c.top = 0;

        if (-1 == xioctl(fd, VIDIOC_S_CROP, &crop)) {
            switch (errno) {
            case EINVAL:
                /* Cropping not supported. */
                break;
            default:
                /* Errors ignored. */
                break;
            }
        }
    } else {
        printf("errno VIDIOC_CROPCAP");
        return 1;
    }
    struct v4l2_fmtdesc fmtdesc;
    fmtdesc.index = 0;
    fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    while( 0 == xioctl(fd , VIDIOC_ENUM_FMT , &fmtdesc))
    {
        fmtdesc.index++;
    }
    CLEAR(fmt);

    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    if (-1 == xioctl(fd, VIDIOC_G_FMT, &fmt))
        printf("errno VIDIOC_G_FMT");
    fmt.fmt.pix_mp.width       = IMG_WIDTH;
    fmt.fmt.pix_mp.height      = IMG_HEIGHT;
    fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_YUYV;
    fmt.fmt.pix_mp.field       = V4L2_FIELD_INTERLACED;

    if (-1 == xioctl(fd, VIDIOC_S_FMT, &fmt)) {
        printf("errno VIDIOC_S_FMT");
        return 1;
    }
    return 0;
}

int CameraV4L2::open_device(void) {
    struct stat st;

    if (-1 == stat(dev_name, &st)) {
        fprintf(stderr, "Cannot identify '%s': %d, %s\\n",
            dev_name, errno, strerror(errno));
        return 1;
    }

    if (!S_ISCHR(st.st_mode)) {
        fprintf(stderr, "%s is no devicen", dev_name);
        return 1;
    }

    fd = open(dev_name, O_RDWR | O_NONBLOCK, 0);

    if (-1 == fd) {
        fprintf(stderr, "Cannot open '%s': %d, %s\\n",
            dev_name, errno, strerror(errno));
        return 1;
    }
    return 0;
}

wxByte CameraV4L2::BitsPerPixel()
{
    // YUV422 Y only is 8-bit
    return 8;
}

bool CameraV4L2::Connect(const wxString& camId) {
    if (open_device()) {
        return true;
    }
    if (init_device()) {
        close(fd);
        return true;
    }
    if (init_mmap()) {
        return true;
    }

    Connected = true;
    return false;;
}

bool CameraV4L2::Disconnect() {
    if (-1 == close(fd))
        printf("Failed to close\n");
    Connected = false;
    streamming = false;
    return false;
}

long long getmillisec() {
    struct timeval cur_time;
    gettimeofday(&cur_time, NULL);
    long long milliseconds = cur_time.tv_sec*1000LL + cur_time.tv_usec/1000;
    return milliseconds;
}

bool CameraV4L2::Capture(int duration, usImage& img, int options, const wxRect& subframe) {
    if (img.Init(IMG_WIDTH, IMG_HEIGHT)) {
        pFrame->Alert(_("Memory allocation error"));
        throw ERROR_INFO("img.Init failed");
    }
    long long prev_time = getmillisec();

    struct v4l2_control v4l2_ctl;
    v4l2_ctl.id = V4L2_CID_VBLANK;
    v4l2_ctl.value = 2210 * 51 * duration / 1000 - IMG_HEIGHT;
    if (xioctl(fd, VIDIOC_S_CTRL, &v4l2_ctl) == -1) {
        printf("Set control error\n");
    }
    v4l2_ctl.id = V4L2_CID_EXPOSURE;
    v4l2_ctl.value = 2210 * 51 * duration / 1000;
    if (xioctl(fd, VIDIOC_S_CTRL, &v4l2_ctl) == -1) {
        printf("Set control error\n");
    }

    // Begin V4L2 operation
    struct v4l2_plane planes[1];
    struct v4l2_buffer buf;
    CLEAR(buf);

    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.m.planes = planes;
    buf.length = 1;
    buf.index = 0;

    if (!streamming) {
        start_capturing();
    }

    if (-1 == xioctl(fd, VIDIOC_QBUF, &buf)) {
        printf("VIDIOC_QBUF error\n");
        return true;
    }

    fd_set fds;
    struct timeval tv;
    int r;

    FD_ZERO(&fds);
    FD_SET(fd, &fds);

    /* Timeout. */
    tv.tv_sec = 2;
    tv.tv_usec = 0;

    r = select(fd + 1, &fds, NULL, NULL, &tv);
    if (0 == r) {
        fprintf(stderr, "timeout\\n");
    }

    while (xioctl(fd, VIDIOC_DQBUF, &buf) == -1) {
        switch (errno) {
            case EAGAIN:
                continue;
            default:
                printf("VIDIOC_DQBUF error\n");
            return true;
        }
    }
    long long cur_time = getmillisec();

    uint16_t * buf_data = (uint16_t *) membuf[buf.index].start;
    for (int i = 0; i < img.NPixels; i++) {
        // Capture only Y channel
        img.ImageData[i] = *buf_data++ & 0xFF;
    }
    total_time += (cur_time - prev_time);
    total_frame++;
    printf("Capture delay %ums ave %f\n", (uint16_t) (cur_time - prev_time),
        (float)total_time / total_frame);
    prev_time = cur_time;

    return false;
}

#endif

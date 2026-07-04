#define LOG_TAG "Miracast-UIBC"
#define _GNU_SOURCE
#include <linux/input-event-codes.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include "miracle-uibcctl.h"
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <netdb.h>
#include<arpa/inet.h>
#include <math.h>
#include <pthread.h>
#include <linux/input.h>
#include <sys/poll.h>
#include <sys/eventfd.h>
#include <errno.h>
#include <dirent.h>
#include <limits.h>
#include "cdx_log.h"
#include "input.h-labels.h"

#include "list.h"

#define INPUTDEV_NUM_MAX 16
#define INPUT_EVENTS_MAX 128
#define MULTI_TOUCH_MAX  16
static const char *DEVICE_PATH = "/dev/input";

enum KeyValue {
    UP = 0,
    DOWN,
    REPEAT,
    UNKNOW
};

static struct label key_value_labels[] = {
        { "UP", 0 },
        { "DOWN", 1 },
        { "REPEAT", 2 },
        LABEL_END,
};

enum DeviceType {
    Mouse,
    Keyboard,
    SingleTouch,
    MultiTouch,
    Unknow
};

/* this macro is used to tell if "bit" is set in "array"
 * it selects a byte from the array, and does a boolean AND
 * operation with a byte that only has the relevant bit set.
 * eg. to check for the 12th bit, we do (array[1] & 1<<4)
 */
#define test_bit(bit, array)    ((array)[(bit)/8] & (1<<((bit)%8)))

typedef enum {
    GENERIC_TOUCH_or_LEFT_MOUSE_DOWN = 0,
    GENERIC_TOUCH_or_LEFT_MOUSE_UP,
    GENERIC_TOUCH_or_MOUSE_MOVE,
    GENERIC_KEY_DOWN,
    GENERIC_KEY_UP,
    GENERIC_ZOOM,
    GENERIC_VERTICAL_SCROLL,
    GENERIC_HORIZONTAL_SCROLL,
    GENERIC_ROTATE
} MessageType;

typedef struct {
   char* m_PacketData;
   size_t m_PacketDataLen;
   bool m_DataValid;
} UibcMessage;

struct mt_event {
    //int mt_slot_id;
    int mt_tracking_id;
    int mt_position_x;
    int mt_position_y;
    int mt_map_x;
    int mt_map_y;
};

// include single and MultiTouch
struct parsed_event {
    struct mt_event st_mt_event[MULTI_TOUCH_MAX];//MultiTouch, index represent slot
    int last_mt_slot;
    int btn_state;//touch or mouse or left start: down/up
    int abs_x;//single touch
    int abs_y;
    int abs_map_x;//single touch mapped to video resolution
    int abs_map_y;
    int rel_x;
    int rel_y;
    int rel_add_x;
    int rel_add_y;
    MessageType message_type;
    char multi_msg[256];//format: %d,%d,%d,%d,%d...
};

struct input_device {
    struct list_head list;
	char devpath[256];
	char name[80];
	char location[80];
	char idstr[80];

	struct input_id id;
    struct input_absinfo absX_info;
    struct input_absinfo absY_info;

	int version;

	int fd;
	struct  input_event st_input_event[INPUT_EVENTS_MAX]; /* u16/type,  u16/code, s32/value */
	int32_t input_event_index;//how many input events
    struct parsed_event st_parsed_event;
    int32_t type;//device type
};

struct UIBCImplS {
	pthread_t pid;
	char host[32];
	unsigned short port;
	int sockfd;
	uint64_t exit;
    int state; /* 0:init, 1:working, -1:error */

    struct pollfd pfds[INPUTDEV_NUM_MAX];//up to 16 input device
    int evfd;
    int    nfds;
    struct list_head inputDevListHead;
    unsigned int video_width;
    unsigned int video_height;
};

UibcMessage buildUibcMessage(MessageType type, const char* inEventDesc, double widthRatio, double heightRatio);

static char** str_split(char* pStr, const char* pDelim, size_t* size);

void getUIBCGenericTouchPacket(const char *inEventDesc, UibcMessage* uibcmessage, double widthRatio, double heightRatio);
void getUIBCGenericKeyPacket(const char *inEventDesc, UibcMessage* uibcmessage);
void getUIBCGenericZoomPacket(const char *inEventDesc,UibcMessage* uibcmessage);
void getUIBCGenericScalePacket(const char *inEventDesc, UibcMessage* uibcmessage);
void getUIBCGenericRotatePacket(const char *inEventDesc, UibcMessage* uibcmessage);

void hexdump(void *_data, size_t len);
void binarydump(void *_data, size_t len);

int sendUibcMessage(UibcMessage* uibcmessage, int sockfd);

/**
 *@axis: stand for code, such as ABX_X, ABX_Y
 */
static int getAbsInfo(struct input_device *devinfo, int axis, struct input_absinfo* absinfo, uint8_t* absBitmask ){
    if (axis >= 0 && axis <= ABS_MAX && test_bit(axis, absBitmask)) {
        if(ioctl(devinfo->fd, EVIOCGABS(axis), absinfo)) {
            CDX_LOGW("Error reading absolute controller for device %d fd %d, errno=%d",
                  axis, devinfo->fd, errno);
            return -1;
        }
        return 0;
    }
    return -1;
}

static int registerDeviceForPoll(struct UIBCImplS *impl, struct input_device* dev) {
    if (dev == NULL || dev->fd < 0) {
        CDX_LOGE("Cannot call registerDeviceForPoll with null Device");
        return -1;
    }
    if (impl->nfds >= INPUTDEV_NUM_MAX) {
        CDX_LOGE("already open %d input devices, up to the max num", INPUTDEV_NUM_MAX);
        close(dev->fd);
        free(dev);
        return -1;
    }

    impl->pfds[impl->nfds].fd = dev->fd;
    impl->pfds[impl->nfds++].events = POLLIN;
    //initiate
	dev->st_parsed_event.rel_x = impl->video_width/2;
	dev->st_parsed_event.rel_y = impl->video_height/2;
    dev->st_parsed_event.last_mt_slot = 0;
	dev->input_event_index = 0;
    for (int i = 0; i < MULTI_TOUCH_MAX; i++) {
        dev->st_parsed_event.st_mt_event[i].mt_tracking_id = -1;
    }

    return 0;
}

static struct input_device *inputdev_open(struct UIBCImplS *impl, const char *devpath)
{
	struct input_device *dev;
    int ret;

	if (!devpath)
		goto errout;

	dev = calloc(1, sizeof(struct input_device));
	if (!dev)
	{
		CDX_LOGE("could not malloc, %s", strerror(errno));
		goto errout;
	}

	dev->fd = open(devpath, O_RDWR);
	if (dev->fd < 0)
	{
		CDX_LOGE("could not open device '%s', %s",
			devpath, strerror(errno));
		free(dev);
		goto errout;
	}

	strcpy(dev->devpath, devpath);

	if (ioctl(dev->fd, EVIOCGVERSION, &dev->version))
	{
		CDX_LOGW("could not get driver version for '%s', %s",
			devpath, strerror(errno));
	}
	if (ioctl(dev->fd, EVIOCGID, &dev->id))
	{
		CDX_LOGW("could not get driver id for '%s', %s",
			devpath, strerror(errno));
    }

	if (ioctl(dev->fd, EVIOCGNAME(sizeof(dev->name)-1), dev->name) < 1)
	{
		CDX_LOGW("could not get device name for %s, %s",
			devpath, strerror(errno));
	}
	if (ioctl(dev->fd, EVIOCGPHYS(sizeof(dev->location)-1), dev->location) < 1)
	{
		CDX_LOGW("could not get location for %s, %s",
			devpath, strerror(errno));
	}
	if (ioctl(dev->fd, EVIOCGUNIQ(sizeof(dev->idstr) - 1), dev->idstr) < 1)
	{
		CDX_LOGW("could not get idstring for %s, %s",
			devpath, strerror(errno));
    }

    uint8_t keyBitmask[(KEY_MAX + 1) / 8];
    uint8_t absBitmask[(ABS_MAX + 1) / 8];
    uint8_t relBitmask[(REL_MAX + 1) / 8];

    // Figure out the kinds of events the device reports.
    ioctl(dev->fd, EVIOCGBIT(EV_KEY, sizeof(keyBitmask)), keyBitmask);
    ioctl(dev->fd, EVIOCGBIT(EV_ABS, sizeof(absBitmask)), absBitmask);
    ioctl(dev->fd, EVIOCGBIT(EV_REL, sizeof(relBitmask)), relBitmask);

	if (1)
	{
		CDX_LOGD("    devpath:  %s\n", dev->devpath);
		CDX_LOGD("    bus:      %04x\n"
					 	"    vendor    %04x\n"
						"    product   %04x\n"
						"    version   %04x\n",
						dev->id.bustype, dev->id.vendor,
						dev->id.product, dev->id.version);
		CDX_LOGD("    name:     \"%s\"\n", dev->name);
		CDX_LOGD("    location: \"%s\"\n"
						"    id:       \"%s\"\n", dev->location, dev->idstr);
		CDX_LOGD("    version:  %d.%d.%d\n",
						dev->version >> 16,
						(dev->version >> 8) & 0xff,
						dev->version & 0xff);
        // Is this an old style single-touch driver?
        CDX_LOGD("BTN_TOUCH:%d", test_bit(BTN_TOUCH, keyBitmask));

        CDX_LOGD("ABS_X:%d, ABS_Y:%d, REL_X:%d ,REL_Y:%d",
              test_bit(ABS_X, absBitmask),test_bit(ABS_Y, absBitmask),
              test_bit(REL_X, relBitmask), test_bit(REL_X, relBitmask));

        if (test_bit(ABS_X, absBitmask) &&  test_bit(ABS_Y, absBitmask)) {
            dev->type = SingleTouch;
        }
        if (test_bit(REL_X, relBitmask) &&  test_bit(REL_X, relBitmask)) {
            dev->type = Mouse;
        }

        CDX_LOGD("ABS_MT_POSITION_X:%d, ABS_MT_POSITION_Y:%d, ABS_MT_TRACKING_ID:%d, ABS_MT_SLOT:%d ",
              test_bit(ABS_MT_POSITION_X, absBitmask),test_bit(ABS_MT_POSITION_Y, absBitmask),
              test_bit(ABS_MT_TRACKING_ID, absBitmask), test_bit(ABS_MT_SLOT, relBitmask));

        if (test_bit(ABS_MT_POSITION_X, absBitmask) &&  test_bit(ABS_MT_POSITION_Y, absBitmask)) {
            if (test_bit(ABS_MT_TRACKING_ID, absBitmask)) {//we now only support MultiTouch B protocol, not support MultiTouch A protocal
                dev->type = MultiTouch;
            }
        }

        ret = getAbsInfo(dev, ABS_X, &dev->absX_info, absBitmask);
        if(!ret)
            CDX_LOGD("X minValue = %d, maxValue = %d ", dev->absX_info.minimum, dev->absX_info.maximum);
        ret = getAbsInfo(dev, ABS_Y, &dev->absY_info, absBitmask);
        if(!ret)
            CDX_LOGD("Y minValue = %d, maxValue = %d ", dev->absY_info.minimum, dev->absY_info.maximum);
    }
    if (registerDeviceForPoll(impl, dev)) {
        return NULL;
    }
    list_add_tail(&dev->list, &impl->inputDevListHead);
    return dev;

errout:
	CDX_LOGE("could open device '%s'!", devpath);
	return NULL;
}

static int scan_dir(struct UIBCImplS *impl, const char *dirname)
{
    char devname[PATH_MAX];
    char *filename;
    DIR *dir;
    struct dirent *de;
    dir = opendir(dirname);
    if(dir == NULL)
        return -1;
    strcpy(devname, dirname);
    filename = devname + strlen(devname);
    *filename++ = '/';
    while((de = readdir(dir))) {
        if(de->d_name[0] == '.' &&
           (de->d_name[1] == '\0' ||
            (de->d_name[1] == '.' && de->d_name[2] == '\0')))
            continue;
        strcpy(filename, de->d_name);
        CDX_LOGD("opening file node %s", devname);
        inputdev_open(impl, devname);
    }
    closedir(dir);
    return 0;
}

static const char *get_label(const struct label *labels, int value)
{
    while(labels->name && value != labels->value) {
        labels++;
    }
    return labels->name;
}

static void print_event_label(int type, int code, int value, int print_flags)
{
    const char *type_label, *code_label, *value_label;

    if (print_flags) {
        type_label = get_label(ev_labels, type);
        code_label = NULL;
        value_label = NULL;

        switch(type) {
            case EV_SYN:
                code_label = get_label(syn_labels, code);
                break;
            case EV_KEY:
                code_label = get_label(key_labels, code);
                value_label = get_label(key_value_labels, value);
                break;
            case EV_REL:
                code_label = get_label(rel_labels, code);
                break;
            case EV_ABS:
                code_label = get_label(abs_labels, code);
                switch(code) {
                    case ABS_MT_TOOL_TYPE:
                        value_label = get_label(mt_tool_labels, value);
                }
                break;
            case EV_MSC:
                code_label = get_label(msc_labels, code);
                break;
            case EV_LED:
                code_label = get_label(led_labels, code);
                break;
            case EV_SND:
                code_label = get_label(snd_labels, code);
                break;
            case EV_SW:
                code_label = get_label(sw_labels, code);
                break;
            case EV_REP:
                code_label = get_label(rep_labels, code);
                break;
            case EV_FF:
                code_label = get_label(ff_labels, code);
                break;
            case EV_FF_STATUS:
                code_label = get_label(ff_status_labels, code);
                break;
        }

        char buffer[256];
        int i = 0;
        if (type_label)
            i += sprintf(buffer+i, "%-12.12s", type_label);
        else
            i += sprintf(buffer+i, "%04x        ", type);

        if (code_label)
            i += sprintf(buffer+i, " %-20.20s", code_label);
        else
            i += sprintf(buffer+i, " %04x                ", code);

        if (value_label)
            i += sprintf(buffer+i, " %-20.20s", value_label);
        else
            i += sprintf(buffer+i, " %08x            ", value);
        CDX_LOGD("    %s", buffer);
    } else {
        CDX_LOGD("%04x %04x %08x (%d)", type, code, value, value);
    }
}

static void inputdev_close(struct UIBCImplS *impl)
{
    while (!list_empty(&impl->inputDevListHead)){
        struct list_head* pos = impl->inputDevListHead.next;
        list_del(pos);
        struct input_device* dev = list_entry(pos, struct input_device, list);
        close(dev->fd);
        free(dev);
        CDX_LOGD("close device '%s'", dev->devpath);
    }
	return;
}

static int inputdev_read(struct input_device *dev, struct input_event *event)
{
	int res;

	res = read(dev->fd, event, sizeof(struct input_event));
	if (res < (int)sizeof(struct input_event))
	{
		CDX_LOGE("could not get event");
		return -1;
	}

    print_event_label(event->type, event->code, event->value, 1);
	return 0;
}

static int get_multi_touch_point_num(struct input_device* tmpdev) {
    struct parsed_event* tmp_parsed_event = &tmpdev->st_parsed_event;
    int count = 0;
    for (int i = 0; i< MULTI_TOUCH_MAX; i++) {
        if (tmp_parsed_event->st_mt_event[i].mt_tracking_id != -1){
            count++;
        }
    }
    return count;
}

static int mouse_event_processer(struct UIBCImplS *impl, struct input_device* tmpdev, char* buf) {
    struct parsed_event* tmp_parsed_event = &tmpdev->st_parsed_event;
    tmp_parsed_event->rel_add_x += tmp_parsed_event->rel_x;
    if (tmp_parsed_event->rel_add_x < 0) { tmp_parsed_event->rel_add_x = 0; }
    if (tmp_parsed_event->rel_add_x > impl->video_width) {
        tmp_parsed_event->rel_add_x = impl->video_width;
    }

    tmp_parsed_event->rel_add_y += tmp_parsed_event->rel_y;
    if (tmp_parsed_event->rel_add_y < 0) { tmp_parsed_event->rel_add_y = 0; }
    if (tmp_parsed_event->rel_add_y > impl->video_height) {
        tmp_parsed_event->rel_add_y = impl->video_height;
    }

    tmp_parsed_event->message_type = GENERIC_TOUCH_or_MOUSE_MOVE;

    if (tmp_parsed_event->btn_state == DOWN)
    {
        tmp_parsed_event->message_type = GENERIC_TOUCH_or_LEFT_MOUSE_DOWN;
    }
    else if (tmp_parsed_event->btn_state == UP)
    {
        tmp_parsed_event->message_type = GENERIC_TOUCH_or_LEFT_MOUSE_UP;
    }
    else if (tmp_parsed_event->btn_state == -1)
    {
        tmp_parsed_event->message_type = GENERIC_TOUCH_or_MOUSE_MOVE;
    }
    else
    {
        CDX_LOGE("ERROR event!!!");
        return -1;
    }

    int msg_num_pointers = 1;
    int msg_pointer_id = 2;//pointerID, whatever

    sprintf(buf, "%d,%d,%d,%d,%d", tmp_parsed_event->message_type, msg_num_pointers,
            msg_pointer_id, tmp_parsed_event->rel_add_x, tmp_parsed_event->rel_add_y);

    return 0;
}

static int single_touch_event_processer(struct UIBCImplS *impl, struct input_device* tmpdev, char* buf) {
    int minX = tmpdev->absX_info.minimum;
    int minY = tmpdev->absY_info.minimum;
    int maxX = tmpdev->absX_info.maximum;
    int maxY = tmpdev->absY_info.maximum;
    struct parsed_event* tmp_parsed_event = &tmpdev->st_parsed_event;
    /**
     * https://source.android.com/devices/input/touch-devices
     * displayX = (x - minX) * displayWidth / (maxX - minX + 1)
     * displayY = (y - minY) * displayHeight / (maxY - minY + 1)
     */
    tmp_parsed_event->abs_map_x = (tmp_parsed_event->abs_x - minX) * impl->video_width / (maxX - minX + 1);
    tmp_parsed_event->abs_map_y = (tmp_parsed_event->abs_y - minY) * impl->video_height / (maxY - minY + 1);
    tmp_parsed_event->message_type = GENERIC_TOUCH_or_MOUSE_MOVE;

    if (tmp_parsed_event->btn_state == DOWN)
    {
        tmp_parsed_event->message_type = GENERIC_TOUCH_or_LEFT_MOUSE_DOWN;
    }
    else if (tmp_parsed_event->btn_state == UP)
    {
        tmp_parsed_event->message_type = GENERIC_TOUCH_or_LEFT_MOUSE_UP;
    }
    else if (tmp_parsed_event->btn_state == -1)
    {
        tmp_parsed_event->message_type = GENERIC_TOUCH_or_MOUSE_MOVE;
    }
    else
    {
        CDX_LOGE("ERROR event!!!");
        return -1;
    }

    int msg_num_pointers = 1;
    int msg_pointer_id = 0;//pointerID, only zero point id can swipe screen in some activity 

    sprintf(buf, "%d,%d,%d,%d,%d", tmp_parsed_event->message_type, msg_num_pointers,
            msg_pointer_id, tmp_parsed_event->abs_map_x, tmp_parsed_event->abs_map_y);

    return 0;
}

static int multi_touch_event_processer(struct UIBCImplS *impl, struct input_device* tmpdev, char* buf) {
    int minX = tmpdev->absX_info.minimum;
    int minY = tmpdev->absY_info.minimum;
    int maxX = tmpdev->absX_info.maximum;
    int maxY = tmpdev->absY_info.maximum;
    struct parsed_event* tmp_parsed_event = &tmpdev->st_parsed_event;
    /**
     * https://source.android.com/devices/input/touch-devices
     * displayX = (x - minX) * displayWidth / (maxX - minX + 1)
     * displayY = (y - minY) * displayHeight / (maxY - minY + 1)
     */
    for (int i = 0; i < MULTI_TOUCH_MAX; i++) {
        if (tmp_parsed_event->st_mt_event[i].mt_tracking_id != -1) {//valid point
            tmp_parsed_event->st_mt_event[i].mt_map_x =
                (tmp_parsed_event->st_mt_event[i].mt_position_x - minX) * impl->video_width / (maxX - minX + 1);
            tmp_parsed_event->st_mt_event[i].mt_map_y =
                (tmp_parsed_event->st_mt_event[i].mt_position_y - minY) * impl->video_height / (maxY - minY + 1);
            tmp_parsed_event->message_type = GENERIC_TOUCH_or_MOUSE_MOVE;
        }
    }

    if (tmp_parsed_event->btn_state == DOWN)
    {
        tmp_parsed_event->message_type = GENERIC_TOUCH_or_LEFT_MOUSE_DOWN;
    }
    else if (tmp_parsed_event->btn_state == UP)
    {
        tmp_parsed_event->message_type = GENERIC_TOUCH_or_LEFT_MOUSE_UP;
    }
    else if (tmp_parsed_event->btn_state == -1)
    {
        tmp_parsed_event->message_type = GENERIC_TOUCH_or_MOUSE_MOVE;
    }
    else
    {
        CDX_LOGE("ERROR event!!!");
        return -1;
    }

    int j = 0;
    int point_num = get_multi_touch_point_num(tmpdev);
    if (point_num > 0){
        j += sprintf(buf+j, "%d,%d", tmp_parsed_event->message_type, point_num);
        for (int i = 0; i < MULTI_TOUCH_MAX; i++) {
            if (tmp_parsed_event->st_mt_event[i].mt_tracking_id != -1) {//valid point
                j += sprintf(buf+j, ",%d,%d,%d", i,
                //j += sprintf(buf+j, ",%d,%d,%d", tmp_parsed_event->st_mt_event[i].mt_tracking_id,
                             tmp_parsed_event->st_mt_event[i].mt_map_x,
                             tmp_parsed_event->st_mt_event[i].mt_map_y);
            }
        }
        strcpy(tmp_parsed_event->multi_msg, buf);
    } else {
        //when multi touch up, there is no point coordinate to upload
        //we use the last coordinate instead
        if (tmp_parsed_event->message_type == GENERIC_TOUCH_or_LEFT_MOUSE_UP) {
            tmp_parsed_event->multi_msg[0] = '1';//GENERIC_TOUCH_or_LEFT_MOUSE_UP
            strcpy(buf, tmp_parsed_event->multi_msg);
        }
    }
    return 0;
}

// format: "typeId, number of pointers, pointer Id1, X coordnate, Y coordinate, pointer Id2, X coordnate, Y coordnate,..."
int MT_MessageSend(struct UIBCImplS *impl, struct input_device* tmpdev) {
    int useless_event_count = 0;
    struct parsed_event* tmp_parsed_event = &tmpdev->st_parsed_event;
    //initiate up/down state
    tmp_parsed_event->btn_state = -1;

    for (int i = 0; i < tmpdev->input_event_index; i++) {
	    struct input_event temp_input_event = tmpdev->st_input_event[i]; /* u16/type,  u16/code, s32/value */
        int value = temp_input_event.value;
        switch (temp_input_event.type) {
        case EV_ABS:
            switch (temp_input_event.code) {
                case ABS_MT_TRACKING_ID:
                    tmp_parsed_event->st_mt_event[tmp_parsed_event->last_mt_slot].mt_tracking_id = value;
                    break;
                case ABS_MT_POSITION_X:
                    tmp_parsed_event->st_mt_event[tmp_parsed_event->last_mt_slot].mt_position_x = value;
                    break;
                case ABS_MT_POSITION_Y:
                    tmp_parsed_event->st_mt_event[tmp_parsed_event->last_mt_slot].mt_position_y = value;
                    break;
                case ABS_X:
                    tmp_parsed_event->abs_x = value;
                    break;
                case ABS_Y:
                    tmp_parsed_event->abs_y = value;
                    break;
                case ABS_MT_SLOT:
                    tmp_parsed_event->last_mt_slot = value;
                    break;
                default:
                    break;
            }
            break;
        case EV_KEY:
            switch (temp_input_event.code) {
                case BTN_TOUCH:
                case BTN_MOUSE:
                    tmp_parsed_event->btn_state = value;
                    break;
                default:
                    break;
            }
            break;
        case EV_REL:
            switch (temp_input_event.code) {
                case REL_X:
                    tmp_parsed_event->rel_x = value;
                    break;
                case REL_Y:
                    tmp_parsed_event->rel_y = value;
                    break;
            }
            break;
        case EV_SYN:
            switch (temp_input_event.code) {
                case SYN_DROPPED:
                    return -1;//Dropped input event until the next input sync.
                    break;
                default:
                    break;
            }
        default:
            useless_event_count++;
            break;
        }
    }

    if (useless_event_count == tmpdev->input_event_index) {
        CDX_LOGD("there is not useful input events, ignore this input group!");
        return -1;
    }

    char msg_buf[256] = {0};
    switch (tmpdev->type) {
        case Mouse:
            mouse_event_processer(impl, tmpdev, msg_buf);
            break;
        case SingleTouch:
            single_touch_event_processer(impl, tmpdev, msg_buf);
            break;
        case MultiTouch:
            multi_touch_event_processer(impl, tmpdev, msg_buf);
            break;
        default:
            break;
    }


    UibcMessage uibcmessage;
    uibcmessage = buildUibcMessage(tmpdev->st_parsed_event.message_type, msg_buf, 1, 1);
    int ret = sendUibcMessage(&uibcmessage, impl->sockfd);
    if (ret)
    {
        CDX_LOGE("ERROR sendUibcMessage");
        impl->state = -1;
        return -1;
    }

    return ret;
}

static void *MainThread(void *arg)
{
    //TODO: Add miracle TUI interface
    //TODO: Add parsearg
    //--daemon (read stdin)

  struct UIBCImplS *impl = arg;
  unsigned short portno;
  struct hostent *server;
  struct sockaddr_in serv_addr;
//  char buffer[256];
//  int r;
  //struct input_event event;

//  log_max_sev = LOG_INFO;

  server = gethostbyname(impl->host);
  portno = impl->port;

  CDX_LOGD("server %s port %d", impl->host, portno);

  if (server == NULL)
  {
    CDX_LOGE("ERROR, no such host\n");
    return NULL;
  }

  impl->sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (impl->sockfd < 0)
  {
    CDX_LOGE("ERROR opening socket");
  	impl->state = -1;
  	return NULL;
  }

  bzero((char *) &serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  bcopy(server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
  serv_addr.sin_port = htons(portno);

  if (connect(impl->sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
  {
    CDX_LOGE("ERROR connecting");
  	impl->state = -1;
  	return NULL;
  }

  CDX_LOGD("Connect SUCCESS!");
  impl->state = 1;

  //int daemon = 0;
  while (!impl->exit)
  {
      int ret= poll(impl->pfds, impl->nfds, -1);
      if (ret == -1) {
          CDX_LOGE("poll return errno %s", strerror(errno));
      }

      for(int i = 0; i < impl->nfds; i++){
          if(impl->pfds[i].revents & POLLIN){
              if (impl->pfds[i].fd == impl->evfd) {
                  uint64_t exit_signal;
                  read(impl->evfd, &exit_signal, sizeof(exit_signal));
                  CDX_LOGW("receive exit thread signal: %lld", exit_signal);
                  break;
              } else {
                  struct input_device *tmpdev;
                  int find_device = 0;
                  list_for_each_entry(tmpdev, &impl->inputDevListHead, list) {
                      if (tmpdev->fd == impl->pfds[i].fd) {
                          find_device = 1;
                          break;
                      }
                  }
                  if (!find_device) {
                      CDX_LOGE("do not find adaptive device, ignore this event");
                      continue;
                  }

                  int ret = 0;
                  ret = inputdev_read(tmpdev, &tmpdev->st_input_event[tmpdev->input_event_index]);
                  if (ret != 0) /* some error */
                  {
                      CDX_LOGE("inputdev_read ERROR...");
                      tmpdev->input_event_index = 0;
                      continue;
                  }
                  /* event finsh, send UIBC msg */
                  if (tmpdev->st_input_event[tmpdev->input_event_index].type == EV_SYN)
                  {
                      MT_MessageSend(impl, tmpdev);
                      tmpdev->input_event_index = 0;
                      continue;
                  }
                  tmpdev->input_event_index++;
              }
          }
      }
  }

  close(impl->sockfd);
  return NULL;
}

const char *int2binary(int x, int padding)
{
  char *b;
  int min_padding = x > 0 ? floor(log2(x)) : 0;
  if (padding < min_padding) {
    padding = min_padding;
  }

  b = (char *)malloc (sizeof (char *) * padding + 1);
  strcpy(b, "");

  int z;
  for (z = pow(2,padding); z > 0; z >>= 1) {
    strcat(b, ((x & z) == z) ? "1" : "0");
  }

  return b;
}

int sendUibcMessage(UibcMessage* uibcmessage, int sockfd) {
  ssize_t n;

//  printf("sending %zu bytes\n", uibcmessage->m_PacketDataLen);

  n = write(sockfd, uibcmessage->m_PacketData , uibcmessage->m_PacketDataLen);
  free(uibcmessage->m_PacketData);
  uibcmessage->m_PacketData = NULL;

  if (n < 0) {
    CDX_LOGE("ERROR writing to socket");
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

UibcMessage buildUibcMessage(MessageType type,
    const char* inEventDesc,
    double widthRatio,
    double heightRatio)
{
  UibcMessage uibcmessage;
  uibcmessage.m_PacketData = NULL;
  uibcmessage.m_PacketDataLen = 0;
  uibcmessage.m_DataValid = false;

  switch (type) {
    case GENERIC_TOUCH_or_LEFT_MOUSE_DOWN:
    case GENERIC_TOUCH_or_LEFT_MOUSE_UP:
    case GENERIC_TOUCH_or_MOUSE_MOVE:
      getUIBCGenericTouchPacket(inEventDesc,
          &uibcmessage,
          widthRatio,
          heightRatio);
      break;

    case GENERIC_KEY_DOWN:
    case GENERIC_KEY_UP:
      getUIBCGenericKeyPacket(inEventDesc, &uibcmessage);
      break;

    case GENERIC_ZOOM:
      getUIBCGenericZoomPacket(inEventDesc, &uibcmessage);
      break;

    case GENERIC_VERTICAL_SCROLL:
    case GENERIC_HORIZONTAL_SCROLL:
      getUIBCGenericScalePacket(inEventDesc, &uibcmessage);
      break;

    case GENERIC_ROTATE:
      getUIBCGenericRotatePacket(inEventDesc, &uibcmessage);
      break;
  };
  return uibcmessage;
}

// format: "typeId, number of pointers, pointer Id1, X coordnate, Y coordinate, pointer Id2, X coordnate, Y coordnate,..."
void getUIBCGenericTouchPacket(const char *inEventDesc,
    UibcMessage* uibcmessage,
    double widthRatio,
    double heightRatio) {
  CDX_LOGD("getUIBCGenericTouchPacket (%s)", inEventDesc);
  char* outData;
  int32_t typeId = 0;
  int32_t numberOfPointers = 0;
  size_t uibcBodyLen = 0;
  int32_t genericPacketLen = 0;
  int32_t pointerId, x, y;
  size_t size;

  char** splitedStr = str_split((char*)inEventDesc, ",", &size);

  if ((int)size - 5 < 0 || ((int)size - 5) % 3 != 0) {
    CDX_LOGE("getUIBCGenericTouchPacket (%s)", "bad input event");
    return;
  }
  int offset_split = 0;
  typeId = atoi(*(splitedStr + offset_split++));
  numberOfPointers = atoi(*(splitedStr + offset_split++));

  genericPacketLen = numberOfPointers * 5 + 1;
  uibcBodyLen = genericPacketLen + 7; // Generic header length = 7
  //Padding to even number
  uibcBodyLen = (uibcBodyLen % 2 == 0) ? uibcBodyLen : (uibcBodyLen + 1);

  outData = (char*)malloc(uibcBodyLen);
  // UIBC header Octets
  int offset_header = 0;
  //Version (3 bits),T (1 bit),Reserved(8 bits),Input Category (4 bits)
  outData[offset_header++] = 0x00; // 000 0 0000
  outData[offset_header++] = 0x00; // 0000 0000

  //Length(16 bits)
  outData[offset_header++] = (uibcBodyLen >> 8) & 0xFF; // first 8 bytes
  outData[offset_header++] = uibcBodyLen & 0xFF; // last 8 bytes

  //Generic Input Body Format
  outData[offset_header++] = typeId & 0xFF; // Tyoe ID, 1 octet

  // Length, 2 octets
  outData[offset_header++] = (genericPacketLen >> 8) & 0xFF; // first 8 bytes
  outData[offset_header++] = genericPacketLen & 0xFF; //last 8 bytes

  // Number of pointers, 1 octet
  outData[offset_header++] = numberOfPointers & 0xFF; 

  int offset = 0;
  //CDX_LOGD("getUIBCGenericTouchPacket numberOfPointers=[%d]\n", numberOfPointers);
  for (int i = 0; i < numberOfPointers; i++) {
    int splitoffset = offset_split + (i * 3);
    pointerId = atoi(*(splitedStr + splitoffset++));
    offset = offset_header + ( i * 5);
    outData[offset++] = pointerId & 0xFF;

    x = atoi(*(splitedStr + splitoffset++));
    x = (int32_t)((double)x / widthRatio);
    outData[offset++] = (x >> 8) & 0xFF;
    outData[offset++] = x & 0xFF;

    y = atoi(*(splitedStr + splitoffset++));
    y = (int32_t)((double)y / heightRatio);
    outData[offset++] = (y >> 8) & 0xFF;
    outData[offset++] = y & 0xFF;
    //CDX_LOGD("getUIBCGenericTouchPacket PointerId=[%d], X-coordinate=[%d], Y-coordinate=[%d]\n",pointerId, x, y);
  }

  while (offset < uibcBodyLen) {
    outData[offset++] = 0x00;
  }

  for (int i = 0; i < size; i++) {
    free(*(splitedStr + i));
  }
  free(splitedStr);

  binarydump(outData, uibcBodyLen);
  uibcmessage->m_DataValid = true;
  uibcmessage->m_PacketData = outData;
  uibcmessage->m_PacketDataLen = uibcBodyLen;
}

void hexdump(void *_data, size_t len)
{
  unsigned char *data = _data;
  size_t count;

  int line = 15;
  for (count = 0; count < len; count++) {
    if ((count & line) == 0) {
      fprintf(stderr,"%04zu: ", count);
    }
    fprintf(stderr,"%02x ", *data);
    data++;
    if ((count & line) == line) {
      fprintf(stderr,"\n");
    }
  }
  if ((count & line) != 0) {
    fprintf(stderr,"\n");
  }
}

void binarydump(void *_data, size_t len)
{
  unsigned char *data = _data;
  size_t count;

  int line = 7;
  for (count = 0; count < len; count++) {
    if ((count & line) == 0) {
      fprintf(stderr,"%04zu: ", count);
    }
    fprintf(stderr,"%s ", int2binary(*data, 8));
    data++;
    if ((count & line) == line) {
      fprintf(stderr,"\n");
    }
  }
  if ((count & line) != 0) {
    fprintf(stderr,"\n");
  }
}
// format: "typeId, Key code 1(0x00), Key code 2(0x00)"
void getUIBCGenericKeyPacket(const char *inEventDesc,
    UibcMessage* uibcmessage) {
  CDX_LOGD("getUIBCGenericKeyPacket (%s)", inEventDesc);
  char* outData = uibcmessage->m_PacketData;
  int32_t typeId = 0;
  int32_t uibcBodyLen = 0;
  int32_t genericPacketLen = 0;
  int32_t temp = 0;
  size_t size;
  char** splitedStr = str_split((char*)inEventDesc, ",", &size);

  if (size > 0) {

    if (((int)size) % 3 != 0) {
        CDX_LOGE("getUIBCGenericKeyPacket (%s)", "bad input event");
        return;
    }
    int i;
    for (i = 0; i < size; i++) {
      CDX_LOGD("getUIBCGenericKeyPacket splitedStr tokens=[%s]\n", *(splitedStr + i));

      switch (i) {
        case 0: {
                  typeId = atoi(*(splitedStr + i));
                  CDX_LOGD("getUIBCGenericKeyPacket typeId=[%d]\n", typeId);
                  genericPacketLen = 5;
                  uibcBodyLen = genericPacketLen + 7; // Generic header legth = 7
                  outData = (char*)malloc(uibcBodyLen + 1);
                  // UIBC header
                  outData[0] = 0x00; //Version (3 bits),T (1 bit),Reserved(8 bits),Input Category (4 bits)
                  outData[1] = 0x00; //Version (3 bits),T (1 bit),Reserved(8 bits),Input Category (4 bits)
                  outData[2] = (uibcBodyLen >> 8) & 0xFF; //Length(16 bits)
                  outData[3] = uibcBodyLen & 0xFF; //Length(16 bits)
                  //Generic Input Body Format
                  outData[4] = typeId & 0xFF; // Tyoe ID, 1 octet
                  outData[5] = (genericPacketLen >> 8) & 0xFF; // Length, 2 octets
                  outData[6] = genericPacketLen & 0xFF; // Length, 2 octets
                  outData[7] = 0x00; // reserved
                  break;
                }
        case 1: {
                  sscanf(*(splitedStr + i), " 0x%04X", &temp);
                  if (temp == 0) {
                    outData[8] = 0x00;
                    outData[9] = 0x00;
                  }
                  CDX_LOGD("getUIBCGenericKeyPacket key code 1=[%d]\n", temp);
                  outData[8] = (temp >> 8) & 0xFF;
                  outData[9] = temp & 0xFF;

                  break;
                }
        case 2: {
                  sscanf(*(splitedStr + i), " 0x%04X", &temp);
                  if (temp == 0) {
                    outData[10] = 0x00;
                    outData[11] = 0x00;
                  }
                  CDX_LOGD("getUIBCGenericKeyPacket key code 2=[%d]\n", temp);
                  outData[10] = (temp >> 8) & 0xFF;
                  outData[11] = temp & 0xFF;

                  break;
                }
        default: {
                 }
                 break;
      }
      free(*(splitedStr + i));
    }
  }
  free(splitedStr);

  binarydump(outData, uibcBodyLen);
  uibcmessage->m_DataValid = true;
  uibcmessage->m_PacketData = outData;
  uibcmessage->m_PacketDataLen = uibcBodyLen;
}

// format: "typeId,  X coordnate, Y coordnate, integer part, fraction part"
void getUIBCGenericZoomPacket(const char *inEventDesc, UibcMessage* uibcmessage) {
  CDX_LOGD("getUIBCGenericZoomPacket (%s)", inEventDesc);
  char* outData = uibcmessage->m_PacketData;
  int32_t typeId;
  int32_t genericPacketLen;
  int32_t uibcBodyLen = 0;
  int32_t xCoord, yCoord, integerPart, FractionPart;
  size_t size;

  char** splitedStr = str_split((char*)inEventDesc, ",", &size);

  if (splitedStr) {
    int i;
    for (i = 0; * (splitedStr + i); i++) {
      //CDX_LOGD("getUIBCGenericZoomPacket splitedStr tokens=[%s]\n", *(splitedStr + i));

      switch (i) {
        case 0: {
                  typeId = atoi(*(splitedStr + i));
                  //CDX_LOGD("getUIBCGenericZoomPacket typeId=[%d]\n", typeId);

                  genericPacketLen = 6;
                  uibcBodyLen = genericPacketLen + 7; // Generic herder leh = 7
                  outData = (char*)malloc(uibcBodyLen + 1);
                  // UIBC header
                  outData[0] = 0x00; //Version (3 bits),T (1 bit),Reserved(8 bits),Input Category (4 bits)
                  outData[1] = 0x00; //Version (3 bits),T (1 bit),Reserved(8 bits),Input Category (4 bits)
                  outData[2] = (uibcBodyLen >> 8) & 0xFF; //Length(16 bits)
                  outData[3] = uibcBodyLen & 0xFF; //Length(16 bits)
                  //Generic Input Body Format
                  outData[4] = typeId & 0xFF; // Tyoe ID, 1 octet
                  outData[5] = (genericPacketLen >> 8) & 0xFF; // Length, 2 octets
                  outData[6] = genericPacketLen & 0xFF; // Length, 2 octets
                  break;
                }

        case 1: {
                  xCoord = atoi(*(splitedStr + i));
                  outData[7] = (xCoord >> 8) & 0xFF;
                  outData[8] = xCoord & 0xFF;
                  CDX_LOGD("getUIBCGenericZoomPacket xCoord=[%d]\n", xCoord);
                  break;
                }
        case 2: {
                  yCoord = atoi(*(splitedStr + i));
                  CDX_LOGD("getUIBCGenericZoomPacket yCoord=[%d]\n", yCoord);
                  break;
                }
        case 3: {
                  integerPart = atoi(*(splitedStr + i));
                  outData[11] = integerPart & 0xFF;
                  //CDX_LOGD("getUIBCGenericZoomPacket integerPart=[%d]\n", integerPart);
                  break;
                }
        case 4: {
                  FractionPart = atoi(*(splitedStr + i));
                  outData[12] = FractionPart & 0xFF;
                  //CDX_LOGD("getUIBCGenericZoomPacket FractionPart=[%d]\n", FractionPart);

                  break;
                }
        default: {
                   break;
                 }
      }

      free(*(splitedStr + i));
    }
    free(splitedStr);
  }
  //hexdump(outData, uibcBodyLen);
  uibcmessage->m_DataValid = true;
  uibcmessage->m_PacketDataLen = uibcBodyLen;
}

// format: "typeId,  unit, direction, amount to scroll"
void getUIBCGenericScalePacket(const char *inEventDesc, UibcMessage* uibcmessage) {
  CDX_LOGD("getUIBCGenericScalePacket (%s)", inEventDesc);
  char* outData = uibcmessage->m_PacketData;
  int32_t typeId;
  int32_t uibcBodyLen = 0;
  int32_t genericPacketLen;
  int32_t temp;
  size_t size;
  char** splitedStr = str_split((char*)inEventDesc, ",", &size);

  if (splitedStr) {
    int i;
    for (i = 0; * (splitedStr + i); i++) {
      //CDX_LOGD("getUIBCGenericScalePacket splitedStr tokens=[%s]\n", *(splitedStr + i));

      switch (i) {
        case 0: {
                  typeId = atoi(*(splitedStr + i));
                  //CDX_LOGD("getUIBCGenericScalePacket typeId=[%d]\n", typeId);
                  genericPacketLen = 2;
                  uibcBodyLen = genericPacketLen + 7; // Generic herder leh = 7
                  outData = (char*)malloc(uibcBodyLen + 1);
                  // UIBC header
                  outData[0] = 0x00; //Version (3 bits),T (1 bit),Reserved(8 bits),Input Category (4 bits)
                  outData[1] = 0x00; //Version (3 bits),T (1 bit),Reserved(8 bits),Input Category (4 bits)
                  outData[2] = (uibcBodyLen >> 8) & 0xFF; //Length(16 bits)
                  outData[3] = uibcBodyLen & 0xFF; //Length(16 bits)
                  //Generic Input Body Format
                  outData[4] = typeId & 0xFF; // Tyoe ID, 1 octet
                  outData[5] = (genericPacketLen >> 8) & 0xFF; // Length, 2 octets
                  outData[6] = genericPacketLen & 0xFF; // Length, 2 octets
                  outData[7] = 0x00; // Clear the byte
                  outData[8] = 0x00; // Clear the byte
                  /*
                     B15B14; Scroll Unit Indication bits.
                     0b00; the unit is a pixel (normalized with respect to the WFD Source display resolution that is conveyed in an RTSP M4 request message).
                     0b01; the unit is a mouse notch (where the application is responsible for representing the number of pixels per notch).
                     0b10-0b11; Reserved.

                     B13; Scroll Direction Indication bit.
                     0b0; Scrolling to the right. Scrolling to the right means the displayed content being shifted to the left from a user perspective.
                     0b1; Scrolling to the left. Scrolling to the left means the displayed content being shifted to the right from a user perspective.

B12:B0; Number of Scroll bits.
Number of units for a Horizontal scroll.
*/
                  break;
                }
        case 1: {
                  temp = atoi(*(splitedStr + i));
                  //CDX_LOGD("getUIBCGenericScalePacket unit=[%d]\n", temp);
                  outData[7] = (temp >> 8) & 0xFF;
                  break;
                }
        case 2: {
                  temp = atoi(*(splitedStr + i));
                  //CDX_LOGD("getUIBCGenericScalePacket direction=[%d]\n", temp);
                  outData[7] |= ((temp >> 10) & 0xFF);
                  break;

                }
        case 3: {
                  temp = atoi(*(splitedStr + i));
                  //CDX_LOGD("getUIBCGenericScalePacket amount to scroll=[%d]\n", temp);
                  outData[7] |= ((temp >> 12) & 0xFF);
                  outData[8] = temp & 0xFF;

                  break;
                }
        default: {
                   break;
                 }
      }

      free(*(splitedStr + i));
    }

    free(splitedStr);
  }
  //hexdump(outData, uibcBodyLen);
  uibcmessage->m_DataValid = true;
  uibcmessage->m_PacketDataLen = uibcBodyLen;
}

// format: "typeId,  integer part, fraction part"
void getUIBCGenericRotatePacket(const char * inEventDesc, UibcMessage* uibcmessage) {
  CDX_LOGD("getUIBCGenericRotatePacket (%s)", inEventDesc);
  char* outData = uibcmessage->m_PacketData;
  int32_t typeId;
  int32_t genericPacketLen;
  int32_t uibcBodyLen = 0;
  int32_t integerPart, FractionPart;
  size_t size;

  char** splitedStr = str_split((char*)inEventDesc, ",", &size);

  if (splitedStr) {
    int i;
    for (i = 0; * (splitedStr + i); i++) {
      //CDX_LOGD("getUIBCGenericRotatePacket splitedStr tokens=[%s]\n", *(splitedStr + i));

      switch (i) {
        case 0: {
                  typeId = atoi(*(splitedStr + i));
                  //CDX_LOGD("getUIBCGenericRotatePacket typeId=[%d]\n", typeId);
                  genericPacketLen = 2;
                  uibcBodyLen = genericPacketLen + 7; // Generic herder leh = 7
                  outData = (char*)malloc(uibcBodyLen + 1);
                  // UIBC header
                  outData[0] = 0x00; //Version (3 bits),T (1 bit),Reserved(8 bits),Input Category (4 bits)
                  outData[1] = 0x00; //Version (3 bits),T (1 bit),Reserved(8 bits),Input Category (4 bits)
                  outData[2] = (uibcBodyLen >> 8) & 0xFF; //Length(16 bits)
                  outData[3] = uibcBodyLen & 0xFF; //Length(16 bits)
                  //Generic Input Body Format
                  outData[4] = typeId & 0xFF; // Tyoe ID, 1 octet
                  outData[5] = (genericPacketLen >> 8) & 0xFF; // Length, 2 octets
                  outData[6] = genericPacketLen & 0xFF; // Length, 2 octets
                  break;
                }
        case 1: {
                  integerPart = atoi(*(splitedStr + i));
                  outData[7] = integerPart & 0xFF;
                  //CDX_LOGD("getUIBCGenericRotatePacket integerPart=[%d]\n", integerPart);
                  break;
                }
        case 2: {
                  FractionPart = atoi(*(splitedStr + i));
                  outData[8] = FractionPart & 0xFF;
                  //CDX_LOGD("getUIBCGenericRotatePacket FractionPart=[%d]\n", FractionPart);

                  break;
                }
        default: {
                   break;
                 }
      }
      free(*(splitedStr + i));
    }

    free(splitedStr);
  }
  //hexdump(outData, uibcBodyLen);
  uibcmessage->m_DataValid = true;
  uibcmessage->m_PacketDataLen = uibcBodyLen;
}


char** str_split(char* pStr, const char* pDelim, size_t* size) {
  char** result    = 0;
  size_t count     = 0;
  char* tmp        = pStr;
  char* tmpStr     = NULL;
  char* last_comma = 0;

  asprintf(&tmpStr, "%s", pStr);

  /* Count how many elements will be extracted. */
  while (*tmp) {
    if (*pDelim == *tmp) {
      count++;
      last_comma = tmp;
    }
    tmp++;
  }

  /* Add space for trailing token. */
  count += last_comma < (pStr + strlen(pStr) - 1) ? 1 : 0;

  result = (char**)malloc(sizeof(char*) * count);

  *size = count;

  tmp = tmpStr = strdup(pStr);
  size_t idx  = 0;
  char* token;
  while ((token = strsep(&tmp, pDelim)) != NULL) {
    * (result + idx++) = strdup(token);
  }
  free(tmpStr);
  return result;
}

UIBCContextT *UIBC_Instance(char const* host, unsigned short port, struct config_t* negotiated_video_format)
{
	struct UIBCImplS *impl = malloc(sizeof(*impl));
	CDX_LOGD("Instance.");

	memset(impl, 0x00, sizeof(*impl));

	sprintf(impl->host, "%s", host);
	impl->port = port;
	impl->exit = 0;
	impl->state = 0;
    impl->nfds = 0;
    INIT_LIST_HEAD(&impl->inputDevListHead);

    if (negotiated_video_format == NULL) {
        impl->video_width  = 1920;
        impl->video_height = 1080;
    } else {
        impl->video_width  = negotiated_video_format->width;
        impl->video_height = negotiated_video_format->height;
        free(negotiated_video_format);
        CDX_LOGD("get video resolution from WFDSource: %dx%d", impl->video_width, impl->video_height);
    }

    scan_dir(impl, DEVICE_PATH);

	if (list_empty(&impl->inputDevListHead))
	{
		CDX_LOGE("ERROR, find null input device node in path %s", DEVICE_PATH);
		free(impl);
		impl = NULL;
		return NULL;
	}

    impl->evfd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    impl->pfds[impl->nfds].fd = impl->evfd;
    impl->pfds[impl->nfds++].events = POLLIN;

	pthread_create(&impl->pid, NULL, MainThread, impl);

	while (impl->state == 0)
	{
		usleep(10000);
	}
	if (impl->state == 1)
	{
		CDX_LOGD("Instance Finish.");
	}
	else if (impl->state == -1)
	{
		CDX_LOGE("ERROR, error in thread.");
		free(impl);
		impl = NULL;
	}
	return impl;
}

int UIBC_Destroy(UIBCContextT *ctx)
{
	struct UIBCImplS *impl = ctx;
    if (impl == NULL) {
        return 0;
    }
	impl->exit = 3;
    write(impl->evfd, &impl->exit, sizeof(impl->exit));
    pthread_join(impl->pid, NULL);
    inputdev_close(impl);
    close(impl->evfd);
	free(impl);

	return 0;
}


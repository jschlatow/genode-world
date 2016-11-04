
#pragma once
#define BIT(x)	(1UL << (x))
#define CLR_BIT(p,n) ((p) &= ~((1) << (n)))
#define SET_BIT(p,n) ((p) |= (1 << (n)))

#define HDMI_IEEE_OUI 0x000c03
#define HDMI_INFOFRAME_HEADER_SIZE  4
#define HDMI_AVI_INFOFRAME_SIZE    13
#define HDMI_SPD_INFOFRAME_SIZE    25
#define HDMI_AUDIO_INFOFRAME_SIZE  10


enum hdmi_infoframe_type {
	HDMI_INFOFRAME_TYPE_VENDOR = 0x81,
	HDMI_INFOFRAME_TYPE_AVI = 0x82,
	HDMI_INFOFRAME_TYPE_SPD = 0x83,
	HDMI_INFOFRAME_TYPE_AUDIO = 0x84,
};

enum hdmi_colorspace {
	HDMI_COLORSPACE_RGB,
	HDMI_COLORSPACE_YUV422,
	HDMI_COLORSPACE_YUV444,
	HDMI_COLORSPACE_YUV420,
	HDMI_COLORSPACE_RESERVED4,
	HDMI_COLORSPACE_RESERVED5,
	HDMI_COLORSPACE_RESERVED6,
	HDMI_COLORSPACE_IDO_DEFINED,
};

enum hdmi_scan_mode {
	HDMI_SCAN_MODE_NONE,
	HDMI_SCAN_MODE_OVERSCAN,
	HDMI_SCAN_MODE_UNDERSCAN,
	HDMI_SCAN_MODE_RESERVED,
};

enum hdmi_colorimetry {
	HDMI_COLORIMETRY_NONE,
	HDMI_COLORIMETRY_ITU_601,
	HDMI_COLORIMETRY_ITU_709,
	HDMI_COLORIMETRY_EXTENDED,
};

enum hdmi_picture_aspect {
	HDMI_PICTURE_ASPECT_NONE,
	HDMI_PICTURE_ASPECT_4_3,
	HDMI_PICTURE_ASPECT_16_9,
	HDMI_PICTURE_ASPECT_RESERVED,
};

enum hdmi_active_aspect {
	HDMI_ACTIVE_ASPECT_16_9_TOP = 2,
	HDMI_ACTIVE_ASPECT_14_9_TOP = 3,
	HDMI_ACTIVE_ASPECT_16_9_CENTER = 4,
	HDMI_ACTIVE_ASPECT_PICTURE = 8,
	HDMI_ACTIVE_ASPECT_4_3 = 9,
	HDMI_ACTIVE_ASPECT_16_9 = 10,
	HDMI_ACTIVE_ASPECT_14_9 = 11,
	HDMI_ACTIVE_ASPECT_4_3_SP_14_9 = 13,
	HDMI_ACTIVE_ASPECT_16_9_SP_14_9 = 14,
	HDMI_ACTIVE_ASPECT_16_9_SP_4_3 = 15,
};

enum hdmi_extended_colorimetry {
	HDMI_EXTENDED_COLORIMETRY_XV_YCC_601,
	HDMI_EXTENDED_COLORIMETRY_XV_YCC_709,
	HDMI_EXTENDED_COLORIMETRY_S_YCC_601,
	HDMI_EXTENDED_COLORIMETRY_ADOBE_YCC_601,
	HDMI_EXTENDED_COLORIMETRY_ADOBE_RGB,

	/* The following EC values are only defined in CEA-861-F. */
	HDMI_EXTENDED_COLORIMETRY_BT2020_CONST_LUM,
	HDMI_EXTENDED_COLORIMETRY_BT2020,
	HDMI_EXTENDED_COLORIMETRY_RESERVED,
};

enum hdmi_quantization_range {
	HDMI_QUANTIZATION_RANGE_DEFAULT,
	HDMI_QUANTIZATION_RANGE_LIMITED,
	HDMI_QUANTIZATION_RANGE_FULL,
	HDMI_QUANTIZATION_RANGE_RESERVED,
};

/* non-uniform picture scaling */
enum hdmi_nups {
	HDMI_NUPS_UNKNOWN,
	HDMI_NUPS_HORIZONTAL,
	HDMI_NUPS_VERTICAL,
	HDMI_NUPS_BOTH,
};

enum hdmi_ycc_quantization_range {
	HDMI_YCC_QUANTIZATION_RANGE_LIMITED,
	HDMI_YCC_QUANTIZATION_RANGE_FULL,
};

enum hdmi_content_type {
	HDMI_CONTENT_TYPE_GRAPHICS,
	HDMI_CONTENT_TYPE_PHOTO,
	HDMI_CONTENT_TYPE_CINEMA,
	HDMI_CONTENT_TYPE_GAME,
};

struct hdmi_avi_infoframe {
	enum hdmi_infoframe_type type;
	unsigned char version;
	unsigned char length;
	enum hdmi_colorspace colorspace;
	enum hdmi_scan_mode scan_mode;
	enum hdmi_colorimetry colorimetry;
	enum hdmi_picture_aspect picture_aspect;
	enum hdmi_active_aspect active_aspect;
	bool itc;
	enum hdmi_extended_colorimetry extended_colorimetry;
	enum hdmi_quantization_range quantization_range;
	enum hdmi_nups nups;
	unsigned char video_code;
	enum hdmi_ycc_quantization_range ycc_quantization_range;
	enum hdmi_content_type content_type;
	unsigned char pixel_repeat;
	unsigned short top_bar;
	unsigned short bottom_bar;
	unsigned short left_bar;
	unsigned short right_bar;
};

static uint8_t  hdmi_infoframe_checksum(uint8_t *ptr, size_t size) {
    uint8_t csum = 0;
    size_t i;

    /* compute checksum */
    for (i = 0; i < size; i++)
        csum += ptr[i];

    return 256 - csum;
};
static void hdmi_infoframe_set_checksum(void *buffer, size_t size)
{
    uint8_t *ptr = (uint8_t*)buffer;

    ptr[3] = hdmi_infoframe_checksum((uint8_t*)buffer, size);
};
size_t hdmi_avi_infoframe_pack(struct hdmi_avi_infoframe *frame, 
        void *buffer, size_t size) {
    uint8_t *ptr = (uint8_t*)buffer;
    size_t length;

    length = HDMI_INFOFRAME_HEADER_SIZE + frame->length;

    if (size < length)
        return -1;

    //memset(buffer, 0, size);
    Genode::memset(buffer, 0, size);

    ptr[0] = frame->type;
    ptr[1] = frame->version;
    ptr[2] = frame->length;
    ptr[3] = 0; /* checksum */

    /* start infoframe payload */
    ptr += HDMI_INFOFRAME_HEADER_SIZE;

    ptr[0] = ((frame->colorspace & 0x3) << 5) | (frame->scan_mode & 0x3);

    /*
     * Data byte 1, bit 4 has to be set if we provide the active format
     * aspect ratio
     */
    if (frame->active_aspect & 0xf)
        ptr[0] |= BIT(4);

    /* Bit 3 and 2 indicate if we transmit horizontal/vertical bar data */
    if (frame->top_bar || frame->bottom_bar)
        ptr[0] |= BIT(3);

    if (frame->left_bar || frame->right_bar)
        ptr[0] |= BIT(2);

    ptr[1] = ((frame->colorimetry & 0x3) << 6) |
        ((frame->picture_aspect & 0x3) << 4) |
        (frame->active_aspect & 0xf);

    ptr[2] = ((frame->extended_colorimetry & 0x7) << 4) |
        ((frame->quantization_range & 0x3) << 2) |
        (frame->nups & 0x3);

    if (frame->itc)
        ptr[2] |= BIT(7);

    ptr[3] = frame->video_code & 0x7f;

    ptr[4] = ((frame->ycc_quantization_range & 0x3) << 6) |
        ((frame->content_type & 0x3) << 4) |
        (frame->pixel_repeat & 0xf);

    ptr[5] = frame->top_bar & 0xff;
    ptr[6] = (frame->top_bar >> 8) & 0xff;
    ptr[7] = frame->bottom_bar & 0xff;
    ptr[8] = (frame->bottom_bar >> 8) & 0xff;
    ptr[9] = frame->left_bar & 0xff;
    ptr[10] = (frame->left_bar >> 8) & 0xff;
    ptr[11] = frame->right_bar & 0xff;
    ptr[12] = (frame->right_bar >> 8) & 0xff;

    hdmi_infoframe_set_checksum(buffer, length);

    return length;
};

int hdmi_avi_infoframe_init(struct hdmi_avi_infoframe *frame) {
        Genode::memset(frame, 0, sizeof(*frame));
 
         frame->type    = HDMI_INFOFRAME_TYPE_AVI;
         frame->version = 2;
         frame->length  = HDMI_AVI_INFOFRAME_SIZE;
 
         return 0;
 };

#include "asf_parser.h"

namespace lge {
namespace mm {

static const char header_guid[] = "\x30\x26\xB2\x75\x8E\x66\xCF\x11\xA6\xD9\x00\xAA\x00\x62\xCE\x6C";
static const char stream_properties_guid[] = "\x91\x07\xDC\xB7\xB7\xA9\xCF\x11\x8E\xE6\x00\xC0\x0C\x20\x53\x65";
static const char stream_type_audio_guid[] = "\x40\x9E\x69\xF8\x4D\x5B\xCF\x11\xA8\xFD\x00\x80\x5F\x5C\x44\x2B";
static const char stream_type_video_guid[] = "\xC0\xEF\x19\xBC\x4D\x5B\xCF\x11\xA8\xFD\x00\x80\x5F\x5C\x44\x2B";

/**
 * ================================================================================
 * @fn : toNumber
 * @brief : read each byte of byte array, convert it to number (long long)
 * @section : Function flow (Pseudo-code or Decision Table)
 * - read byte as large as type size, although data size is greater than type size.
 * @param[in] data : byte array including number data.
 * @param[in] type_size : size of data type.
 * @param[in] data_size : size of data array.
 * @section Global Variables: none
 * @section Dependencies: none
 * @return long long
 * ===================================================================================
 */
long long AsfParser::toNumber(const char *data, unsigned int type_size, unsigned int data_size)
{
    long long sum = 0;
    unsigned int last, i;

    last = data_size > type_size ? type_size : data_size;

    for (i = 0; i < last; i++)
        sum |= (unsigned char) (data[i]) << (i * 8);

    return sum;
}

/**
 * ================================================================================
 * @fn : readQword
 * @brief : read 8bytes from file descriptor(fd) and return word value.
 * @section : Function flow (Pseudo-code or Decision Table)
 * - read 8 bytes from file descripter and convert it to number and return it.
 * @param[in] fd : file descriptor.
 * @section Global Variables: none
 * @section Dependencies: none
 * @return long long
 * ===================================================================================
 */
long long AsfParser::readQword(int fd)
{
    char v[8];
    if (read(fd, &v, 8) != 8)
        return 0;
    return toNumber(v, sizeof(unsigned long long), 8);
}

/**
 * ================================================================================
 * @fn : parseStreamProperties
 * @brief : get stream property. (video or audio or invalid)
 * @section : Function flow
 * - if file does not have ASF hearder return invalid type
 * - get audio stream and video stream count
 * - if video stream count > 0 return video type.
 * - else if strem count > 0 return audio type.
 * - else return invaild type.
 * @param[in] filename : asf file name.
 * @section Global Variables : none
 * @section Dependencies : none
 * @return : AsfMediaType
 * ===================================================================================
 */
int AsfParser::parseStreamProperties(int fd, int *type)
{
    struct {
        char stream_type[16];
        char error_correction_type[16];
        uint64_t time_offset;
        uint32_t type_specific_len;
        uint32_t error_correction_data_len;
        uint16_t flags;
        uint32_t reserved; /* don't use, unaligned */
    } __attribute__((packed)) props;
    unsigned int stream_id;
    int r;

    r = read(fd, &props, sizeof(props));
    if (r != sizeof(props))
        return r;

    stream_id = le16toh(props.flags) & 0x7F;
    /* Not a valid stream */
    if (!stream_id)
        return r;

    if (memcmp(props.stream_type, stream_type_audio_guid, 16) == 0)
        *type = 0x111; /* audio */
    if (memcmp(props.stream_type, stream_type_video_guid, 16) == 0)
        *type = 0x222; /* video */
    else if(*type == 0)
        return -1;

    return 0;
}

/**
 * ================================================================================
 * @fn : getMediaType
 * @brief : Get type of ASF file. Chek if it has audio or video or both.
 * @section : Function flow
 * - if file does not have ASF hearder return invalid type
 * - get audio stream and video stream count
 * - if video stream count > 0 return video type.
 * - else if strem count > 0 return audio type.
 * - else return invaild type.
 * @param[in] filename : asf file name.
 * @section Global Variables : none
 * @section Dependencies : none
 * @return : AsfMediaType
 * ===================================================================================
 */
AsfMediaType AsfParser::getMediaType(char* fileName)
{
    char guid[16];
    int type = 0;
    unsigned long long hdrsize;
    off_t pos_end, pos = 0;
    unsigned int size;

    int fd = open(fileName, O_RDONLY);

    if (fd < 0)
        return AsfInvalid;

    if (read(fd, &guid, 16) != 16)
    {
        close(fd);
        return AsfInvalid;
    }

    if (memcmp(guid, header_guid, 16) != 0)
    {
        close(fd);
        return AsfInvalid;
    }

    hdrsize = readQword(fd);
    pos_end = lseek(fd, 6, SEEK_CUR) - 24 + hdrsize;

    int audioStreamCount = 0;
    int videoStreamCount = 0;
    while (true)
    {
        if (!pos)
            pos = lseek(fd, 0, SEEK_CUR);

        if (pos > pos_end - 24)
            break;

        if ( (size = read(fd, &guid, 16)) != 16)
            break;

        size = readQword(fd);

        if (memcmp(guid, stream_properties_guid, 16) == 0) {
            parseStreamProperties(fd, &type);
            if (type == 0x111)
                audioStreamCount++;
            else if (type == 0x222)
                videoStreamCount++;
        }
        pos = lseek(fd, pos + size, SEEK_SET);
    }
    close(fd);

    if(videoStreamCount > 0) {
        return AsfVideo;
    } else if (audioStreamCount > 0) {
        return AsfAudio;
    } else {
        return AsfInvalid;
    }

}

}
}

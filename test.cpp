/*
 * test.h
 *
 *  Created on: 8 мая 2018 г.
 *      Author: olegvedi@gmail.com
 */

#include <stdio.h>
#include <string.h>

#include "ReadAVI.h"

int main(int argc, char *argv[])
{
    if (argc < 2) {
	printf("Use %s file.avi\n", argv[0]);
	return 1;
    }

    ReadAVI readAVI(argv[1]);

    int indexes = readAVI.GetIndexCnt();
    printf("indexes:%d\n", indexes);

    ReadAVI::stream_format_t stream_format = readAVI.GetVideoFormat();

    printf("compression_type:%s\n", stream_format.compression_type);

    const char *ext = "raw";
    if (!strcmp(stream_format.compression_type, "MJPG")) {
	ext = "jpg";
    } else if (!strcmp(stream_format.compression_type, "MPNG")) {
	ext = "png";
    }

    ReadAVI::frame_entry_t frame_entry;
    frame_entry.type = ReadAVI::ctype_video_data;
    frame_entry.pointer = 0;

#define MAX_FRAMES 20

    char filename[100];
    int frame_size;
    int cnt = 0;
    while ((frame_size = readAVI.GetFrameFromIndex(&frame_entry)) >= 0) {
	if (frame_size > 0) {
	    FILE *fp;
	    sprintf(filename, "out/frame_%4.4d.%s", cnt, ext);
	    fp = fopen(filename, "wb");
	    if (fp) {
		fwrite(frame_entry.buf, 1, frame_size, fp);
		fclose(fp);
	    } else
		break;

	    cnt++;
	    if(cnt >= MAX_FRAMES)
		break;
	}
    }

    return 0;
}


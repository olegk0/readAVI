/*
 * player.h
 *
 *  Created on: 8 мая 2018 г.
 *      Author: olegvedi@gmail.com
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <SDL2/SDL.h>
#include <png.h>

#include "ReadAVI.h"

typedef struct {
    unsigned char *buf;
    unsigned len;
    unsigned pos;
} read_buf_str_t;

void ReadBuf(
    png_structp png_ptr, png_byte* raw_data, png_size_t read_length) {

    read_buf_str_t* read_buf_str = (read_buf_str_t*)png_get_io_ptr(png_ptr);
    const png_byte* src = read_buf_str->buf + read_buf_str->pos;
    memcpy(raw_data, src, read_length);
    read_buf_str->pos += read_length;
}

int UnpackPNG(unsigned char *src_buf, unsigned src_length, unsigned bpp, unsigned char *dst_buf)
{
    int ret = 0;
    png_infop info_ptr = NULL, end_info = NULL;
    png_structp png_ptr = NULL;
    png_byte header[8];
    png_byte *image_data = NULL;
    png_bytep *row_pointers = NULL;

    // read the header
    memcpy(header, src_buf, 8);

    if (png_sig_cmp(header, 0, 8)) {
	fprintf(stderr, "error: is not a PNG.\n");
	return -1;
    }

    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr) {
	fprintf(stderr, "error: png_create_read_struct returned 0.\n");
	return -1;
    }

    try {
	// create png info struct
	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr) {
	    fprintf(stderr, "error: png_create_info_struct returned 0.\n");
	    throw 1;
	}

	// create png info struct
	end_info = png_create_info_struct(png_ptr);
	if (!end_info) {
	    fprintf(stderr, "error: png_create_info_struct returned 0.\n");
	    throw 1;
	}

	// the code in this if statement gets called if libpng encounters an error
	if (setjmp(png_jmpbuf(png_ptr))) {
	    fprintf(stderr, "error from libpng\n");
	    throw 1;
	}

	// init png reading
	//png_init_io(png_ptr, fp);
	read_buf_str_t read_buf_str;
	read_buf_str.buf = src_buf;
	read_buf_str.len = src_length;
	read_buf_str.pos = 8;

	png_set_read_fn(png_ptr, &read_buf_str, ReadBuf);

	// let libpng know you already read the first 8 bytes
	png_set_sig_bytes(png_ptr, 8);

	// read all the info up to the image data
	png_read_info(png_ptr, info_ptr);

	// variables to pass to get info
	int bit_depth, color_type;
	png_uint_32 temp_width, temp_height;

	// get info about png
	png_get_IHDR(png_ptr, info_ptr, &temp_width, &temp_height, &bit_depth, &color_type, NULL, NULL, NULL);

	// Update the png info struct.
	png_read_update_info(png_ptr, info_ptr);

	// Row size in bytes.
	int rowbytes = png_get_rowbytes(png_ptr, info_ptr);

	// glTexImage2d requires rows to be 4-byte aligned
	rowbytes += 3 - ((rowbytes - 1) % 4);

	// Allocate the image_data as a big block, to be given to opengl
	image_data = (png_byte *) malloc(rowbytes * temp_height * sizeof(png_byte) + 15);
	if (image_data == NULL) {
	    fprintf(stderr, "error: could not allocate memory for PNG image data\n");
	    throw 1;
	}

	// row_pointers is for pointing to image_data for reading the png with libpng
	row_pointers = (png_bytep *) malloc(temp_height * sizeof(png_bytep));
	if (row_pointers == NULL) {
	    fprintf(stderr, "error: could not allocate memory for PNG row pointers\n");
	    throw 1;
	}

	// set the individual row_pointers to point at the correct offsets of image_data
	unsigned i;
	for (i = 0; i < temp_height; i++) {
//	    row_pointers[temp_height - 1 - i] = image_data + i * rowbytes;
	    row_pointers[i] = image_data + i * rowbytes;
	}

	// read the png into image_data through row_pointers
	png_read_image(png_ptr, row_pointers);

	memcpy(dst_buf, image_data, temp_width * temp_height * bpp / 8);

	// clean up
	png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
	free(image_data);
	free(row_pointers);

    } catch (...) {
	if (png_ptr)
	    png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
	if (image_data)
	    free(image_data);
	if (row_pointers)
	    free(row_pointers);
	ret = -1;
    }

    return ret;
}

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
    ReadAVI::avi_header_t avi_header = readAVI.GetAviHeader();

    printf("compression_type:%s\n", stream_format.compression_type);

    if (strcmp(stream_format.compression_type, "MPNG")) {
	printf("Only MPNG supported\n");
	exit(1);
    }

    int width = stream_format.image_width, height = stream_format.image_height;
    SDL_Window *MainWindow = SDL_CreateWindow("My Game Window", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width,
	    height, 0);
    if (!MainWindow)
	exit(1);

    SDL_Surface *MainSurface = SDL_GetWindowSurface(MainWindow);

    if (MainSurface) {
	SDL_Surface *BufSurface = SDL_CreateRGBSurface(0, width, height, stream_format.bits_per_pixel, 0, 0, 0, 0);

	ReadAVI::frame_entry_t frame_entry;
	frame_entry.type = ReadAVI::ctype_video_data;
	frame_entry.pointer = 0;

	int frame_size;
	while ((frame_size = readAVI.GetFrameFromIndex(&frame_entry)) >= 0) {
	    if (frame_size > 0) {
		SDL_LockSurface(BufSurface);
		int ret = UnpackPNG(frame_entry.buf, frame_size, stream_format.bits_per_pixel, (unsigned char*)BufSurface->pixels);
		SDL_UnlockSurface(BufSurface);
		if (ret == 0) {
		    SDL_BlitSurface(BufSurface, NULL, MainSurface, NULL);
		    SDL_UpdateWindowSurface(MainWindow);
		}
	    }

	    usleep(avi_header.TimeBetweenFrames);
	}

	SDL_FreeSurface(MainSurface);
    }

    SDL_DestroyWindow(MainWindow);

    return 0;
}


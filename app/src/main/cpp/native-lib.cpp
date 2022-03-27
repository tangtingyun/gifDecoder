#include <jni.h>
#include <string>

extern "C" {
#include <libavutil/avutil.h>
#include <webp/decode.h>
#include "imageio/image_dec.h"
#include "imageio/image_enc.h"
#include "imageio/imageio_util.h"
}


void decodeWebp(char *path) {
    int ok = 0;
    const char *in_file = NULL;
    const char *out_file = NULL;

    WebPDecoderConfig config;
    WebPDecBuffer *const output_buffer = &config.output;
    WebPBitstreamFeatures *const bitstream = &config.input;
    WebPOutputFileFormat format = PNG;
    uint8_t *external_buffer = NULL;
    int use_external_memory = 0;
    const uint8_t *data = NULL;

    int incremental = 0;
    int c;


    if (!WebPInitDecoderConfig(&config)) {
        fprintf(stderr, "Library version mismatch!\n");
    }

    for (c = 1; c < argc; ++c) {
        int parse_error = 0;
        if (!strcmp(argv[c], "-h") || !strcmp(argv[c], "-help")) {
            Help();
            FREE_WARGV_AND_RETURN(0);
        } else if (!strcmp(argv[c], "-o") && c < argc - 1) {
            out_file = (const char *) GET_WARGV(argv, ++c);
        } else if (!strcmp(argv[c], "-alpha")) {
            format = ALPHA_PLANE_ONLY;
        } else if (!strcmp(argv[c], "-nofancy")) {
            config.options.no_fancy_upsampling = 1;
        } else if (!strcmp(argv[c], "-nofilter")) {
            config.options.bypass_filtering = 1;
        } else if (!strcmp(argv[c], "-pam")) {
            format = PAM;
        } else if (!strcmp(argv[c], "-ppm")) {
            format = PPM;
        } else if (!strcmp(argv[c], "-bmp")) {
            format = BMP;
        } else if (!strcmp(argv[c], "-tiff")) {
            format = TIFF;
        } else if (!strcmp(argv[c], "-quiet")) {
            quiet = 1;
        } else if (!strcmp(argv[c], "-version")) {
            const int version = WebPGetDecoderVersion();
            printf("%d.%d.%d\n",
                   (version >> 16) & 0xff, (version >> 8) & 0xff, version & 0xff);
            FREE_WARGV_AND_RETURN(0);
        } else if (!strcmp(argv[c], "-pgm")) {
            format = PGM;
        } else if (!strcmp(argv[c], "-yuv")) {
            format = RAW_YUV;
        } else if (!strcmp(argv[c], "-pixel_format") && c < argc - 1) {
            const char *const fmt = argv[++c];
            if (!strcmp(fmt, "RGB")) format = RGB;
            else if (!strcmp(fmt, "RGBA")) format = RGBA;
            else if (!strcmp(fmt, "BGR")) format = BGR;
            else if (!strcmp(fmt, "BGRA")) format = BGRA;
            else if (!strcmp(fmt, "ARGB")) format = ARGB;
            else if (!strcmp(fmt, "RGBA_4444")) format = RGBA_4444;
            else if (!strcmp(fmt, "RGB_565")) format = RGB_565;
            else if (!strcmp(fmt, "rgbA")) format = rgbA;
            else if (!strcmp(fmt, "bgrA")) format = bgrA;
            else if (!strcmp(fmt, "Argb")) format = Argb;
            else if (!strcmp(fmt, "rgbA_4444")) format = rgbA_4444;
            else if (!strcmp(fmt, "YUV")) format = YUV;
            else if (!strcmp(fmt, "YUVA")) format = YUVA;
            else {
                fprintf(stderr, "Can't parse pixel_format %s\n", fmt);
                parse_error = 1;
            }
        } else if (!strcmp(argv[c], "-external_memory") && c < argc - 1) {
            use_external_memory = ExUtilGetInt(argv[++c], 0, &parse_error);
            parse_error |= (use_external_memory > 2 || use_external_memory < 0);
            if (parse_error) {
                fprintf(stderr, "Can't parse 'external_memory' value %s\n", argv[c]);
            }
        } else if (!strcmp(argv[c], "-mt")) {
            config.options.use_threads = 1;
        } else if (!strcmp(argv[c], "-alpha_dither")) {
            config.options.alpha_dithering_strength = 100;
        } else if (!strcmp(argv[c], "-nodither")) {
            config.options.dithering_strength = 0;
        } else if (!strcmp(argv[c], "-dither") && c < argc - 1) {
            config.options.dithering_strength =
                    ExUtilGetInt(argv[++c], 0, &parse_error);
        } else if (!strcmp(argv[c], "-crop") && c < argc - 4) {
            config.options.use_cropping = 1;
            config.options.crop_left = ExUtilGetInt(argv[++c], 0, &parse_error);
            config.options.crop_top = ExUtilGetInt(argv[++c], 0, &parse_error);
            config.options.crop_width = ExUtilGetInt(argv[++c], 0, &parse_error);
            config.options.crop_height = ExUtilGetInt(argv[++c], 0, &parse_error);
        } else if ((!strcmp(argv[c], "-scale") || !strcmp(argv[c], "-resize")) &&
                   c < argc - 2) {  // '-scale' is left for compatibility
            config.options.use_scaling = 1;
            config.options.scaled_width = ExUtilGetInt(argv[++c], 0, &parse_error);
            config.options.scaled_height = ExUtilGetInt(argv[++c], 0, &parse_error);
        } else if (!strcmp(argv[c], "-flip")) {
            config.options.flip = 1;
        } else if (!strcmp(argv[c], "-v")) {
            verbose = 1;
#ifndef WEBP_DLL
        } else if (!strcmp(argv[c], "-noasm")) {
            VP8GetCPUInfo = NULL;
#endif
        } else if (!strcmp(argv[c], "-incremental")) {
            incremental = 1;
        } else if (!strcmp(argv[c], "--")) {
            if (c < argc - 1) in_file = (const char *) GET_WARGV(argv, ++c);
            break;
        } else if (argv[c][0] == '-') {
            fprintf(stderr, "Unknown option '%s'\n", argv[c]);
            Help();
            FREE_WARGV_AND_RETURN(-1);
        } else {
            in_file = (const char *) GET_WARGV(argv, c);
        }

        if (parse_error) {
            Help();
            FREE_WARGV_AND_RETURN(-1);
        }
    }

    if (in_file == NULL) {
        fprintf(stderr, "missing input file!!\n");
        Help();
        FREE_WARGV_AND_RETURN(-1);
    }

    if (quiet) verbose = 0;

    {
        VP8StatusCode status = VP8_STATUS_OK;
        size_t data_size = 0;
        if (!LoadWebP(in_file, &data, &data_size, bitstream)) {
            FREE_WARGV_AND_RETURN(-1);
        }

        switch (format) {
            case PNG:
#ifdef HAVE_WINCODEC_H
                output_buffer->colorspace = bitstream->has_alpha ? MODE_BGRA : MODE_BGR;
#else
                output_buffer->colorspace = bitstream->has_alpha ? MODE_RGBA : MODE_RGB;
#endif
                break;
            case PAM:
                output_buffer->colorspace = MODE_RGBA;
                break;
            case PPM:
                output_buffer->colorspace = MODE_RGB;  // drops alpha for PPM
                break;
            case BMP:
                output_buffer->colorspace = bitstream->has_alpha ? MODE_BGRA : MODE_BGR;
                break;
            case TIFF:
                output_buffer->colorspace = bitstream->has_alpha ? MODE_RGBA : MODE_RGB;
                break;
            case PGM:
            case RAW_YUV:
                output_buffer->colorspace = bitstream->has_alpha ? MODE_YUVA : MODE_YUV;
                break;
            case ALPHA_PLANE_ONLY:
                output_buffer->colorspace = MODE_YUVA;
                break;
                // forced modes:
            case RGB:
                output_buffer->colorspace = MODE_RGB;
                break;
            case RGBA:
                output_buffer->colorspace = MODE_RGBA;
                break;
            case BGR:
                output_buffer->colorspace = MODE_BGR;
                break;
            case BGRA:
                output_buffer->colorspace = MODE_BGRA;
                break;
            case ARGB:
                output_buffer->colorspace = MODE_ARGB;
                break;
            case RGBA_4444:
                output_buffer->colorspace = MODE_RGBA_4444;
                break;
            case RGB_565:
                output_buffer->colorspace = MODE_RGB_565;
                break;
            case rgbA:
                output_buffer->colorspace = MODE_rgbA;
                break;
            case bgrA:
                output_buffer->colorspace = MODE_bgrA;
                break;
            case Argb:
                output_buffer->colorspace = MODE_Argb;
                break;
            case rgbA_4444:
                output_buffer->colorspace = MODE_rgbA_4444;
                break;
            case YUV:
                output_buffer->colorspace = MODE_YUV;
                break;
            case YUVA:
                output_buffer->colorspace = MODE_YUVA;
                break;
            default:
                goto Exit;
        }

        if (use_external_memory > 0 && format >= RGB) {
            external_buffer = AllocateExternalBuffer(&config, format,
                                                     use_external_memory);
            if (external_buffer == NULL) goto Exit;
        }

        {
            Stopwatch stop_watch;
            if (verbose) StopwatchReset(&stop_watch);

            if (incremental) {
                status = DecodeWebPIncremental(data, data_size, &config);
            } else {
                status = DecodeWebP(data, data_size, &config);
            }
            if (verbose) {
                const double decode_time = StopwatchReadAndReset(&stop_watch);
                fprintf(stderr, "Time to decode picture: %.3fs\n", decode_time);
            }
        }

        ok = (status == VP8_STATUS_OK);
        if (!ok) {
            PrintWebPError(in_file, status);
            goto Exit;
        }
    }

    if (out_file != NULL) {
        if (!quiet) {
            WFPRINTF(stderr, "Decoded %s.", (const W_CHAR *) in_file);
            fprintf(stderr, " Dimensions: %d x %d %s. Format: %s. Now saving...\n",
                    output_buffer->width, output_buffer->height,
                    bitstream->has_alpha ? " (with alpha)" : "",
                    kFormatType[bitstream->format]);
        }
        ok = SaveOutput(output_buffer, format, out_file);
    } else {
        if (!quiet) {
            WFPRINTF(stderr, "File %s can be decoded ", (const W_CHAR *) in_file);
            fprintf(stderr, "(dimensions: %d x %d %s. Format: %s).\n",
                    output_buffer->width, output_buffer->height,
                    bitstream->has_alpha ? " (with alpha)" : "",
                    kFormatType[bitstream->format]);
            fprintf(stderr, "Nothing written; "
                            "use -o flag to save the result as e.g. PNG.\n");
        }
    }
    Exit:
    WebPFreeDecBuffer(output_buffer);
    WebPFree((void *) external_buffer);
    WebPFree((void *) data);
    FREE_WARGV_AND_RETURN(ok ? 0 : -1);
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_example_gifencoder_MainActivity_stringFromJNI(
        JNIEnv *env,
        jobject /* this */) {
    std::string info = "FFmpeg的版本号是: ";
    info.append(av_version_info());
//    decodeWebp("/sdcard/test.webp");

    return env->NewStringUTF(info.c_str());
}


extern "C"
JNIEXPORT jstring JNICALL
Java_com_example_gifencoder_MainActivity_decodeWebp(JNIEnv *env, jobject thiz, jstring path) {
    const char *path_ = env->GetStringUTFChars(path, 0);
//    decodeWebp((char *) path_);
    env->ReleaseStringUTFChars(path, path_);
    const int version = WebPGetDecoderVersion();

    std::string info = std::to_string((version >> 16) & 0xff);
    info.append(". ");
    info.append(std::to_string((version >> 8) & 0xff));
    info.append(". ");
    info.append(std::to_string(version & 0xff));
    return env->NewStringUTF(info.c_str());
}
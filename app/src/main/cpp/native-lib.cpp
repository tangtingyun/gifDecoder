#include <jni.h>
#include <string>

extern "C" {
#include <libavutil/avutil.h>
#include <webp/decode.h>
}


void decodeWebp(char *path) {
    const uint8_t *data;
    size_t data_size = 1;
    int width, height;
    WebPGetInfo(data, data_size, &width, &height);
    FILE *file =fopen(path, "wb");
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_example_gifencoder_MainActivity_stringFromJNI(
        JNIEnv *env,
        jobject /* this */) {
    std::string info = "FFmpeg的版本号是: ";
    info.append(av_version_info());
    decodeWebp("/sdcard/test.webp");

    return env->NewStringUTF(info.c_str());
}


#ifndef FTPSEND_MY_LOG_H
#define FTPSEND_MY_LOG_H

#include<android/log.h>
#define LOG_TAG "ftpUpload"
#define LOGI(fmt, args...) __android_log_print(ANDROID_LOG_INFO,  LOG_TAG, fmt, ##args)
#define LOGD(fmt, args...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, fmt, ##args)
#define LOGE(fmt, args...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, fmt, ##args)
#endif

/*
__android_log_print(ANDROID_LOG_XXX,LOG_TAG,content)
第一个参数是LOG级别；

*/
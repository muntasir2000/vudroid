/*
 * DjvuDroidBridge.cpp
 *
 *  Created on: 17.01.2010
 *      Author: Cool
 */

#include <jni.h>
#include <stdlib.h>
#include <DjvuDroidTrace.h>
#include <ddjvuapi.h>

#define HANDLE_TO_DOC(handle) (ddjvu_document_t*)handle
#define HANDLE(ptr) (jlong)ptr

extern "C" jlong
Java_org_vudroid_djvudroid_codec_DjvuContext_create(JNIEnv *env,
                                    jclass cls)
{
	ddjvu_context_t* context = ddjvu_context_create(DJVU_DROID);
	DEBUG_PRINT("Creating context: %x", context);
	return (jlong) context;
}

extern "C" void
Java_org_vudroid_djvudroid_codec_DjvuContext_free(JNIEnv *env,
                                    jclass cls,
                                    jlong contextHandle)
{
	ddjvu_context_release((ddjvu_context_t *)contextHandle);
}

extern "C" jlong
Java_org_vudroid_djvudroid_codec_DjvuDocument_open(JNIEnv *env,
                                    jclass cls,
                                    jlong contextHandle,
                                    jstring url)
{
    const char* urlString = env->GetStringUTFChars(url, NULL);
	DEBUG_PRINT("Opening document: %s", urlString);
    jlong docHandle = (jlong)(ddjvu_document_create((ddjvu_context_t*)(contextHandle), urlString, TRUE));
	env->ReleaseStringUTFChars(url, urlString);
    return docHandle;
}

void CallNewStreamCallback(JNIEnv* env, jobject thiz, const ddjvu_message_t* msg)
{
	DEBUG_WRITE("Calling handleNewStream callback");
	jclass cls = env->GetObjectClass(thiz);
	if (!cls)
		return;
    jmethodID handleNewStreamId = env->GetMethodID(cls, "handleNewStream", "(Ljava/lang/String;IJ)V");
    if (!handleNewStreamId)
    	return;
    env->CallVoidMethod(thiz, handleNewStreamId, env->NewStringUTF(msg->m_newstream.url), msg->m_newstream.streamid, (jlong)msg->m_any.document);
}

void ThrowDjvuError(JNIEnv* env, const ddjvu_message_t* msg)
{
    jclass exceptionClass = env->FindClass("java/lang/RuntimeException");
    if (!exceptionClass)
    	return;
    DEBUG_PRINT("Error: %s, %s:%s-%s", msg->m_error.message, msg->m_error.filename, msg->m_error.lineno, msg->m_error.function);
    env->ThrowNew(exceptionClass, msg->m_error.message);
}

extern "C" void
Java_org_vudroid_djvudroid_codec_DjvuContext_handleMessage(JNIEnv *env,
                                    jobject thiz,
                                    jlong contextHandle)
{
	const ddjvu_message_t *msg;
	ddjvu_context_t* ctx = (ddjvu_context_t*)(contextHandle);
	DEBUG_PRINT("handleMessage for ctx: %x",ctx);
	if(msg = ddjvu_message_wait(ctx))
    {
        switch (msg->m_any.tag)
        {
            case DDJVU_ERROR:
            	ThrowDjvuError(env, msg);
                break;
            case DDJVU_INFO:
                break;
            case DDJVU_NEWSTREAM:
            	CallNewStreamCallback(env, thiz, msg);
                break;
            default:
                break;
        }
        ddjvu_message_pop(ctx);
	}
}

extern "C" void
Java_org_vudroid_djvudroid_codec_DjvuContext_streamWrite(JNIEnv *env,
                                    jclass cls,
                                    jlong docHandle,
                                    jint streamId,
                                    jobject buffer,
                                    jint dataLen)
{
	ddjvu_stream_write((ddjvu_document_t*)docHandle, streamId, (const char *)env->GetDirectBufferAddress(buffer), dataLen);
}

extern "C" void
Java_org_vudroid_djvudroid_codec_DjvuContext_streamClose(JNIEnv *env,
                                    jclass cls,
                                    jlong docHandle,
                                    jint streamId,
                                    jboolean stop)
{
	ddjvu_stream_close((ddjvu_document_t*)docHandle, streamId, stop);
}

extern "C" jlong
Java_org_vudroid_djvudroid_codec_DjvuDocument_getPage(JNIEnv *env,
                                    jclass cls,
                                    jlong docHandle,
                                    jint pageNumber)
{
	DEBUG_PRINT("getPage num: %d", pageNumber);
	return (jlong)ddjvu_page_create_by_pageno((ddjvu_document_t*)docHandle, pageNumber);
}

extern "C" void
Java_org_vudroid_djvudroid_codec_DjvuDocument_free(JNIEnv *env,
                                    jclass cls,
                                    jlong docHandle)
{
	ddjvu_document_release((ddjvu_document_t*)docHandle);
}

extern "C" jint
Java_org_vudroid_djvudroid_codec_DjvuDocument_getPageCount(JNIEnv *env,
                                    jclass cls,
                                    jlong docHandle)
{
	return ddjvu_document_get_pagenum(HANDLE_TO_DOC(docHandle));
}

extern "C" jboolean
Java_org_vudroid_djvudroid_codec_DjvuPage_isDecodingDone(JNIEnv *env,
                                    jclass cls,
                                    jlong pageHandle)
{
	return ddjvu_page_decoding_done((ddjvu_page_t*)pageHandle);
}

extern "C" jint
Java_org_vudroid_djvudroid_codec_DjvuPage_getWidth(JNIEnv *env,
                                    jclass cls,
                                    jlong pageHangle)
{
	return ddjvu_page_get_width((ddjvu_page_t*)pageHangle);
}

extern "C" jint
Java_org_vudroid_djvudroid_codec_DjvuPage_getHeight(JNIEnv *env,
                                    jclass cls,
                                    jlong pageHangle)
{
	return ddjvu_page_get_height((ddjvu_page_t*)pageHangle);
}

extern "C" jboolean
Java_org_vudroid_djvudroid_codec_DjvuPage_renderPage(JNIEnv *env,
                                    jclass cls,
                                    jlong pageHangle,
                                    jint targetWidth,
                                    jint targetHeight,
                                    jobject buffer)
{
	DEBUG_WRITE("Rendering page");
	ddjvu_page_t* page = (ddjvu_page_t*)((pageHangle));
    ddjvu_rect_t pageRect;
    pageRect.x = 0;
    pageRect.y = 0;
    pageRect.w = targetWidth;
    pageRect.h = targetHeight;
    ddjvu_rect_t targetRect = pageRect;
    unsigned int masks[] = {0xF800, 0x07E0, 0x001F};
    ddjvu_format_t* pixelFormat = ddjvu_format_create(DDJVU_FORMAT_RGBMASK16, 3, masks);
    ddjvu_format_set_row_order(pixelFormat, TRUE);
    jboolean result = ddjvu_page_render(page, DDJVU_RENDER_COLOR, &pageRect, &targetRect, pixelFormat, targetWidth * 2, (char*)env->GetDirectBufferAddress(buffer));
    ddjvu_format_release(pixelFormat);
    return result;
}

extern "C" void
Java_org_vudroid_djvudroid_codec_DjvuPage_free(JNIEnv *env,
                                    jclass cls,
                                    jlong pageHangle)
{
	ddjvu_page_release((ddjvu_page_t*)pageHangle);
}

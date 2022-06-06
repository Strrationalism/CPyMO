#include <jni.h>
#include <SDL.h>
#include <android/log.h>

static jclass mVisualHelperClass;
static jmethodID midTextToSpeech;
static jmethodID midPlaySound;

JNIEXPORT jboolean JNICALL
Java_xyz_xydm_cpymo_Config_nativeGetNonVisuallyImpairedHelp(JNIEnv *env, jclass clazz)
{
#ifdef NON_VISUALLY_IMPAIRED_HELP
    return JNI_TRUE;
#else
    return JNI_FALSE;
#endif
}

JNIEXPORT void JNICALL
Java_xyz_xydm_cpymo_VisualHelper_nativeSetupJNI(JNIEnv *env, jclass clazz)
{
    mVisualHelperClass = (jclass)((*env)->NewGlobalRef(env, clazz));
    midTextToSpeech = (*env)->GetStaticMethodID(env, clazz, "textToSpeech", "(Ljava/lang/String;)Z");
    midPlaySound = (*env)->GetStaticMethodID(env, clazz, "playSound", "(I)V");
}

/* Show toast notification */
int Android_JNI_TextToSpeech(const char* text)
{
    JNIEnv *env = SDL_AndroidGetJNIEnv();
    jstring jtext = (*env)->NewStringUTF(env, text);
    jboolean result = (*env)->CallStaticBooleanMethod(env, mVisualHelperClass, midTextToSpeech, jtext);
    (*env)->DeleteLocalRef(env, jtext);
    return (int)result;
}

void Android_JNI_PlaySound(int sound_type)
{
    JNIEnv *env = SDL_AndroidGetJNIEnv();
    (*env)->CallStaticVoidMethod(env, mVisualHelperClass, midPlaySound, sound_type);
}

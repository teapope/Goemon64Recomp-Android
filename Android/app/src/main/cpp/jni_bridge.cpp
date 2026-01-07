#include <jni.h>
#include <android/log.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <GLES3/gl3.h>
#include <EGL/egl.h>
#include <string>
#include <memory>

#define LOG_TAG "Goemon64JNI"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

// Forward declarations of engine functions
// These will link to the actual Goemon64Recomp engine code
namespace Goemon64 {
    bool InitializeEngine(const char* dataPath);
    bool LoadROM(const char* romPath);
    bool StartGame();
    void StopGame();
    void PauseGame();
    void ResumeGame();
    void ProcessFrame();
    void HandleInput(int action, int keyCode, float x, float y);
    void OnSurfaceChanged(int width, int height);
}

// Global state
struct EngineState {
    ANativeWindow* window = nullptr;
    EGLDisplay display = EGL_NO_DISPLAY;
    EGLSurface surface = EGL_NO_SURFACE;
    EGLContext context = EGL_NO_CONTEXT;
    int width = 0;
    int height = 0;
    bool initialized = false;
    std::string dataPath;
    std::string romPath;
};

static EngineState g_engineState;

// EGL initialization
static bool InitEGL() {
    LOGI("Initializing EGL");
    
    // Get display
    g_engineState.display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (g_engineState.display == EGL_NO_DISPLAY) {
        LOGE("Failed to get EGL display");
        return false;
    }
    
    // Initialize EGL
    EGLint major, minor;
    if (!eglInitialize(g_engineState.display, &major, &minor)) {
        LOGE("Failed to initialize EGL");
        return false;
    }
    
    LOGI("EGL version: %d.%d", major, minor);
    
    // Choose config
    const EGLint configAttribs[] = {
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_BLUE_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_RED_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_DEPTH_SIZE, 24,
        EGL_STENCIL_SIZE, 8,
        EGL_NONE
    };
    
    EGLConfig config;
    EGLint numConfigs;
    if (!eglChooseConfig(g_engineState.display, configAttribs, &config, 1, &numConfigs)) {
        LOGE("Failed to choose EGL config");
        return false;
    }
    
    // Set format
    EGLint format;
    eglGetConfigAttrib(g_engineState.display, config, EGL_NATIVE_VISUAL_ID, &format);
    ANativeWindow_setBuffersGeometry(g_engineState.window, 0, 0, format);
    
    // Create surface
    g_engineState.surface = eglCreateWindowSurface(g_engineState.display, config, 
                                                    g_engineState.window, nullptr);
    if (g_engineState.surface == EGL_NO_SURFACE) {
        LOGE("Failed to create EGL surface");
        return false;
    }
    
    // Create context
    const EGLint contextAttribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 3,
        EGL_NONE
    };
    
    g_engineState.context = eglCreateContext(g_engineState.display, config, 
                                            EGL_NO_CONTEXT, contextAttribs);
    if (g_engineState.context == EGL_NO_CONTEXT) {
        LOGE("Failed to create EGL context");
        return false;
    }
    
    // Make current
    if (!eglMakeCurrent(g_engineState.display, g_engineState.surface, 
                       g_engineState.surface, g_engineState.context)) {
        LOGE("Failed to make EGL context current");
        return false;
    }
    
    // Enable vsync
    eglSwapInterval(g_engineState.display, 1);
    
    LOGI("EGL initialized successfully");
    return true;
}

static void ShutdownEGL() {
    if (g_engineState.display != EGL_NO_DISPLAY) {
        eglMakeCurrent(g_engineState.display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        
        if (g_engineState.context != EGL_NO_CONTEXT) {
            eglDestroyContext(g_engineState.display, g_engineState.context);
            g_engineState.context = EGL_NO_CONTEXT;
        }
        
        if (g_engineState.surface != EGL_NO_SURFACE) {
            eglDestroySurface(g_engineState.display, g_engineState.surface);
            g_engineState.surface = EGL_NO_SURFACE;
        }
        
        eglTerminate(g_engineState.display);
        g_engineState.display = EGL_NO_DISPLAY;
    }
}

// JNI functions
extern "C" {

JNIEXPORT jboolean JNICALL
Java_com_goemon64recomp_android_MainActivity_initNativeEngine(
        JNIEnv* env, jobject /* this */, jobject surface, jstring jDataPath) {
    
    LOGI("Initializing native engine");
    
    // Get native window from surface
    g_engineState.window = ANativeWindow_fromSurface(env, surface);
    if (!g_engineState.window) {
        LOGE("Failed to get native window");
        return JNI_FALSE;
    }
    
    // Get data path
    const char* dataPathStr = env->GetStringUTFChars(jDataPath, nullptr);
    g_engineState.dataPath = dataPathStr;
    env->ReleaseStringUTFChars(jDataPath, dataPathStr);
    
    LOGI("Data path: %s", g_engineState.dataPath.c_str());
    
    // Initialize EGL
    if (!InitEGL()) {
        LOGE("Failed to initialize EGL");
        return JNI_FALSE;
    }
    
    // Initialize game engine
    if (!Goemon64::InitializeEngine(g_engineState.dataPath.c_str())) {
        LOGE("Failed to initialize game engine");
        ShutdownEGL();
        return JNI_FALSE;
    }
    
    g_engineState.initialized = true;
    LOGI("Native engine initialized successfully");
    
    return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
Java_com_goemon64recomp_android_MainActivity_loadROM(
        JNIEnv* env, jobject /* this */, jstring jRomPath) {
    
    if (!g_engineState.initialized) {
        LOGE("Engine not initialized");
        return JNI_FALSE;
    }
    
    const char* romPathStr = env->GetStringUTFChars(jRomPath, nullptr);
    g_engineState.romPath = romPathStr;
    env->ReleaseStringUTFChars(jRomPath, romPathStr);
    
    LOGI("Loading ROM: %s", g_engineState.romPath.c_str());
    
    bool result = Goemon64::LoadROM(g_engineState.romPath.c_str());
    
    if (result) {
        LOGI("ROM loaded successfully");
    } else {
        LOGE("Failed to load ROM");
    }
    
    return result ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_goemon64recomp_android_MainActivity_startGame(
        JNIEnv* /* env */, jobject /* this */) {
    
    if (!g_engineState.initialized) {
        LOGE("Engine not initialized");
        return JNI_FALSE;
    }
    
    LOGI("Starting game");
    
    bool result = Goemon64::StartGame();
    
    if (result) {
        LOGI("Game started successfully");
    } else {
        LOGE("Failed to start game");
    }
    
    return result ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT void JNICALL
Java_com_goemon64recomp_android_MainActivity_stopGame(
        JNIEnv* /* env */, jobject /* this */) {
    
    LOGI("Stopping game");
    Goemon64::StopGame();
    
    ShutdownEGL();
    
    if (g_engineState.window) {
        ANativeWindow_release(g_engineState.window);
        g_engineState.window = nullptr;
    }
    
    g_engineState.initialized = false;
}

JNIEXPORT void JNICALL
Java_com_goemon64recomp_android_MainActivity_pauseGame(
        JNIEnv* /* env */, jobject /* this */) {
    
    LOGI("Pausing game");
    Goemon64::PauseGame();
}

JNIEXPORT void JNICALL
Java_com_goemon64recomp_android_MainActivity_resumeGame(
        JNIEnv* /* env */, jobject /* this */) {
    
    LOGI("Resuming game");
    Goemon64::ResumeGame();
}

JNIEXPORT void JNICALL
Java_com_goemon64recomp_android_MainActivity_onSurfaceChanged(
        JNIEnv* /* env */, jobject /* this */, jint width, jint height) {
    
    LOGI("Surface changed: %dx%d", width, height);
    
    g_engineState.width = width;
    g_engineState.height = height;
    
    if (g_engineState.initialized) {
        Goemon64::OnSurfaceChanged(width, height);
    }
}

JNIEXPORT jboolean JNICALL
Java_com_goemon64recomp_android_MainActivity_processInput(
        JNIEnv* /* env */, jobject /* this */, 
        jint action, jint keyCode, jfloat x, jfloat y) {
    
    if (!g_engineState.initialized) {
        return JNI_FALSE;
    }
    
    Goemon64::HandleInput(action, keyCode, x, y);
    return JNI_TRUE;
}

JNIEXPORT void JNICALL
Java_com_goemon64recomp_android_MainActivity_renderFrame(
        JNIEnv* /* env */, jobject /* this */) {
    
    if (!g_engineState.initialized) {
        return;
    }
    
    // Process frame
    Goemon64::ProcessFrame();
    
    // Swap buffers
    if (g_engineState.display != EGL_NO_DISPLAY && 
        g_engineState.surface != EGL_NO_SURFACE) {
        eglSwapBuffers(g_engineState.display, g_engineState.surface);
    }
}

} // extern "C"

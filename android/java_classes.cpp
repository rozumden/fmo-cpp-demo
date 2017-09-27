#include "java_classes.hpp"

// Object

Object::Object(JNIEnv* env, jobject obj, bool disposeOfObj) :
        mEnv(env),
        mObj(obj),
        mObjDelete(disposeOfObj) {}

Object::~Object() {
    clear();
}

Object::Object(Object&& rhs) : mEnv(rhs.mEnv), mObj(rhs.mObj), mObjDelete(rhs.mObjDelete) {
    rhs.mObj = nullptr;
}

Object& Object::operator=(Object&& rhs) {
    clear();
    mEnv = rhs.mEnv;
    mObj = rhs.mObj;
    mObjDelete = rhs.mObjDelete;
    rhs.mObj = nullptr;
    return *this;
}

void Object::clear() {
    if (mObjDelete && mObj != nullptr) {
        mEnv->DeleteLocalRef(mObj);
        mObj = nullptr;
    }
}

// Callback

namespace {
    struct CallbackBindings {
        jclass class_;
        jmethodID log;
        jmethodID onObjectsDetected;

        CallbackBindings(JNIEnv* env) {
            jclass local = env->FindClass("cz/fmo/Lib$Callback");
            class_ = (jclass) env->NewGlobalRef(local);
            env->DeleteLocalRef(local);

            log = env->GetMethodID(class_, "log", "(Ljava/lang/String;)V");
            onObjectsDetected = env->GetMethodID(class_, "onObjectsDetected",
                                                 "([Lcz/fmo/Lib$Detection;)V");
        }
    };

    std::unique_ptr<CallbackBindings> bCallback;
}

void Callback::log(const char* cStr) {
    jstring string = mEnv->NewStringUTF(cStr);
    mEnv->CallVoidMethod(mObj, bCallback->log, string);
    mEnv->DeleteLocalRef(string);
}

void Callback::onObjectsDetected(const DetectionArray& detections) {
    mEnv->CallVoidMethod(mObj, bCallback->onObjectsDetected, detections.mObj);
}

// Detection

namespace {
    struct DetectionBindings {
        jclass class_;
        jmethodID init_;
        jfieldID id;
        jfieldID predecessorId;
        jfieldID centerX;
        jfieldID centerY;
        jfieldID directionX;
        jfieldID directionY;
        jfieldID length;
        jfieldID radius;
        jfieldID velocity;
        jfieldID predecessor;

        DetectionBindings(JNIEnv* env) {
            jclass local = env->FindClass("cz/fmo/Lib$Detection");
            class_ = (jclass) env->NewGlobalRef(local);
            env->DeleteLocalRef(local);

            init_ = env->GetMethodID(class_, "<init>", "()V");

            id = env->GetFieldID(class_, "id", "I");
            predecessorId = env->GetFieldID(class_, "predecessorId", "I");
            centerX = env->GetFieldID(class_, "centerX", "I");
            centerY = env->GetFieldID(class_, "centerY", "I");
            directionX = env->GetFieldID(class_, "directionX", "F");
            directionY = env->GetFieldID(class_, "directionY", "F");
            length = env->GetFieldID(class_, "length", "F");
            radius = env->GetFieldID(class_, "radius", "F");
            velocity = env->GetFieldID(class_, "velocity", "F");
            predecessor = env->GetFieldID(class_, "predecessor", "Lcz/fmo/Lib$Detection;");
        }
    };

    std::unique_ptr<DetectionBindings> bDetection;
}

Detection::Detection(JNIEnv* env, const fmo::Algorithm::Detection& det) :
        Object(env,
               env->NewObject(bDetection->class_, bDetection->init_),
               true) {
    mEnv->SetIntField(mObj, bDetection->id, det.object.id);
    mEnv->SetIntField(mObj, bDetection->predecessorId, det.predecessor.id);
    mEnv->SetIntField(mObj, bDetection->centerX, det.object.center.x);
    mEnv->SetIntField(mObj, bDetection->centerY, det.object.center.y);
    mEnv->SetFloatField(mObj, bDetection->directionX, det.object.direction[0]);
    mEnv->SetFloatField(mObj, bDetection->directionY, det.object.direction[1]);
    mEnv->SetFloatField(mObj, bDetection->length, det.object.length);
    mEnv->SetFloatField(mObj, bDetection->radius, det.object.radius);
    mEnv->SetFloatField(mObj, bDetection->velocity, det.object.velocity);
}

fmo::Pos Detection::getCenter() const {
    jint x = mEnv->GetIntField(mObj, bDetection->centerX);
    jint y = mEnv->GetIntField(mObj, bDetection->centerY);
    return {x, y};
}

float Detection::getRadius() const {
    return mEnv->GetFloatField(mObj, bDetection->radius);
}

Detection Detection::getPredecessor() const {
    jobject ref = mEnv->GetObjectField(mObj, bDetection->predecessor);
    return {mEnv, ref, true};
}

// DetectionArray

DetectionArray::DetectionArray(JNIEnv* env, jsize length) :
        Object(env, env->NewObjectArray(length, bDetection->class_, nullptr), true) {}

void DetectionArray::set(int i, const Detection& detection) {
    mEnv->SetObjectArrayElement((jobjectArray) mObj, i, detection.mObj);
}

// TriangleStripBuffers

namespace {
    struct TriangleStripBuffersBindings {
        jclass class_;
        jfieldID posMat;
        jfieldID pos;
        jfieldID color;
        jfieldID numVertices;

        TriangleStripBuffersBindings(JNIEnv* env) {
            jclass local = env->FindClass("cz/fmo/graphics/TriangleStripRenderer$Buffers");
            class_ = (jclass) env->NewGlobalRef(local);
            env->DeleteLocalRef(local);

            posMat = env->GetFieldID(class_, "posMat", "[F");
            pos = env->GetFieldID(class_, "pos", "Ljava/nio/FloatBuffer;");
            color = env->GetFieldID(class_, "color", "Ljava/nio/FloatBuffer;");
            numVertices = env->GetFieldID(class_, "numVertices", "I");
        }
    };

    std::unique_ptr<TriangleStripBuffersBindings> bTriangleStripBuffers;
}

TriangleStripBuffers::TriangleStripBuffers(JNIEnv* env, jobject obj, bool disposeOfObj) :
        Object(env, obj, disposeOfObj) {
    // copy number of vertices
    mNumVertices = mEnv->GetIntField(mObj, bTriangleStripBuffers->numVertices);

    // obtain direct access to data
    int maxNumPos = 0;
    {
        jobject buf = mEnv->GetObjectField(mObj, bTriangleStripBuffers->pos);
        maxNumPos = int(mEnv->GetDirectBufferCapacity(buf) / 2);
        mPos = (Pos*) mEnv->GetDirectBufferAddress(buf);
        mEnv->DeleteLocalRef(buf);
    }
    int maxNumColors = 0;
    {
        jobject buf = mEnv->GetObjectField(mObj, bTriangleStripBuffers->color);
        maxNumColors = int(mEnv->GetDirectBufferCapacity(buf) / 4);
        mColor = (Color*) mEnv->GetDirectBufferAddress(buf);
        mEnv->DeleteLocalRef(buf);
    }

    // store maximum number of vertices to store
    mMaxVertices = (maxNumPos < maxNumColors) ? maxNumPos : maxNumColors;
}

TriangleStripBuffers::~TriangleStripBuffers() {
    // write the number of vertices back
    mEnv->SetIntField(mObj, bTriangleStripBuffers->numVertices, mNumVertices);
}

void TriangleStripBuffers::addVertex(const Pos& pos, const Color& color) {
    if (mNumVertices == mMaxVertices) return;
    mPos[mNumVertices] = pos;
    mColor[mNumVertices] = color;
    mNumVertices++;
}

// FontBuffers

namespace {
    struct FontBuffersBindings {
        jclass class_;
        jfieldID posMat;
        jfieldID pos;
        jfieldID uv;
        jfieldID color;
        jfieldID idx;
        jfieldID numCharacters;

        FontBuffersBindings(JNIEnv* env) {
            jclass local = env->FindClass("cz/fmo/graphics/FontRenderer$Buffers");
            class_ = (jclass) env->NewGlobalRef(local);
            env->DeleteLocalRef(local);

            posMat = env->GetFieldID(class_, "posMat", "[F");
            pos = env->GetFieldID(class_, "pos", "Ljava/nio/FloatBuffer;");
            uv = env->GetFieldID(class_, "uv", "Ljava/nio/FloatBuffer;");
            color = env->GetFieldID(class_, "color", "Ljava/nio/FloatBuffer;");
            idx = env->GetFieldID(class_, "idx", "Ljava/nio/IntBuffer;");
            numCharacters = env->GetFieldID(class_, "numCharacters", "I");
        }
    };

    std::unique_ptr<FontBuffersBindings> bFontBuffers;
}

FontBuffers::FontBuffers(JNIEnv* env, jobject obj, bool disposeOfObj) :
        Object(env, obj, disposeOfObj) {
    // copy number of characters
    mNumCharacters = mEnv->GetIntField(mObj, bFontBuffers->numCharacters);
    mNumVertices = 4 * mNumCharacters;

    // obtain direct access to data
    {
        jobject buf = mEnv->GetObjectField(mObj, bFontBuffers->pos);
        mMaxCharacters = int(mEnv->GetDirectBufferCapacity(buf)) / 8;
        mMaxVertices = mMaxCharacters * 4;
        mPos = (Pos*) mEnv->GetDirectBufferAddress(buf);
        mEnv->DeleteLocalRef(buf);
    }
    {
        jobject buf = mEnv->GetObjectField(mObj, bFontBuffers->uv);
        mUV = (UV*) mEnv->GetDirectBufferAddress(buf);
        mEnv->DeleteLocalRef(buf);
    }
    {
        jobject buf = mEnv->GetObjectField(mObj, bFontBuffers->color);
        mColor = (Color*) mEnv->GetDirectBufferAddress(buf);
        mEnv->DeleteLocalRef(buf);
    }
    {
        jobject buf = mEnv->GetObjectField(mObj, bFontBuffers->idx);
        mIdx = (Idx*) mEnv->GetDirectBufferAddress(buf);
        mEnv->DeleteLocalRef(buf);
    }
}

FontBuffers::~FontBuffers() {
    // write the number of characters back
    mEnv->SetIntField(mObj, bFontBuffers->numCharacters, mNumCharacters);
}

void FontBuffers::addRectangle(const Pos& pos1, const Pos& pos2, const UV& uv1, const UV& uv2,
                               const Color& color) {
    addVertex(pos1, uv1, color);
    Pos pos;
    UV uv;
    pos.x = pos1.x;
    pos.y = pos2.y;
    uv.u = uv1.u;
    uv.v = uv2.v;
    addVertex(pos, uv, color);
    pos.x = pos2.x;
    pos.y = pos1.y;
    uv.u = uv2.u;
    uv.v = uv1.v;
    addVertex(pos, uv, color);
    addVertex(pos2, uv2, color);
    addCharacter();
}

void FontBuffers::addVertex(const Pos& pos, const UV& uv, const Color& color) {
    if (mNumVertices == mMaxVertices) return;
    mPos[mNumVertices] = pos;
    mUV[mNumVertices] = uv;
    mColor[mNumVertices] = color;
    mNumVertices++;
}

void FontBuffers::addCharacter() {
    if (mNumCharacters == mMaxCharacters) return;
    auto& i = mIdx[mNumCharacters].i;
    i[0] = mNumVertices - 4;
    i[1] = mNumVertices - 4;
    i[2] = mNumVertices - 3;
    i[3] = mNumVertices - 2;
    i[4] = mNumVertices - 1;
    i[5] = mNumVertices - 1;
    mNumCharacters++;
}

// initJavaClasses

void initJavaClasses(JNIEnv* env) {
    if (!bDetection) {
        bDetection = std::make_unique<DetectionBindings>(env);
    }
    if (!bCallback) {
        bCallback = std::make_unique<CallbackBindings>(env);
    }
    if (!bTriangleStripBuffers) {
        bTriangleStripBuffers = std::make_unique<TriangleStripBuffersBindings>(env);
    }
    if (!bFontBuffers) {
        bFontBuffers = std::make_unique<FontBuffersBindings>(env);
    }
}

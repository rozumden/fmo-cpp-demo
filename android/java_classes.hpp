#ifndef FMO_ANDROID_JAVA_CLASSES_HPP
#define FMO_ANDROID_JAVA_CLASSES_HPP

#include <algorithm>
#include <cstdint>
#include <fmo/assert.hpp>
#include <fmo/algebra.hpp>
#include <fmo/algorithm.hpp>
#include <jni.h>

// forward declarations
struct Detection;
struct DetectionArray;

/**
 * Models java.lang.Object
 */
class Object {
public:
    virtual ~Object();

    Object(Object&&);

    Object& operator=(Object&&);

    Object(JNIEnv* env, jobject obj, bool disposeOfObj);

    bool isNull() const { return mObj == nullptr; }

    void clear();

protected:
    JNIEnv* mEnv;
    jobject mObj;
    bool mObjDelete;
};

/**
 * Models cz.fmo.Lib$FrameCallback
 */
struct Callback : public Object {
    using Object::Object;

    Callback(Callback&& rhs) : Object(std::move(rhs)) {}

    virtual ~Callback() override = default;

    void log(const char* cStr);

    void onObjectsDetected(const DetectionArray& detections);
};

/**
 * Models cz.fmo.Lib$Detection
 */
struct Detection : public Object {
    using Object::Object;

    Detection(Detection&& rhs) : Object(std::move(rhs)) {}

    Detection& operator=(Detection&& rhs) {
        Object::operator=(std::move(rhs));
        return *this;
    }

    virtual ~Detection() override = default;

    Detection(JNIEnv* env, const fmo::Algorithm::Detection& det);

    fmo::Pos getCenter() const;

    float getRadius() const;

    Detection getPredecessor() const;

    friend struct DetectionArray;
};

/**
 * Models cz.fmo.Lib$Detection[]
 */
struct DetectionArray : public Object {
    using Object::Object;

    virtual ~DetectionArray() override = default;

    DetectionArray(JNIEnv* env, jsize length);

    void set(int i, const Detection& detection);

    friend void Callback::onObjectsDetected(const DetectionArray&);
};

/**
 * Models cz.fmo.graphics.TriangleStripRenderer$Buffers
 */
struct TriangleStripBuffers : public Object {
    virtual ~TriangleStripBuffers() override;

    TriangleStripBuffers(JNIEnv* env, jobject obj, bool disposeOfObj);

    struct Pos {
        float x, y;
    };

    struct Color {
        float r, g, b, a;
    };

    void addVertex(const Pos& pos, const Color& color);

private:
    Pos* mPos;
    Color* mColor;
    int mMaxVertices;
    int mNumVertices;
};

/**
 * Models cz.fmo.graphics.FontRenderer$Buffers
 */
/**
 * Models cz.fmo.graphics.TriangleStripRenderer$Buffers
 */
struct FontBuffers : public Object {
    virtual ~FontBuffers() override;

    FontBuffers(JNIEnv* env, jobject obj, bool disposeOfObj);

    struct Pos {
        float x, y;
    };

    struct UV {
        float u, v;
    };

    struct Color {
        float r, g, b, a;
    };

    struct Idx {
        int i[6];
    };

    void addRectangle(const Pos& pos1, const Pos& pos2, const UV& uv1, const UV& uv2,
                      const Color& color);

    void addVertex(const Pos& pos, const UV& uv, const Color& color);

    void addCharacter();

private:
    Pos* mPos;
    UV* mUV;
    Color* mColor;
    Idx* mIdx;
    int mMaxCharacters;
    int mMaxVertices;
    int mNumCharacters;
    int mNumVertices;
};

/**
 * Wraps other objects, extending their lifetime past the duration of the native function.
 */
template<typename T>
struct Reference {
    Reference(const Reference&) = delete;

    Reference& operator=(const Reference&) = delete;

    Reference() noexcept : mObj(nullptr) {}

    Reference(Reference&& rhs) noexcept : Reference() { std::swap(mObj, rhs.mObj); }

    Reference& operator=(Reference&& rhs) noexcept {
        std::swap(mObj, rhs.mObj);
        return *this;
    }

    Reference(JNIEnv* env, jobject obj) : mObj(env->NewGlobalRef(obj)) {}

    ~Reference() {
        FMO_ASSERT(mObj == nullptr, "release() must be called before a Reference is destroyed");
    }

    void release(JNIEnv* env) {
        env->DeleteGlobalRef(mObj);
        mObj = nullptr;
    }

    T get(JNIEnv* env) { return {env, mObj, false}; }

private:
    jobject mObj;
};

/**
 * Call before using above objects. Must be called in a Java thread.
 */
void initJavaClasses(JNIEnv* env);

#endif // FMO_ANDROID_JAVA_CLASSES_HPP

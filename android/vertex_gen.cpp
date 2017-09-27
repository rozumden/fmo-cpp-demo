#include "java_interface.hpp"
#include "java_classes.hpp"

namespace {
    TriangleStripBuffers::Pos shiftPoint(fmo::Pos center, float dist, fmo::NormVector dir) {
        TriangleStripBuffers::Pos result;
        result.x = float(center.x) + (dist * dir.x);
        result.y = float(center.y) + (dist * dir.y);
        return result;
    }
}

JNIEXPORT void JNICALL Java_cz_fmo_Lib_generateCurve
        (JNIEnv* env, jclass, jobject detObj, jfloatArray rgba, jobject bObj) {
    initJavaClasses(env);
    static constexpr float DECAY_BASE = 0.33f;
    static constexpr float DECAY_RATE = 0.50f;

    // get data
    TriangleStripBuffers b(env, bObj, false);
    TriangleStripBuffers::Color color;
    env->GetFloatArrayRegion(rgba, 0, 4, (float*) &color);
    float decay = 1.f - DECAY_BASE;

    // first vertex
    Detection d(env, detObj, false);
    Detection dNext = d.getPredecessor();
    if (dNext.isNull()) return;
    fmo::Pos pos = d.getCenter();
    fmo::Pos posNext = dNext.getCenter();
    auto dir = fmo::NormVector(posNext - pos);
    auto norm = fmo::perpendicular(dir);
    auto radius = d.getRadius();
    auto vA = shiftPoint(pos, radius, norm);
    auto vB = shiftPoint(pos, -radius, norm);
    color.a = DECAY_BASE + decay;
    b.addVertex(vA, color);
    b.addVertex(vA, color);
    b.addVertex(vB, color);

    while (true) {
        d = std::move(dNext);
        dNext = d.getPredecessor();
        pos = posNext;

        if (dNext.isNull()) break;

        posNext = dNext.getCenter();
        dir = fmo::NormVector(posNext - pos);
        auto normPrev = norm;
        norm = fmo::perpendicular(dir);
        auto normAvg = fmo::average(norm, normPrev);
        radius = d.getRadius();
        vA = shiftPoint(pos, radius, normAvg);
        vB = shiftPoint(pos, -radius, normAvg);
        color.a = DECAY_BASE + (decay *= DECAY_RATE);
        b.addVertex(vA, color);
        b.addVertex(vB, color);
        decay *= DECAY_RATE;
    }

    // last vertex
    radius = d.getRadius();
    vA = shiftPoint(pos, radius, norm);
    vB = shiftPoint(pos, -radius, norm);
    color.a = DECAY_BASE + (decay *= DECAY_RATE);
    b.addVertex(vA, color);
    b.addVertex(vB, color);
    b.addVertex(vB, color);
    decay *= DECAY_RATE;
}

JNIEXPORT void JNICALL
Java_cz_fmo_Lib_generateString(JNIEnv* env, jclass, jstring str, jfloat x, jfloat y, jfloat h,
                               jfloatArray rgba, jobject bObj) {
    initJavaClasses(env);
    FontBuffers b(env, bObj, false);
    FontBuffers::Color color;
    env->GetFloatArrayRegion(rgba, 0, 4, (float*) &color);
    const jchar* cstr = env->GetStringChars(str, nullptr);
    jsize strLen = env->GetStringLength(str);
    static constexpr float charWidth = 0.639f;
    static constexpr float charStepX = 0.8f * charWidth;
    float w = charWidth * h;
    float ws = charStepX * h;
    FontBuffers::Pos pos1, pos2;
    pos1.y = y - 0.5f * h;
    pos2.y = y + 0.5f * h;
    for (jsize i = 0; i < strLen; i++) {
        pos1.x = x + (float(i) * ws);
        pos2.x = pos1.x + w;
        int ui = cstr[i] % 16;
        int vi = cstr[i] / 16;
        FontBuffers::UV uv1, uv2;
        uv1.u = float(ui) / 16.f;
        uv2.u = uv1.u + (1 / 16.f);
        uv1.v = float(vi) / 8.f;
        uv2.v = uv1.v + (1 / 8.f);
        b.addRectangle(pos1, pos2, uv1, uv2, color);
    }
    env->ReleaseStringChars(str, cstr);
}

JNIEXPORT void JNICALL Java_cz_fmo_Lib_generateRectangle
        (JNIEnv * env, jclass, jfloat x, jfloat y, jfloat w, jfloat h, jfloatArray rgba, jobject bObj) {
    initJavaClasses(env);
    FontBuffers b(env, bObj, false);
    FontBuffers::Pos pos1 = {x, y};
    FontBuffers::Pos pos2 = {x + w, y + h};
    FontBuffers::UV uv1 = {0.f, 0.f};
    FontBuffers::UV uv2 = {0.0625f, 0.125f};
    FontBuffers::Color color;
    env->GetFloatArrayRegion(rgba, 0, 4, (float*) &color);
    b.addRectangle(pos1, pos2, uv1, uv2, color);
}

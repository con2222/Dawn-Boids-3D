#include <C2Core/time_core.hpp>

#include <chrono>
#include <thread>
#include <limits>
#include <numeric>
#include <vector>
#include <algorithm>

namespace C2Core::Time {

struct Context {
    uint64_t lastTimeNs = 0;
    double deltaTime;
    float timeScale = 1.0f;
    double targetFPS;
    double fixedDeltaTime;
    double accumulator = 0;
    uint32_t bufferIndex = 0;
    double frameTimes[1000];
};

Context* create(double targetFps, double fixedUpdateRate) {
    Context* ctx = new Context();
    ctx->targetFPS = targetFps;
    ctx->fixedDeltaTime = 1.0 / fixedUpdateRate;
    return ctx;
}

void destroy(Context* ctx) {
    if (ctx) {
        delete ctx;
    }
}

void startFrame(Context* ctx) {
    uint64_t nowNs = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    if (ctx->lastTimeNs == 0) { ctx->lastTimeNs = nowNs; }
    ctx->deltaTime = (nowNs - ctx->lastTimeNs) / 1'000'000'000.0 * ctx->timeScale;
    ctx->accumulator += ctx->deltaTime;
    ctx->lastTimeNs = nowNs;
};

bool consumeFixedUpdate(Context* ctx) {
    if (ctx->accumulator >= ctx->fixedDeltaTime) {
        ctx->accumulator -= ctx->fixedDeltaTime;
        return true;
    } else {
        return false;
    }
}

void endFrame(Context* ctx, WaitMode mode) {
    uint64_t targetFrameTimeNs = 1'000'000'000.0 / ctx->targetFPS;
    uint64_t nowNs = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    uint64_t elapsedNs = nowNs - ctx->lastTimeNs;

    if (elapsedNs < targetFrameTimeNs) {
        switch(mode) {
            case WaitMode::Sleep:
                if (targetFrameTimeNs - elapsedNs > 0) {
                    std::this_thread::sleep_for(std::chrono::nanoseconds(targetFrameTimeNs - elapsedNs));
                }
                break;
            case WaitMode::Spin:
                while (elapsedNs < targetFrameTimeNs) {
                    nowNs = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
                    elapsedNs = nowNs - ctx->lastTimeNs;
                    std::this_thread::yield();
                }
                break;  
            case WaitMode::Hybrid: {
                uint64_t timeLeftNs = targetFrameTimeNs - elapsedNs;
                if (timeLeftNs > 2'000'000) {
                    std::this_thread::sleep_for(std::chrono::nanoseconds(timeLeftNs - 2'000'000));
                }
                while (elapsedNs < targetFrameTimeNs) {
                    nowNs = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
                    elapsedNs = nowNs - ctx->lastTimeNs;
                    std::this_thread::yield();
                }
                break;
            }
            default:
                break;
        }
    }

    nowNs = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    double elapsedMs = (nowNs - ctx->lastTimeNs) / 1'000'000.0;
    ctx->frameTimes[ctx->bufferIndex] = elapsedMs;
    ctx->bufferIndex++;
    if (ctx->bufferIndex == 1000) {
        ctx->bufferIndex = 0;
    }
}

Stats getStats(const Context* ctx) {
    std::vector<double> times(std::begin(ctx->frameTimes), std::end(ctx->frameTimes));
    std::sort(times.begin(), times.end(), [](double a, double b){ return a > b; });

    double frameSum = std::accumulate(times.begin(), times.end(), 0.0);
    double tenTimesSum = std::accumulate(times.begin(), times.begin() + 10, 0.0);
    double averageFrameTime = frameSum / 1000.0;
    double averageOfWorst10Frames = tenTimesSum / 10;

    Stats stats;
    stats.currentFps = 1000.0 / averageFrameTime;
    stats.averageFrameTime = averageFrameTime;
    stats.minFrameTime = times.back();
    stats.maxFrameTime = times[0];
    stats.low1Percent = 1000.0 / averageOfWorst10Frames;
    return stats;
}

double getDeltaTime(const Context* ctx) { 
    return ctx->deltaTime; 
}
void setTargetFPS(Context* ctx, double targetFps) { 
    ctx->targetFPS = targetFps; 
}

}; // C2Core::Time
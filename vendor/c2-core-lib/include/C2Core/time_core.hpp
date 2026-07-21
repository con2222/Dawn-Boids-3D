#ifndef TIME_CORE_H
#define TIME_CORE_H


namespace C2Core::Time {

typedef struct Context Context;

enum class WaitMode {
    Sleep,
    Spin,
    Hybrid
};

struct Stats {
    double currentFps;
    double averageFrameTime;
    double minFrameTime;
    double maxFrameTime;
    double low1Percent;
};

Context* create(double targetFps, double fixedUpdateRate);
void destroy(Context* ctx);
void startFrame(Context* ctx);
bool consumeFixedUpdate(Context* ctx);
void endFrame(Context* ctx, WaitMode mode);
double getDeltaTime(const Context* ctx);
void setTargetFPS(Context* ctx, double targetFps);
Stats getStats(const Context* ctx);

}; // C2Core::Time

#endif // TIME_CORE_H

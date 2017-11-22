#pragma once

#include "stdafx.h"

namespace anim
{
    enum class State
    {
        Stopped,
        Paused,
        Running
    };

    using TimeMs = int64_t;

    inline TimeMs getMs() noexcept
    {
        const static auto start = std::chrono::steady_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();
    }

    using transition = std::function<double(double delta, double dt)>;

    extern transition linear;
    extern transition sineInOut;
    extern transition halfSine;
    extern transition easeOutBack;
    extern transition easeInCirc;
    extern transition easeOutCirc;
    extern transition easeInCubic;
    extern transition easeOutCubic;
    extern transition easeInQuint;
    extern transition easeOutQuint;

    inline transition bumpy(double bump) {
        auto dt0 = (bump - std::sqrt(bump * (bump - 1.0)));
        auto k = (1.0 / (2.0 * dt0 - 1.0));
        return [bump, dt0, k](double delta, double dt) {
            return delta * (bump - k * (dt - dt0) * (dt - dt0));
        };
    }

    // Basic animated value.
    class value {
    public:
        using ValueType = double;

        value() = default;
        value(double from) : _cur(from), _from(from) {
        }
        value(double from, double to) : _cur(from), _from(from), _delta(to - from) {
        }
        void start(double to) {
            _from = _cur;
            _delta = to - _from;
        }
        void restart() {
            _delta = _from + _delta - _cur;
            _from = _cur;
        }

        double from() const {
            return _from;
        }
        double current() const {
            return _cur;
        }
        double to() const {
            return _from + _delta;
        }
        void add(double delta) {
            _from += delta;
            _cur += delta;
        }
        value &update(double dt, transition func) {
            _cur = _from + func(_delta, dt);
            return *this;
        }
        void finish() {
            _cur = _from + _delta;
            _from = _cur;
            _delta = 0.0;
        }

    private:
        double _cur = 0.0;
        double _from = 0.0;
        double _delta = 0.0;
    };

    inline int interpolate(int a, int b, double b_ratio) {
        return qRound(a + double(b - a) * b_ratio);
    }

class BasicAnimation;

class AnimationImplementation {
public:
    virtual void start() {}
    virtual void step(BasicAnimation *a, TimeMs ms, bool timer) = 0;
    virtual ~AnimationImplementation() {}

};

class AnimationCallbacks {
public:
    AnimationCallbacks(AnimationImplementation *implementation) : _implementation(implementation) {}
    AnimationCallbacks(const AnimationCallbacks &other) = delete;
    AnimationCallbacks &operator=(const AnimationCallbacks &other) = delete;
    AnimationCallbacks(AnimationCallbacks &&other) : _implementation(other._implementation) {
        other._implementation = nullptr;
    }
    AnimationCallbacks &operator=(AnimationCallbacks &&other) {
        std::swap(_implementation, other._implementation);
        return *this;
    }

    void start() { _implementation->start(); }
    void step(BasicAnimation *a, TimeMs ms, bool timer) { _implementation->step(a, ms, timer); }
    ~AnimationCallbacks() { delete _implementation; }

private:
    AnimationImplementation *_implementation;

};

class BasicAnimation {
public:
    BasicAnimation(AnimationCallbacks &&callbacks)
        : _callbacks(std::move(callbacks))
        , _animating(false) {
    }

    void start();
    void stop();

    void step(TimeMs ms, bool timer = false) {
        _callbacks.step(this, ms, timer);
    }

    void step() {
        step(getMs(), false);
    }

    bool animating() const {
        return _animating;
    }

    ~BasicAnimation() {
        if (_animating)
            stop();
    }

private:
    AnimationCallbacks _callbacks;
    bool _animating;

};

template <typename Type>
class AnimationCallbacksRelative : public AnimationImplementation {
public:
    typedef void (Type::*Method)(double, bool);

    AnimationCallbacksRelative(Type *obj, Method method) : _obj(obj), _method(method) {
    }

    void start() override {
        _started = getMs();
    }

    void step(BasicAnimation *a, TimeMs ms, bool timer) override {
        (_obj->*_method)(qMax(ms - _started, TimeMs(0)), timer);
    }

private:
    TimeMs _started = 0;
    Type *_obj = nullptr;
    Method _method = nullptr;

};

template <typename Type>
AnimationCallbacks animation(Type *obj, typename AnimationCallbacksRelative<Type>::Method method) {
    return AnimationCallbacks(new AnimationCallbacksRelative<Type>(obj, method));
}

template <typename Type>
class AnimationCallbacksAbsolute : public AnimationImplementation {
public:
    typedef void (Type::*Method)(TimeMs, bool);

    AnimationCallbacksAbsolute(Type *obj, Method method) : _obj(obj), _method(method) {
    }

    void step(BasicAnimation *a, TimeMs ms, bool timer) override {
        (_obj->*_method)(ms, timer);
    }

private:
    Type *_obj = nullptr;
    Method _method = nullptr;

};

template <typename Type>
AnimationCallbacks animation(Type *obj, typename AnimationCallbacksAbsolute<Type>::Method method) {
    return AnimationCallbacks(new AnimationCallbacksAbsolute<Type>(obj, method));
}

template <typename Type, typename Param>
class AnimationCallbacksRelativeWithParam : public AnimationImplementation {
public:
    typedef void (Type::*Method)(Param, double, bool);

    AnimationCallbacksRelativeWithParam(Param param, Type *obj, Method method) : _param(param), _obj(obj), _method(method) {
    }

    void start() override {
        _started = getMs();
    }

    void step(BasicAnimation *a, TimeMs ms, bool timer) override {
        (_obj->*_method)(_param, qMax(ms - _started, TimeMs(0)), timer);
    }

private:
    TimeMs _started = 0;
    Param _param;
    Type *_obj = nullptr;
    Method _method = nullptr;

};

template <typename Type, typename Param>
AnimationCallbacks animation(Param param, Type *obj, typename AnimationCallbacksRelativeWithParam<Type, Param>::Method method) {
    return AnimationCallbacks(new AnimationCallbacksRelativeWithParam<Type, Param>(param, obj, method));
}

template <typename Type, typename Param>
class AnimationCallbacksAbsoluteWithParam : public AnimationImplementation {
public:
    typedef void (Type::*Method)(Param, TimeMs, bool);

    AnimationCallbacksAbsoluteWithParam(Param param, Type *obj, Method method) : _param(param), _obj(obj), _method(method) {
    }

    void step(BasicAnimation *a, TimeMs ms, bool timer) override {
        (_obj->*_method)(_param, ms, timer);
    }

private:
    Param _param;
    Type *_obj = nullptr;
    Method _method = nullptr;

};

template <typename Type, typename Param>
AnimationCallbacks animation(Param param, Type *obj, typename AnimationCallbacksAbsoluteWithParam<Type, Param>::Method method) {
    return AnimationCallbacks(new AnimationCallbacksAbsoluteWithParam<Type, Param>(param, obj, method));
}

class Animation {
public:
    void step(TimeMs ms) {
        if (_data) {
            _data->a_animation.step(ms);
            if (_data && !_data->a_animation.animating()) {
                _data.reset();
            }
        }
    }

    bool animating() const {
        if (_data) {
            if (_data->a_animation.animating()) {
                return true;
            }
            _data.reset();
        }
        return false;
    }
    bool animating(TimeMs ms) {
        step(ms);
        return animating();
    }

    double current() const {
        assert(_data);
        return _data->value.current();
    }
    double current(double def) const {
        return animating() ? current() : def;
    }
    double current(TimeMs ms, double def) {
        return animating(ms) ? current() : def;
    }

    static constexpr auto kLongAnimationDuration = 1000;

    template <typename Lambda>
    void start(Lambda &&updateCallback, double from, double to, double duration, anim::transition transition = anim::linear, int loopCount = 1) {
        if (loopCount == 0)
        {
            assert(!"loopCount is 0");
            finish();
            return;
        }
        if (!_data)
            _data = std::make_unique<Data>(from, std::forward<Lambda>(updateCallback));

        _data->state = State::Running;
        _data->loopCount = loopCount;
        _data->currentLoop = 0;
        _data->duration = duration;
        _data->transition = transition;
        _data->value.start(to);
        _data->a_animation.start();
    }

    void finish() {
        if (_data) {
            _data->state = State::Stopped;
            _data->value.finish();
            _data->a_animation.stop();
            _data.reset();
        }
    }

    State state() const
    {
        if (_data)
            return _data->state;
        return State::Stopped;
    }

    void pause()
    {
        if (_data && _data->state == State::Running) {
            _data->state = State::Paused;
            _data->a_animation.stop();
        }
    }

    void resume()
    {
        if (_data && _data->state == State::Paused) {
            _data->state = State::Running;
            _data->a_animation.start();
        }
    }

    template <typename Lambda>
    void setUpdateCallback(Lambda &&updateCallback) {
        if (_data) {
            _data->updateCallback = std::forward<Lambda>(updateCallback);
        }
    }

private:

    struct Data {
        template <typename Lambda>
        Data(double from, Lambda updateCallback)
            : value(from, from)
            , a_animation(animation(this, &Data::step))
            , updateCallback(std::move(updateCallback)) {
        }

        void stop_impl()
        {
            state = State::Stopped;
            currentLoop = 0;
            value.finish();
            a_animation.stop();
        }

        void step(double ms, bool timer) {
            const auto dt = (ms / duration);
            if (dt > 1) {
                ++currentLoop;
                if (loopCount == -1 || currentLoop < loopCount)
                {
                    value = anim::value(value.from(), value.to());
                    a_animation.start();
                }
                else
                {
                    stop_impl();
                }
            }
            else
            {
                if (dt == 1 && (loopCount == 1 || (loopCount > 0 && (loopCount - currentLoop == 1))))
                    stop_impl();
                else
                    value.update(dt, transition);
            }
            updateCallback();
        }

        double duration = 0.0;
        int loopCount = 1;
        int currentLoop = 0;
        State state = State::Stopped;
        anim::value value;
        BasicAnimation a_animation;
        std::function<void()> updateCallback;
        anim::transition transition = anim::linear;
    };
    mutable std::unique_ptr<Data> _data;
};

    class AnimationManager final : public QObject
    {
        Q_OBJECT
    public:
        AnimationManager();
        ~AnimationManager();

        void start(BasicAnimation* animation);
        void stop(BasicAnimation* animation);

        static void startManager();
        static void stopManager();

    private:
        void timeout();

    private:
        std::unordered_set<BasicAnimation*> animations_;
        std::unordered_set<BasicAnimation*> stopping_;
        std::unordered_set<BasicAnimation*> starting_;

        bool animating_;

        QTimer timer_;
    };
}

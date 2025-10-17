#pragma once

template <class T>
class SingletonMixin {
public:
    SingletonMixin& operator = (const SingletonMixin&) = delete;
    SingletonMixin& operator = (SingletonMixin&&)      = delete;

    static T& GetInstance() {
        if(!m_instance)
            m_instance = new T_Instance;
        return *m_instance;
    }

protected:
    SingletonMixin() {}

private:
    struct T_Instance : public T {
        T_Instance() : T() {}
    };

    static inline T* m_instance = nullptr;
};

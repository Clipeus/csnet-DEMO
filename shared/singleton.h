#pragma once

#include <memory>

namespace csnet
{
namespace shared
{

// heclper class for singleton object support
template <class T>
class singleton
{
    friend struct deleter;
    template<class U> friend class std::unique_ptr;
    
public:
    // singleton object pointer
    static T* instance()
    {
        if (!_instance)
        {
            T::create();
        }
        return _instance.get();
    }

protected:
    // create singleton object
    static T* create()
    {
        std::unique_ptr<T, singleton<T>::deleter> temp(new T);
        _instance = std::move(temp);
    }
    
    // unique_ptr deletor
    struct deleter 
    {
        virtual void operator()(T* b) { delete b; }
    };
    
    // define virtual destructor for child
    virtual ~singleton() {}
    
protected:
    static std::unique_ptr<T, singleton<T>::deleter> _instance;
};

}
}

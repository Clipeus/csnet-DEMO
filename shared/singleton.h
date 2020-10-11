#pragma once

#include <stdexcept>
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
      template<class U, class D> friend class std::unique_ptr;

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
        return _instance.get();
      }

      // unique_ptr deletor
      struct deleter
      {
        virtual void operator()(T* b) { delete b; }
      };

      // define virtual destructor for child
      virtual ~singleton() {}

    protected:
      static std::unique_ptr<T, deleter> _instance;
    };

    template <class T>
    class singleton_holder
    {
    public:
      // is object created?
      static bool created()
      {
        return _instance;
      }

      // create singleton object
      template<typename... Args>
      static void create(Args&&... args)
      {
        _instance.reset(new T(std::forward<Args>(args)...));
        //_instance = std::make_unique<T>(std::forward<Args>(args)...);
      }

      // singleton object pointer
      static T& instance()
      {
        if (!_instance)
          throw std::runtime_error("Object is not created");
        return *_instance.get();
      }

    private:
      static std::unique_ptr<T> _instance;
    };

  }
}

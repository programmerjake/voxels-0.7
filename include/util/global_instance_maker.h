#ifndef GLOBAL_INSTANCE_MAKER_H_INCLUDED
#define GLOBAL_INSTANCE_MAKER_H_INCLUDED

namespace programmerjake
{
namespace voxels
{
template <typename T>
class global_instance_maker final
{
private:
    static T *instance;
    static void init()
    {
        instance = new T();
    }
    static void deinit()
    {
        delete instance;
    }
    struct helper final
    {
        helper(const helper &) = delete;
        void operator =(const helper &) = delete;
        helper()
        {
            init();
        }
        ~helper()
        {
            deinit();
        }
        const T *passthrough(const T *retval)
        {
            return retval;
        }
    };
    static helper theHelper;
public:
    static const T *getInstance()
    {
        return theHelper.passthrough(instance);
    }
};

template <typename T>
typename global_instance_maker<T>::helper global_instance_maker<T>::theHelper;

template <typename T>
T *global_instance_maker<T>::instance = nullptr;
}
}

#endif // GLOBAL_INSTANCE_MAKER_H_INCLUDED

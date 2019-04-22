#ifndef BASE_SINGLETON_H_
#define BASE_SINGLETON_H_


template <typename T>
T *Singleton()
{
    static T instance;
    return &instance;
}


#endif /* BASE_SINGLETON_H_ */


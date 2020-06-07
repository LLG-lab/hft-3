/**********************************************************************\
**                                                                    **
**             -=≡≣ High Frequency Trading System  ≣≡=-              **
**                                                                    **
**          Copyright  2017 - 2020 by LLG Ryszard Gradowski          **
**                       All Rights Reserved.                         **
**                                                                    **
**  CAUTION! This application is an intellectual propery              **
**           of LLG Ryszard Gradowski. This application as            **
**           well as any part of source code cannot be used,          **
**           modified and distributed by third party person           **
**           without prior written permission issued by               **
**           intellectual property owner.                             **
**                                                                    **
\**********************************************************************/

#include <auto_deallocator.hpp>

#undef AUTODEALLOCATOR_TEST

#ifdef AUTODEALLOCATOR_TEST
#include <iostream>
#endif

//
// Static member initialization.
//

auto_deallocator::resource auto_deallocator::records_;

void *auto_deallocator::operator new(std::size_t size)
{
    #ifdef AUTODEALLOCATOR_TEST
    std::cout << "auto_deallocator: got called [operator new]\n";
    #endif

    void *obj = ::operator new(size);
    auto_deallocator::records_.register_object(obj);

    return obj;
}

void auto_deallocator::operator delete(void *raw_memory, std::size_t size)
{
    #ifdef AUTODEALLOCATOR_TEST
    std::cout << "auto_deallocator: got called [operator delete("
              << raw_memory << ")]\n";
    #endif

    auto_deallocator::records_.unregister_object(raw_memory);
    ::operator delete(raw_memory);
}

auto_deallocator::resource::~resource(void)
{
    cleanup();
}

void auto_deallocator::resource::cleanup(void)
{
    std::set<void *>::iterator it;

    while (true)
    {
        it = registered_.begin();

        if (it == registered_.end())
        {
            break;
        }

        delete reinterpret_cast<auto_deallocator *>(*it);
    }
}

void auto_deallocator::resource::register_object(void *obj)
{
    #ifdef AUTODEALLOCATOR_TEST
    std::cout << "auto_deallocator::resource: Registering object ["
              << obj << "]\n";
    #endif

    registered_.insert(obj);
}

void auto_deallocator::resource::unregister_object(void *obj)
{
    #ifdef AUTODEALLOCATOR_TEST
    std::cout << "auto_deallocator::resource: Unregistering object ["
              << obj << "]\n";
    #endif

    registered_.erase(obj);
}

#include <cstdlib>
#include <functional>
#include <stdexcept>
#include <iostream>
#include <string>
#include <cstdint>
#include <thread>

#include <boost/fiber/all.hpp>

int32_t value1 = 0;
int32_t value2 = 0;

void fn1() {
    boost::fibers::fiber::id id = boost::this_fiber::get_id();
    for (int32_t i = 0; i < 5; ++i)
    {
        ++value1;
        std::cout << "thread " << std::this_thread::get_id() << "fiber "
                  << id << " fn1: increment value1: " << value1 << std::endl;
        // Relinquishes execution control, allowing other fibers to run.
        // @note A fiber that calls yield() is not suspended:
        // it is immediately passed to the scheduler as ready to run.
        boost::this_fiber::yield();
    }
    std::cout << "thread " << std::this_thread::get_id() << "fiber "
              << id << " fn1: returns" << std::endl;
}

void fn2( boost::fibers::fiber &f1) {
    boost::fibers::fiber::id this_id = boost::this_fiber::get_id();
    for (int32_t i = 0; i < 5; ++i)
    {
        ++value2;
        std::cout << "thread " << std::this_thread::get_id() << "fiber " << this_id
                  << " fn2: increment value2: " << value2 << std::endl;
        if ( i == 1)
        {
            boost::fibers::fiber::id id = f1.get_id();
            std::cout << "thread " << std::this_thread::get_id() << "fiber " << id
                      << " fn2: joins fiber " << this_id << std::endl;
            // blocks till f1 is done
            f1.join();
            std::cout << "thread " << std::this_thread::get_id() << "fiber " << id
                      << " fn2: joined fiber " << this_id<< std::endl;
        }
        // Relinquishes execution control, allowing other fibers to run.
        boost::this_fiber::yield();
    }
    std::cout << "thread " << std::this_thread::get_id() << "fiber " << this_id
              << " fn2: returns" << std::endl;
}

int main() {
    try
    {
        boost::fibers::fiber f1( fn1);
        boost::fibers::fiber f2( fn2, std::ref( f1) );

        f2.join();

        std::cout << "thread " << std::this_thread::get_id() << "done." << std::endl;

        return EXIT_SUCCESS;
    }
    catch ( std::exception const& e)
    { std::cerr << "exception: " << e.what() << std::endl; }
    catch (...)
    { std::cerr << "unhandled exception" << std::endl; }
    return EXIT_FAILURE;
}
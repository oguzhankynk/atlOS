/**
 * @file condition_variable.cpp
 * @author Oguzhan KAYNAK
 * @brief Sync::ConditionVariable: wait until a condition may have become true.
 */

#include <sync/condition_variable.hpp>

namespace Sync {

ConditionVariable::ConditionVariable(Threading::Scheduler& scheduler)
    : m_waiters(scheduler)
{
}

void ConditionVariable::wait(Mutex& mutex)
{
    // "Unlock and wait" must be one step, or a notify could come in between
    // and be lost. Cooperative scheduling gives that for free: unlock() only
    // marks the next owner runnable, nothing else runs before wait() blocks.
    mutex.unlock();
    m_waiters.wait();
    mutex.lock();
}

void ConditionVariable::notifyOne()
{
    m_waiters.wakeOne();
}

void ConditionVariable::notifyAll()
{
    m_waiters.wakeAll();
}

} // namespace Sync

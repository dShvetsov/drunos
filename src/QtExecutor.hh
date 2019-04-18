#pragma once

#include "Common.hh"

#include <QtCore>

#include <boost/thread/concurrent_queues/queue_op_status.hpp>

#include <utility>
#include <mutex>

#include <boost/thread/future.hpp>

namespace runos {

using boost::shared_future;
using boost::promise;
using boost::packaged_task;
using boost::future_error;
using boost::make_ready_future;
using boost::async;

struct qt_executor
{
    qt_executor(QObject* target)
        : target(target)
    {
    }

    template<class Closure>
    void submit(Closure&& closure)
    {
        QObject signalSource;
        QObject::connect(
            &signalSource,
            &QObject::destroyed,
            target,
            [f = std::forward<Closure>(closure)]() mutable { f(); }
        );
    }

    void close()
    {
        target = nullptr;
    }

    bool closed(std::lock_guard< std::mutex > &)
    {
        return target == nullptr;
    }

    bool closed()
    {
        std::lock_guard<std::mutex> lk(m);
        return closed(lk);
    }

    bool try_executing_one()
    {
        return false;
    }

    template<class Pred>
    bool reschedule_until(Pred const& )
    {
        return false;
    }

private:
    std::mutex m;
    QObject* target;
};

} // namespace runos

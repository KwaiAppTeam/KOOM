// Author: Qiushi Xue <xueqiushi@kuaishou.com>

#ifndef _LIBS_UTILS_RWLOCK_H
#define _LIBS_UTILS_RWLOCK_H

#include <stdint.h>
#include <sys/types.h>

#if !defined(_WIN32)
# include <pthread.h>
#endif

#include <utils/Errors.h>
#include <utils/ThreadDefs.h>

// ---------------------------------------------------------------------------
namespace android {
// ---------------------------------------------------------------------------

#if !defined(_WIN32)

/*
 * Simple mutex class.  The implementation is system-dependent.
 *
 * The mutex must be unlocked by the thread that locked it.  They are not
 * recursive, i.e. the same thread can't lock it multiple times.
 */
    class RWLock {
        public:
        enum {
            PRIVATE = 0,
            SHARED = 1
        };

        RWLock();
        explicit    RWLock(const char* name);
        explicit    RWLock(int type, const char* name = nullptr);
        ~RWLock();

        status_t    readLock();
        status_t    tryReadLock();
        status_t    writeLock();
        status_t    tryWriteLock();
        void        unlock();

        class AutoRLock {
            public:
            inline explicit AutoRLock(RWLock& rwlock) : mLock(rwlock)  { mLock.readLock(); }
            inline ~AutoRLock() { mLock.unlock(); }
            private:
            RWLock& mLock;
        };

        class AutoWLock {
            public:
            inline explicit AutoWLock(RWLock& rwlock) : mLock(rwlock)  { mLock.writeLock(); }
            inline ~AutoWLock() { mLock.unlock(); }
            private:
            RWLock& mLock;
        };

        private:
        // A RWLock cannot be copied
        RWLock(const RWLock&);
        RWLock&      operator = (const RWLock&);

        pthread_rwlock_t mRWLock;
    };

    inline RWLock::RWLock() {
        pthread_rwlock_init(&mRWLock, nullptr);
    }
    inline RWLock::RWLock(__attribute__((unused)) const char* name) {
        pthread_rwlock_init(&mRWLock, nullptr);
    }
    inline RWLock::RWLock(int type, __attribute__((unused)) const char* name) {
        if (type == SHARED) {
            pthread_rwlockattr_t attr;
            pthread_rwlockattr_init(&attr);
            pthread_rwlockattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
            pthread_rwlock_init(&mRWLock, &attr);
            pthread_rwlockattr_destroy(&attr);
        } else {
            pthread_rwlock_init(&mRWLock, nullptr);
        }
    }
    inline RWLock::~RWLock() {
        pthread_rwlock_destroy(&mRWLock);
    }
    inline status_t RWLock::readLock() {
        return -pthread_rwlock_rdlock(&mRWLock);
    }
    inline status_t RWLock::tryReadLock() {
        return -pthread_rwlock_tryrdlock(&mRWLock);
    }
    inline status_t RWLock::writeLock() {
        return -pthread_rwlock_wrlock(&mRWLock);
    }
    inline status_t RWLock::tryWriteLock() {
        return -pthread_rwlock_trywrlock(&mRWLock);
    }
    inline void RWLock::unlock() {
        pthread_rwlock_unlock(&mRWLock);
    }

#endif // !defined(_WIN32)

// ---------------------------------------------------------------------------
}  // namespace android
// ---------------------------------------------------------------------------

#endif // _LIBS_UTILS_RWLOCK_H
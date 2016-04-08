/**
 * @author David Hatch
 */

#include <pthread.h>

/**
 * Provides mutual exclusion, using pthread.
 */
class Pthread_mutex {
public:
    Pthread_mutex() {
        pthread_mutex_init(&mutex_handle, NULL);
    }

    inline void lock() {
        pthread_mutex_lock(&mutex_handle);
    }

    inline void unlock() {
        pthread_mutex_unlock(&mutex_handle);
    }
private:
    pthread_mutex_t mutex_handle;
};

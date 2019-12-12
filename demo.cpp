#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <thread>
#include <mutex>


class PrivateObject {
public:
    PrivateObject() {
        pipe(controlFds);
    }
    virtual ~PrivateObject() {
        close(controlFds[0]);
        close(controlFds[1]);
        if (recvThread != nullptr)
        {
            stop();
        }
    }

public:
    int start() {
        std::lock_guard<std::mutex> lock(mtx);

        if (recvThread) {
            printf("trigger recvier already started");
            return 1;
        }

        recvThread = new std::thread(&PrivateObject::threadEntity, this);

        return 0;
    }

    int stop() {
        std::lock_guard<std::mutex> lock(mtx);

        if (!recvThread) {
            printf("trigger recvier not started");
            return 1;
        }

        uint8_t c = 'q';
        write(controlFds[1], &c, 1);

        if (std::this_thread::get_id() != recvThread->get_id()) {
            recvThread->join();
        } else {
            printf("can not join in same thread");
        }

        delete recvThread;
        recvThread = nullptr;

        return 0;
    }

protected:
    void threadEntity() {

        while (true) {
            if (selectPass() < 0) {
                printf("trigger recvier thread break");
                break;
            }
        }
    }

    int selectPass() {
        fd_set fds;

        // select start
        FD_ZERO(&fds);
        FD_SET(controlFds[0], &fds);

        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 200*1000;;
        int ret = select(controlFds[0]+1, &fds, 0, &fds, &tv);

        if (ret < 0) {
            // error
            printf("select error: %s", strerror(errno));
        }
        else if(ret == 0){
            getData();
        }
        else if (FD_ISSET(controlFds[0], &fds)) {
            // need to stop the loop
            char c;
            read(controlFds[0], &c, sizeof(c));
            return -1;
        }

        return 0;
    }

    int getData()
    {
        return 0;
    }

protected:
    std::mutex mtx;
    std::thread *recvThread = nullptr;

private:
    int controlFds[2];
};

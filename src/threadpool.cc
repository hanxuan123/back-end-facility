#include "threadpool.h"
#include "SpinlockQueue.h"

struct CompareTaskPriority 
{
    inline bool operator()(const ITask*& x, const ITask*&y) const 
    {
        return x->Priority() < y->Priority();
    }
};

class Dispatcher:public WorkerBodyBase
{
    public:

        Dispatcher(ThreadPool* pool, int workerNum = DEFAULT_WORKER_TASK_MSG_SIZE);
        ~Dispatcher();

        virtual bool PostTask(ITask*);
        virtual int  GetTaskNumber();
        virtual void ClearAllTask();

    protected:

        virtual void Run();
        virtual ITask* GetTask();
        
    private:

        ThreadPool* m_pool;
        sem_t m_notifyer;

        std::vector<Thread*> m_workers;
        SpinlockQueue<ITask*, std::priority_queue<ITask*, \
            std::vector<ITask*>, CompareTaskPriority> > m_queue;
};






ThreadPool::ThreadPool(int num)
    :Worker(new Dispatcher(this, num))
{
}


ThreadPool::~ThreadPool()
{
}

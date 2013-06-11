#ifndef _WORKER_THREAD_H_
#define _WORKER_THREAD_H_

#include "thread.h"
#include "defs.h"
#include "ITask.h"
#include "SpinlockQueue.h"

#include <semaphore.h>

class WorkerBodyBase: public ITask
{
    public:

        WorkerBodyBase();
        virtual ~WorkerBodyBase();

        virtual bool PostTask(ITask*) = 0;
        virtual int  GetTaskNumber() = 0;
        virtual void ClearAllTask()=0;

        virtual void StopRunning();
        virtual bool IsRunning() const {return m_isRuning;}
        virtual void SetNotifyer(sem_t* notify) { m_notifyer = notify; }

    protected:

        virtual void Run()=0;

        //get task from mailbox
        //may block when there is no task.
        //caller take responsibility to free the task.
        virtual ITask* GetTask() = 0;

        inline void SetStopState(bool shouldStop);
        inline void SignalPost();
        inline void SignalConsume();
        inline bool TryConsume();
        inline void Notify() { if (m_notifyer) sem_post(m_notifyer); }

        volatile bool m_isRuning;
        volatile bool m_shouldStop;


    private:

        sem_t m_sem;
        sem_t* m_notifyer;
};


class WorkerBody: public WorkerBodyBase
{
    public:

        WorkerBody(int maxMsgSize = DEFAULT_WORKER_TASK_MSG_SIZE);
        ~WorkerBody();

        virtual bool PostTask(ITask*);
        virtual int  GetTaskNumber();
        virtual void ClearAllTask();

        //take care of calling this function.
        //multitasking-opertion on m_mailbox will 
        //greatly reduce performance.
        //don't call it unless really necessary.
        ITask* TryGetTask();

    protected:

        virtual void Run();
        virtual ITask* GetTask();
        inline bool CheckAvailability();

    private:

        SpinlockQueue<ITask*> m_mailbox;
};


class Worker:public Thread
{
    public:

        Worker(int maxMsgSize = DEFAULT_WORKER_TASK_MSG_SIZE);
        ~Worker();

        virtual bool IsRunning(){ return m_WorkerBody->IsRunning(); }
        void StopRunning() { m_WorkerBody->StopRunning(); }
        bool PostTask(ITask* msg) { return m_WorkerBody->PostTask(msg); }
        int  GetTaskNumber() { return m_WorkerBody->GetTaskNumber(); }


        bool StartWorking();

    protected:

        Worker(WorkerBodyBase*);

        //disable setting task. 
        //this is a special thread specific to a worker thread.
        //It should not be changed externally.
        using Thread::SetTask;
        using Thread::Start;

        WorkerBodyBase* m_WorkerBody;
};

#endif


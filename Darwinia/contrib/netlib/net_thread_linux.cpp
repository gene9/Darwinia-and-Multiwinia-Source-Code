// See net_thread.h for module description

#include "net_thread.h"


NetThread::NetThread()
{
#ifdef HAVE_THREADS
   pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, (int *)0);
   pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, (int *)0);
   m_retVal = NULL;
#endif
}


NetThread::~NetThread()
{
}


NetThreadHandle NetThread::GetThreadHandle()
{
   return m_threadHandle;
}


NetRetCode NetThread::Start(NetCallBack fnptr, void *ptr)
{
   NetRetCode retVal = NetOk;
#ifdef HAVE_THREADS
   if (pthread_create(&m_threadHandle,NULL,fnptr,ptr))
   {
      NetDebugOut("thread creation failed");
      retVal = NetFailed;
   }
#endif
   return retVal;
}


NetRetCode NetThread::Join()
{
   NetRetCode retVal = NetOk;
#ifdef HAVE_THREADS
   if (pthread_join(m_threadHandle,&m_retVal))
   {
      NetDebugOut("thread join failed.");
      retVal = NetFailed;
   }
#endif
   return retVal;
}


NetRetCode NetThread::Detach()
{
   NetRetCode retVal = NetOk;
#ifdef HAVE_THREADS
   if (pthread_detach(m_threadHandle))
   {
      NetDebugOut("thread detach failed.");
      retVal = NetFailed;
   }
#endif
   return retVal;
}


void NetThread::TestCancel()
{
#ifdef HAVE_THREADS
   pthread_testcancel();
#endif
}


void NetThread::Stop(int exitStatus)
{
#ifdef HAVE_THREADS
   pthread_cancel(m_threadHandle);
#endif
}


void NetThread::Exit()
{
#ifdef HAVE_THREADS
   pthread_exit(m_retVal);
#endif
}

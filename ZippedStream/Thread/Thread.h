// Supported with union (c) 2018 Union team

#ifndef __UNION_THREAD_H__
#define __UNION_THREAD_H__
#include <iostream>
using std::string;
using std::cout;
using std::endl;
using std::cin;

namespace Common {
  typedef void* HPROC;

  enum {
    THREAD_TO_BACKGROUND   =  0x00010000,
    THREAD_FROM_BACKGROUND =  0x00020000,
    THREAD_IDLE            = -15,
    THREAD_LOW             = -2,
    THREAD_NORMAL_LOW      = -1,
    THREAD_NORMAL          =  0,
    THREAD_NORMAL_HIGH     =  1,
    THREAD_HIGH            =  2,
    THREAD_CRITICAL        =  15
  };



  class Thread {
  protected:
    HPROC Function;
    ulong ID;
    HANDLE Handle;

  public:
    Thread();
    Thread( HPROC function );
    void Init( HPROC function );
    ulong Detach( void* argument = Null );
    ulong DetachSuspended( void* argument = Null );
    void Break();
    void Suspend();
    void Resume();
    void SetPriority( int priority );
    int GetPriority();
    HPROC GetFunction();
    ulong GetID();
    HANDLE GetHandle();
    ~Thread();
  };

  inline Thread::Thread() {
    Init( Null );
  }

  inline Thread::Thread( HPROC function ) {
    Init( function );
  }

  inline void Thread::Init( HPROC function ) {
    Function = function;
    ID = Invalid;
    Handle = Null;
  }

  inline ulong Thread::Detach( void* argument ) {
    Handle = CreateThread( Null, 0, (LPTHREAD_START_ROUTINE)Function, argument, 0, &ID );
    return ID;
  }

  inline ulong Thread::DetachSuspended( void* argument ) {
    Handle = CreateThread( Null, 0, (LPTHREAD_START_ROUTINE)Function, argument, CREATE_SUSPENDED, &ID );
    return ID;
  }

  inline void Thread::Break() {
    TerminateThread( Handle, 0 );
  }

  inline void Thread::Suspend() {
    SuspendThread( Handle );
  }

  inline void Thread::Resume() {
    ulong result = ResumeThread( Handle ) > 0;
    if( result > 0 )
      while( ResumeThread( Handle ) > 0 );
  }

  inline void Thread::SetPriority( int priority ) {
    SetThreadPriority( Handle, priority );
  }

  inline int Thread::GetPriority() {
    return GetThreadPriority( Handle );
  }

  inline HPROC Thread::GetFunction() {
    return Function;
  }

  inline ulong Thread::GetID() {
    return ID;
  }

  inline HANDLE Thread::GetHandle() {
    return Handle;
  }

  inline Thread::~Thread() {
    // pass
  }



  class Mutex {
    HANDLE Handle;

  public:
    Mutex( LPSTR name = Null );
    Mutex( bool_t create_locked, LPSTR name = Null );
    ulong Enter();
    ulong Enter( const ulong& millisecond );
    void Leave();
    void Close();
    ~Mutex();
  };

  inline Mutex::Mutex( LPSTR name ) {
    Handle = CreateMutex( Null, False, name );
    cout << Handle << endl;
  };

  inline Mutex::Mutex( bool_t create_locked, LPSTR name ) {
    Handle = CreateMutex( Null, create_locked, name );
    cout << Handle << endl;
  };

  inline ulong Mutex::Enter() {
    return WaitForSingleObject( Handle, -1 );
  };

  inline ulong Mutex::Enter( const ulong& millisecond ) {
    return WaitForSingleObject( Handle, millisecond );
  };

  inline void Mutex::Leave() {
    ReleaseMutex( Handle );
  };

  inline void Mutex::Close() {
    if( Handle != Null ) {
      CloseHandle( Handle );
      Handle = Null;
    }
  }

  inline Mutex::~Mutex() {
    Close();
  };




  class ThreadLocker {
    CRITICAL_SECTION CriticalSection;

  public:
    ThreadLocker();
    void Enter();
    void Leave();
    bool IsLocked();
    ~ThreadLocker();
  };

  inline ThreadLocker::ThreadLocker() {
    InitializeCriticalSection( &CriticalSection );
  };

  inline void ThreadLocker::Enter() {
    EnterCriticalSection( &CriticalSection );
  };

  inline void ThreadLocker::Leave() {
    LeaveCriticalSection( &CriticalSection );
  };

  inline bool ThreadLocker::IsLocked() {
    return TryEnterCriticalSection( &CriticalSection ) ? true : false;
  }

  inline ThreadLocker::~ThreadLocker() {
    DeleteCriticalSection( &CriticalSection );
  };
}

#endif
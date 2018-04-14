/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * memory_monitor.cpp
 *
 */

#include "memory_monitor.hpp"

#ifdef __MEMORY_MONITOR_SET_ON

#include <cstdio>
#include <cstdlib>
#include <algorithm>
#include <unistd.h>
#include <execinfo.h>
#include <errno.h>
#include <string.h>

namespace monitor
{
  using std::set;

  //Maximum depth of call-stack
  const int BT_BUFFER_SIZE = 100;

  class MonitorImpl : public Monitor
  {
  private:
    bool m_watchAlloc;
    bool m_watchFree;
    size_t m_timeStamp;
    set<void*> m_records;

    MonitorImpl () :
	m_watchAlloc (false), m_watchFree (false), m_timeStamp (0)
    {
    }

    MonitorImpl (const MonitorImpl&) = delete;

    MonitorImpl&
    operator= (const MonitorImpl&) = delete;

  public:
    ~MonitorImpl () = default;

    static MonitorImpl&
    getInstance ()
    {
      static MonitorImpl inst;
      return inst;
    }

    virtual std::vector<PMemoryBlockInfo>
    getRecords () override;

    virtual void
    start () override
    {
      m_watchAlloc = true;
      m_watchFree = true;
    }

    virtual void
    stop () override
    {
      m_watchAlloc = false;
    }

    virtual void
    stopAll () override
    {
      m_watchAlloc = false;
      m_watchFree = false;
    }

    virtual void
    writeLogFile (const char* path) override;

    void*
    getMem (size_t size);

    void
    freeMem (void* ptr) noexcept;
  };

  Monitor&
  Monitor::getInstance ()
  {
    return static_cast<Monitor&> (MonitorImpl::getInstance ());
  }

  void*
  MonitorImpl::getMem (size_t size)
  {
    if (!m_watchAlloc) {
      return malloc (size);
    }
    else {
      // Pause monitoring
      m_watchAlloc = false;

      static void* buffer[BT_BUFFER_SIZE];

      // Get call stack
      int nptrs = backtrace (buffer, BT_BUFFER_SIZE);

      // Calculate total size and allocate memory
      size_t allSize = MemoryBlockInfo::calcTotalSize (size, nptrs);
      PMemoryBlockInfo ret = reinterpret_cast<PMemoryBlockInfo> (malloc (allSize));

      // Write info
      ++m_timeStamp;
      ret->size = size;
      ret->timeStamp = m_timeStamp;
      ret->nStacks = nptrs;
      std::copy (buffer, buffer + nptrs, ret->callStack);

      // Write offset and record
      ret->offset () = allSize - size;
      m_records.insert (ret->data ());

      m_watchAlloc = true;
      return ret->data ();
    }
  }

  void
  MonitorImpl::freeMem (void* ptr) noexcept
  {
    auto it = m_records.find (ptr);
    if (it == m_records.end ()) {
      // Not recorded
      free (ptr);
    }
    else {
      // Get record
      PMemoryBlockInfo rec = MemoryBlockInfo::fromData (ptr);
      m_records.erase (it);

      if (m_watchFree) {
        // Just erase the record
        free (rec);
      }
      else {
        // Free the memory, but record remained
        rec = reinterpret_cast<PMemoryBlockInfo> (realloc (rec, rec->offset ()));
        if (rec == nullptr) {
          // This should NEVER happen
          exit (1);
        }
        m_records.insert (rec->data ());
      }
    }
  }

  std::vector<PMemoryBlockInfo>
  MonitorImpl::getRecords ()
  {
    std::vector<PMemoryBlockInfo> ret;

    for (auto ptr : m_records) {
      ret.push_back (MemoryBlockInfo::fromData (ptr));
    }
    std::sort (ret.begin (), ret.end (),
               [](const PMemoryBlockInfo lhs, const PMemoryBlockInfo rhs)->bool {
                 return lhs->timeStamp < rhs->timeStamp;
               });

    return ret;
  }

  void
  MonitorImpl::writeLogFile (const char* path)
  {
    std::vector<PMemoryBlockInfo> records = getRecords ();

    FILE* fd = fopen (path, "w+");
    if (fd != nullptr) {
      for (auto rec : records) {
        fprintf (fd, ">>TIME:%lu\n>>SIZE:%lu\n>>DEPT:%d\n>>ADDR:\n",
                 rec->timeStamp, rec->size, rec->nStacks);
        //for synchronization
        fflush (fd);
        backtrace_symbols_fd (rec->callStack, rec->nStacks, fileno (fd));
        fprintf (fd, "<<\n");
        //for sudden break
        fflush (fd);
      }
      fclose (fd);
    }
    else {
      fprintf (stderr, "writeLogFile failed with errno:%d: %s\n",
               errno, strerror (errno));
    }
  }

} // namespace monitor

void*
operator new (size_t size)
{
  return monitor::MonitorImpl::getInstance ().getMem (size);
}

void
operator delete (void* ptr) noexcept
{
  monitor::MonitorImpl::getInstance ().freeMem (ptr);
}

#endif // __MEMORY_MONITOR_SET_ON

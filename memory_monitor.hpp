/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * memory_monitor.hpp
 *
 */

#ifndef NFD_CORE_MEMORY_MONITOR_HPP
#define NFD_CORE_MEMORY_MONITOR_HPP

#define __MEMORY_MONITOR_SET_ON
#ifdef __MEMORY_MONITOR_SET_ON

#include <set>
#include <vector>
#include <cstdlib>

void*
operator new (size_t size);
void
operator delete (void* ptr) noexcept;

namespace monitor
{

  struct MemoryBlockInfo;
  typedef struct MemoryBlockInfo *PMemoryBlockInfo;
  typedef void* /*__attribute__((__may_alias__))*/LPVOID;

  /**
   * @brief Infomation about a memory block.
   *
   * When a tracked memory block is created, the info is stored before data.
   * Like this:
   * +------+----+---------+----------+-----+------------+--------+------+
   * | size | no | nStacks | stack[0] | ... | stack[n-1] | offset | Data |
   * +------+----+---------+----------+-----+------------+--------+------+
   *                                                               ^
   *                                                               |
   *                                                          Return of new
   *
   */
  struct MemoryBlockInfo
  {
    /// The required size of block
    size_t size;

    /// No. after start
    size_t timeStamp;

    /// The depth of call-stack. Maximum value is BT_BUFFER_SIZE defined in cpp.
    int nStacks;

    /// The call-stack. Length = nStacks
    LPVOID callStack[0];

    /**
     * @brief Get the position holds Offset.
     *
     * Offset means distance = &Block->Data - &Block->MemoryInfoBlock.
     */
    size_t&
    offset ()
    {
      return reinterpret_cast<size_t&> (callStack[nStacks]);
    }

    /**
     * @brief Get the pointer to Data.
     */
    LPVOID
    data ()
    {
      size_t* ret = &(this->offset ());
      return reinterpret_cast<LPVOID> (ret + 1);
    }

    /**
     * @brief Calculate the whole size need to be allocated.
     */
    static size_t
    calcTotalSize (size_t size, int nptrs)
    {
      return sizeof(MemoryBlockInfo) + sizeof(LPVOID) * nptrs + sizeof(size_t)
	  + size;
    }

    /**
     * @brief Get the pointer to MemoryBlockInfo from pointer to Data
     */
    static PMemoryBlockInfo
    fromData (void* ptr)
    {
      size_t *offset = reinterpret_cast<size_t*> (ptr) - 1;
      char* ret = reinterpret_cast<char*> (ptr) - *offset;
      return reinterpret_cast<PMemoryBlockInfo> (ret);
    }
  }__attribute__((packed));

  /**
   * @brief a global singleton memory monitor of NFD and ndnSim.
   */
  class Monitor
  {
  private:
    //It takes too long when modifying a header, so put the impl into cpp.

    Monitor (const Monitor&) = delete;
    Monitor&
    operator= (const Monitor&) = delete;

  protected:
    Monitor () = default;

  public:
    /**
     * @brief Get a global instance
     */
    static Monitor&
    getInstance ();

    virtual
    ~Monitor () = default;

    /**
     * @brief Get the list of recored memory blocks
     * @return a vector sorted by time stamp.
     */
    virtual std::vector<PMemoryBlockInfo>
    getRecords () = 0;

    /**
     * @brief Start to record allocations and releasing.
     */
    virtual void
    start () = 0;

    /**
     * @brief Stop recording allocations.
     *
     * Stop recording allocations but continue monitoring releasing of memory.
     */
    virtual void
    stop () = 0;

    /**
     * @brief Stop recording allocations and releasing.
     *
     * After this function called the record list will never change.
     * When a require to free a recorded memory comes,
     *  the Data part will be freed with MemoryBlockInfo remained.
     * Please call this previous to Simulator::Destroy()
     *  if want to use record list after Simulator::Destroy()
     */
    virtual void
    stopAll () = 0;

    /**
     * @brief Write record list to a specified file.
     *
     * @remark No exception thrown as operator new was still hijacked.
     */
    virtual void
    writeLogFile (const char* path) = 0;
  };

} // namespace monitor

#endif // __MEMORY_MONITOR_SET_ON

#endif // NFD_CORE_MEMORY_MONITOR_HPP

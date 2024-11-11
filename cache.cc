#include "simulator.h"
#include "cache.h"
#include "log.h"
#define SATURATION_LIMIT 4
// Cache class
// constructors/destructors
std::unordered_map<IntPtr, HistoryTableEntry> history_table;

Cache::Cache(
   String name,
   String cfgname,
   core_id_t core_id,
   UInt32 num_sets,
   UInt32 associativity,
   UInt32 cache_block_size,
   String replacement_policy,
   cache_t cache_type,
   hash_t hash,
   FaultInjector *fault_injector,
   AddressHomeLookup *ahl)
:
   CacheBase(name, num_sets, associativity, cache_block_size, hash, ahl),
   m_enabled(false),
   m_num_accesses(0),
   m_num_hits(0),
   m_cache_type(cache_type),
   m_fault_injector(fault_injector)
{
   m_set_info = CacheSet::createCacheSetInfo(name, cfgname, core_id, replacement_policy, m_associativity);
   m_sets = new CacheSet*[m_num_sets];
   for (UInt32 i = 0; i < m_num_sets; i++)
   {
      m_sets[i] = CacheSet::createCacheSet(cfgname, core_id, replacement_policy, m_cache_type, m_associativity, m_blocksize, m_set_info);
   }

   #ifdef ENABLE_SET_USAGE_HIST
   m_set_usage_hist = new UInt64[m_num_sets];
   for (UInt32 i = 0; i < m_num_sets; i++)
      m_set_usage_hist[i] = 0;
   #endif
}

Cache::~Cache()
{
   #ifdef ENABLE_SET_USAGE_HIST
   printf("Cache %s set usage:", m_name.c_str());
   for (SInt32 i = 0; i < (SInt32) m_num_sets; i++)
      printf(" %" PRId64, m_set_usage_hist[i]);
   printf("\n");
   delete [] m_set_usage_hist;
   #endif

   if (m_set_info)
      delete m_set_info;

   for (SInt32 i = 0; i < (SInt32) m_num_sets; i++)
      delete m_sets[i];
   delete [] m_sets;
}

Lock&
Cache::getSetLock(IntPtr addr)
{
   IntPtr tag;
   UInt32 set_index;

   splitAddress(addr, tag, set_index);
   assert(set_index < m_num_sets);

   return m_sets[set_index]->getLock();
}

bool
Cache::invalidateSingleLine(IntPtr addr)
{
   IntPtr tag;
   UInt32 set_index;

   splitAddress(addr, tag, set_index);
   assert(set_index < m_num_sets);

   return m_sets[set_index]->invalidate(tag);
}

CacheBlockInfo* Cache::accessSingleLine(IntPtr addr, access_t access_type,
      Byte* buff, UInt32 bytes, SubsecondTime now, bool update_replacement) {
   IntPtr tag;
   UInt32 set_index;
   UInt32 line_index = -1;
   UInt32 block_offset;
   splitAddress(addr, tag, set_index, block_offset);

   CacheSet* set = m_sets[set_index];
   CacheBlockInfo* cache_block_info = set->find(tag, &line_index);

   if (cache_block_info == NULL){
      // on a miss
      for (UInt32 i = 0; i < m_associativity; ++i) {
        CacheBlockInfo* block = m_sets[set_index]->peekSingleLine(i);
        if (block && hasExpired(block, now)) {
            // Block has expired, update its predictor info in history table
            auto& entry = history_table[block->pc];
            entry.ref_count_threshold = std::max(entry.ref_count_threshold, block->ref_count);
            entry.confidence_bit = block->ref_count >= entry.ref_count_threshold;
        }
    }
      return NULL;
   }

   if (m_name == "L2") {
      auto& history_entry = history_table[cache_block_info->pc];
      
      cache_block_info->ref_count++;

      // if (cache_block_info->ref_count < history_entry.ref_count_threshold) {
      //    if (history_entry.filter_cnt == cache_block_info->ref_count) {
      //       history_entry.sat_cnt++;
      //    } else {
      //       history_entry.filter_cnt = cache_block_info->ref_count;
      //       history_entry.sat_cnt = 1;
      //    }

      //    if (history_entry.sat_cnt >= SATURATION_LIMIT) {
      //       history_entry.ref_count_threshold = history_entry.filter_cnt;
      //       history_entry.confidence_bit = true;
      //    } else {
      //       history_entry.confidence_bit = false;
      //    }
      // } else if (cache_block_info->ref_count == history_entry.ref_count_threshold) {
      //    history_entry.confidence_bit = true;
      // }
   }

   if (access_type == LOAD) {
      if (m_fault_injector)
         m_fault_injector->preRead(addr, set_index * m_associativity + line_index, bytes, 
                                   (Byte*)m_sets[set_index]->getDataPtr(line_index, block_offset), now);

      set->read_line(line_index, block_offset, buff, bytes, update_replacement);
   } else {
      set->write_line(line_index, block_offset, buff, bytes, update_replacement);

      if (m_fault_injector)
         m_fault_injector->postWrite(addr, set_index * m_associativity + line_index, bytes, 
                                     (Byte*)m_sets[set_index]->getDataPtr(line_index, block_offset), now);
   }

   return cache_block_info;
}

void Cache::insertSingleLine(IntPtr addr, Byte* fill_buff,
      bool* eviction, IntPtr* evict_addr,
      CacheBlockInfo* evict_block_info, Byte* evict_buff,
      SubsecondTime now, CacheCntlr *cntlr) {
   IntPtr tag;
   UInt32 set_index;
   splitAddress(addr, tag, set_index);

   CacheBlockInfo* cache_block_info = CacheBlockInfo::create(m_cache_type);
   cache_block_info->setTag(tag);

   if (m_name == "L2") {

      auto& history_entry = history_table[hashed_pc];
      
      cache_block_info->ref_count =0;
      cache_block_info->max_C_past = history_entry->ref_count_threshold;
      cache_block_info->conf = history_entry->conf;
      //                               ? history_entry.ref_count_threshold 
      //                               : 0;
   }

   m_sets[set_index]->insert(cache_block_info, fill_buff,
         eviction, evict_block_info, evict_buff, cntlr);

   if (*eviction && evict_block_info) {
      *evict_addr = tagToAddress(evict_block_info->getTag()); //y

      if (m_name == "L2") {
        // auto& entry = history_table[evict_block_info->getTag()];
         history_table[hashed_pc]->ref_count_threshold = evict_block_info->ref_count;
         if( evict_block_info->max_C_past ==  evict_block_info->ref_count){
            history_table[hashed_PC]->conf = true;
         }
         else {
            history_table[hashed_PC]->conf = false;
         }
         // int evict_ref_count = evict_block_info->ref_count;

         // if (evict_ref_count < entry.ref_count_threshold) {
         //    if (entry.filter_cnt == evict_ref_count) {
         //       entry.sat_cnt++;
         //    } else {
         //       entry.filter_cnt = evict_ref_count;
         //       entry.sat_cnt = 1;
         //    }

         //    if (entry.sat_cnt >= SATURATION_LIMIT) {
         //       entry.ref_count_threshold = entry.filter_cnt;
         //       entry.confidence_bit = true;

         //    } else {
         //       entry.confidence_bit = false; //isko dekhna hai 
         //    }
         // } else if (evict_ref_count == entry.ref_count_threshold) {
         //    entry.confidence_bit = true;
         // }
      }
   }

   delete cache_block_info;
}


// Single line cache access at addr
CacheBlockInfo*
Cache::peekSingleLine(IntPtr addr)
{
   IntPtr tag;
   UInt32 set_index;
   splitAddress(addr, tag, set_index);

   return m_sets[set_index]->find(tag);
}

void
Cache::updateCounters(bool cache_hit)
{
   if (m_enabled)
   {
      m_num_accesses ++;
      if (cache_hit)
         m_num_hits ++;
   }
}

void
Cache::updateHits(Core::mem_op_t mem_op_type, UInt64 hits)
{
   if (m_enabled)
   {
      m_num_accesses += hits;
      m_num_hits += hits;
   }
}


bool Cache::hasExpired(CacheBlockInfo* block, SubsecondTime now) {
    // Example condition: expiry based on ref_count and confidence bit
    auto& entry = history_table[block->getTag()];
    return block->ref_count < entry.ref_count_threshold || !entry.confidence_bit;
}

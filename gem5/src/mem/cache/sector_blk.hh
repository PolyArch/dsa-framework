/**
 * Copyright (c) 2018 Inria
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Authors: Daniel Carvalho
 */

/** @file
 * Definition of a simple sector block class. Each sector consists of a
 * sequence of cache blocks that may or may not be present in the cache.
 */

#ifndef __MEM_CACHE_SECTOR_BLK_HH__
#define __MEM_CACHE_SECTOR_BLK_HH__

#include <vector>

#include "mem/cache/blk.hh"
#include "mem/cache/replacement_policies/base.hh"

class SectorBlk;

/**
 * A sector is composed of sub-blocks, and each sub-block has information
 * regarding its sector and a pointer to its sector tag.
 */
class SectorSubBlk : public CacheBlk
{
  private:
    /**
     * Sector block associated to this block.
     */
    SectorBlk* _sectorBlk;

    /**
     * The offset of this sub-block in the sector.
     */
    int _sectorOffset;

  public:
    SectorSubBlk() : CacheBlk(), _sectorBlk(nullptr), _sectorOffset(0) {}
    SectorSubBlk(const SectorSubBlk&) = delete;
    SectorSubBlk& operator=(const SectorSubBlk&) = delete;
    ~SectorSubBlk() {};

    /**
     * Set sector block associated to this block.
     *
     * @param sector_blk The sector block pointer.
     */
    void setSectorBlock(SectorBlk* sector_blk);

    /**
     * Get sector block associated to this block.
     *
     * @return The sector block pointer.
     */
    const SectorBlk* getSectorBlock() const;

    /**
     * Set offset of this sub-block within the sector.
     *
     * @param sector_offset The block's offset.
     */
    void setSectorOffset(const int sector_offset);

    /**
     * Get offset of this sub-block within the sector.
     *
     * @return sector_offset The block's offset.
     */
    int getSectorOffset() const;

    /**
     * Get tag associated to this block.
     *
     * @return The tag value.
     */
    Addr getTag() const;

    /**
     * Set member variables when a block insertion occurs. Resets reference
     * count to 1 (the insertion counts as a reference), and touch block if
     * it hadn't been touched previously. Sets the insertion tick to the
     * current tick. Does not make block valid.
     *
     * @param tag Block address tag.
     * @param is_secure Whether the block is in secure space or not.
     * @param src_master_ID The source requestor ID.
     * @param task_ID The new task ID.
     */
    void insert(const Addr tag, const bool is_secure, const int src_master_ID,
                const uint32_t task_ID) override;
};

/**
 * A Basic Sector block.
 * Contains the tag and a list of blocks associated to this sector.
 */
class SectorBlk : public ReplaceableEntry
{
  private:
    /**
     * Sector tag value. A sector's tag is the tag of all its sub-blocks.
     */
    Addr _tag;

  public:
    SectorBlk() : ReplaceableEntry(), _tag(MaxAddr) {}
    SectorBlk(const SectorBlk&) = delete;
    SectorBlk& operator=(const SectorBlk&) = delete;
    ~SectorBlk() {};

    /** List of blocks associated to this sector. */
    std::vector<SectorSubBlk*> blks;

    /**
     * Checks that a sector block is valid.
     *
     * @return True if any of the blocks in the sector is valid.
     */
    bool isValid() const;

    /**
     * Checks that a sector block is secure. A single secure block suffices
     * to imply that the whole sector is secure, as the insertion proccess
     * asserts that different secure spaces can't coexist in the same sector.
     *
     * @return True if any of the blocks in the sector is secure.
     */
    bool isSecure() const;

    /**
     * Set tag associated to this block.
     *
     * @param The tag value.
     */
    void setTag(const Addr tag);

    /**
     * Get tag associated to this block.
     *
     * @return The tag value.
     */
    Addr getTag() const;
};

#endif //__MEM_CACHE_SECTOR_BLK_HH__

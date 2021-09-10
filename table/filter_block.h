// Copyright (c) 2012 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.
//
// A filter block is stored near the end of a Table file.  It contains
// filters (e.g., bloom filters) for all data blocks in the table combined
// into a single filter block.

#ifndef STORAGE_LEVELDB_TABLE_FILTER_BLOCK_H_
#define STORAGE_LEVELDB_TABLE_FILTER_BLOCK_H_

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "leveldb/slice.h"

#include "util/hash.h"

namespace leveldb {

class FilterPolicy;

// A FilterBlockBuilder is used to construct all of the filters for a
// particular Table.  It generates a single string which is stored as
// a special block in the Table.
//
// The sequence of calls to FilterBlockBuilder must match the regexp:
//      (StartBlock AddKey*)* Finish
class FilterBlockBuilder {
 public:
  explicit FilterBlockBuilder(const FilterPolicy*);

  FilterBlockBuilder(const FilterBlockBuilder&) = delete;
  FilterBlockBuilder& operator=(const FilterBlockBuilder&) = delete;

  void StartBlock(uint64_t block_offset);
  void AddKey(const Slice& key);
  Slice Finish();

 private:
  void GenerateFilter();

  const FilterPolicy* policy_;
  std::string keys_;  // Flattened key contents 所有key连续拼接成的字符串

  // Starting index in keys_ of each key
  // 顺次保存每个key在keys_中的偏移量,长度为keys_所存储的key个数，如key1:aa,key2:bbb,
  // keys_:aabbb,start_:[0,2], 其中[0~2)表示aa、[2,len(keys_))表示bbb的
  std::vector<size_t> start_;

  // Filter data computed so far 布隆过滤器数据集：内存结构为
  // |filter datas|一系列的4B
  // 偏移量位置(长度/4 代表filter_block的个数)...|4B:
  // filterdata的总长度，或者说是一些列4B的起始偏移量|1B:filterblock的固定大小的指数，默认是11，即2kb|
  std::string result_;

  // policy_->CreateFilter() argument //从keys_中切割
  // 顺次取出每个key，如：["aa", "bbb"]
  std::vector<Slice> tmp_keys_;

  //对应datablock的每2KB会追加一个元素，元素的值为filterblock的最新偏移量，第一个元素是0
  std::vector<uint32_t> filter_offsets_;
};

class FilterBlockReader {
 public:
  // REQUIRES: "contents" and *policy must stay live while *this is live.
  FilterBlockReader(const FilterPolicy* policy, const Slice& contents);

  //检测一个key是否存在于data_block当中
  // key: 被检测key
  // block_offset, data_block的偏移量，即block的起始地址
  // return true存在
  bool KeyMayMatch(uint64_t block_offset, const Slice& key);

 private:
  const FilterPolicy* policy_;
  const char* data_;  // Pointer to filter data (at block-start)
  const char*
      offset_;  // Pointer to beginning of offset array (at block-end)
                // block的尾部，filter_offsets的开始位置，每4B表示block所对应的偏移量
  size_t num_;  // Number of entries in offset array
  size_t base_lg_;  // Encoding parameter (see kFilterBaseLg in .cc file)
};

}  // namespace leveldb

#endif  // STORAGE_LEVELDB_TABLE_FILTER_BLOCK_H_

/*
 * Copyright (c) 2021. Kwai, Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *         http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Created by lbtrace on 2021.
 *
 */
#ifndef KOOM_NATIVE_OOM_SRC_MAIN_JNI_INCLUDE_UTILS_CONCURRENT_HASH_MAP_H_
#define KOOM_NATIVE_OOM_SRC_MAIN_JNI_INCLUDE_UTILS_CONCURRENT_HASH_MAP_H_

#include <map>
#include <mutex>
#include <vector>

template <typename K, typename V, typename Hash = std::hash<K>>
class ConcurrentHashMap {
 public:
  ConcurrentHashMap(unsigned bucketNumber = kDefaultBucketNum,
                    const Hash &hash = Hash())
      : table_(bucketNumber), hash_(hash) {}

  template <typename Predicate>
  void Dump(Predicate &p) {
    for (auto &bucket : table_) {
      bucket.Dump(p);
    }
  }

  void Insert(const K &key, V &&value) {
    table_[Hashcode(key)].Insert(key, std::move(value));
  }

  void Put(const K &key, V &&value) {
    table_[Hashcode(key)].Put(key, std::move(value));
  }

  void Erase(const K &key) { table_[Hashcode(key)].Erase(key); }

  std::size_t Size() const {
    std::size_t size = 0;
    for (auto &bucket : table_) {
      size += bucket.Size();
    }
    return size;
  }

  std::size_t Count(const K &key) const {
    return table_[Hashcode(key)].Count(key);
  }

  void Clear() {
    for (auto &bucket : table_) {
      bucket.Clear();
    }
  }

 private:
  static const unsigned kDefaultBucketNum = 521;  // Prime Number is better

  class Bucket {
   public:
    void Insert(const K &key, V &&value) {
      std::lock_guard<std::mutex> lock(mutex_);
      item_.emplace(key, std::move(value));
    }

    void Put(const K &key, V &&value) {
      std::lock_guard<std::mutex> lock(mutex_);
      item_.erase(key);
      item_.emplace(key, std::move(value));
    }

    void Erase(const K &key) {
      std::lock_guard<std::mutex> lock(mutex_);
      item_.erase(key);
    }

    template <typename Predicate>
    void Dump(Predicate &p) {
      std::lock_guard<std::mutex> lock(mutex_);
      for (auto it = item_.begin(); it != item_.end(); it++) {
        p(it->second);
      }
    }

    std::size_t Size() const {
      std::lock_guard<std::mutex> lock(mutex_);
      return item_.size();
    }

    std::size_t Count(const K &key) const {
      std::lock_guard<std::mutex> lock(mutex_);
      return item_.count(key);
    }

    void Clear() {
      std::lock_guard<std::mutex> lock(mutex_);
      item_.clear();
    }

   private:
    using Item = std::map<K, V>;
    Item item_;
    mutable std::mutex mutex_;
  };

  inline std::size_t Hashcode(const K &key) {
    return hash_(key) % table_.size();
  }

  std::vector<Bucket> table_;
  Hash hash_;
};
#endif  // KOOM_NATIVE_OOM_SRC_MAIN_JNI_INCLUDE_UTILS_CONCURRENT_HASH_MAP_H_
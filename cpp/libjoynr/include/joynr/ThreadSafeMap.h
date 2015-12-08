/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
 * %%
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *      http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * #L%
 */
#ifndef THREADSAFEMAP_H
#define THREADSAFEMAP_H
#include "joynr/PrivateCopyAssign.h"

#include <QReadWriteLock>
#include <map>

namespace joynr
{

template <class Key, class T>
class ThreadSafeMap
{
public:
    ThreadSafeMap();
    virtual ~ThreadSafeMap()
    {
    }
    void insert(const Key& key, const T& value);
    void remove(const Key& key);
    T value(const Key& key);
    T take(const Key& key);
    bool contains(const Key& key);
    void deleteAll();
    int size();

private:
    DISALLOW_COPY_AND_ASSIGN(ThreadSafeMap);
    std::map<Key, T> map;
    QReadWriteLock lock;
};

template <class Key, class T>
ThreadSafeMap<Key, T>::ThreadSafeMap()
        : map(), lock()
{
}

template <class Key, class T>
void ThreadSafeMap<Key, T>::insert(const Key& key, const T& value)
{
    lock.lockForWrite();
    map.insert(std::make_pair(key, value));
    lock.unlock();
}

template <class Key, class T>
void ThreadSafeMap<Key, T>::remove(const Key& key)
{
    lock.lockForWrite();
    map.erase(map.find(key));
    lock.unlock();
}

template <class Key, class T>
T ThreadSafeMap<Key, T>::value(const Key& key)
{
    T aValue;
    lock.lockForRead();
    aValue = map.find(key)->second;
    lock.unlock();
    return aValue;
}

template <class Key, class T>
T ThreadSafeMap<Key, T>::take(const Key& key)
{
    T aValue;
    lock.lockForWrite();
    auto mapElement = map.find(key);
    if (mapElement != map.end()) {
        aValue = mapElement->second;
        map.erase(mapElement);
    }
    lock.unlock();
    return aValue;
}

template <class Key, class T>
bool ThreadSafeMap<Key, T>::contains(const Key& key)
{
    bool aValue;
    lock.lockForRead();
    aValue = map.find(key) != map.end();
    lock.unlock();
    return aValue;
}

template <class Key, class T>
void ThreadSafeMap<Key, T>::deleteAll()
{
    lock.lockForWrite();
    for (const Key& str : map.keys()) {
        T value = map.take(str);
        delete value;
        value = NULL;
    }
    lock.unlock();
}

template <class Key, class T>
int ThreadSafeMap<Key, T>::size()
{
    return map.size();
}

} // namespace joynr

#endif // THREADSAFEMAP_H

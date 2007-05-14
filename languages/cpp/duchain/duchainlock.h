/* This file is part of KDevelop
    Copyright (C) 2007 Kris Wong <kwong@fuse.net>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef DUCHAINLOCK_H
#define DUCHAINLOCK_H

#include "duchainexport.h"

/**
 * In a DEBUG build, keeps track of additional locking information.
 * In a non-DEBUG build, is actually a QReadWriteLock.
 */
#ifdef NDEBUG

#include <QReadWriteLock>

#define ENSURE_CHAIN_READ_LOCKED
#define ENSURE_CHAIN_WRITE_LOCKED

class DUCHAIN_EXPORT DUChainLock : public QReadWriteLock
{
public:
  DUChainLock() : QReadWriteLock() {;}
  ~DUChainLock() {;}
};

class DUCHAIN_EXPORT DUChainReadLocker : public QReadLocker
{
public:
  DUChainReadLocker(DUChainLock* duChainLock) : QReadLocker(duChainLock) {;}
  ~DUChainReadLocker() {;}
};

class DUCHAIN_EXPORT DUChainWriteLocker : public QWriteLocker
{
public:
  DUChainWriteLocker(DUChainLock* duChainLock) : QWriteLocker(duChainLock) {;}
  ~DUChainWriteLocker() {;}
};

#else

#include <QMutex>
#include <QSet>

/**
 * Macros for ensuring the DUChain is locked properly.
 *
 * These should be used in every method that accesses or modifies a
 * member on the DUChain or one of its contexts.
 */
#define ENSURE_CHAIN_READ_LOCKED Q_ASSERT(DUChain::lock()->currentThreadHasReadLock() || DUChain::lock()->currentThreadHasWriteLock());
#define ENSURE_CHAIN_WRITE_LOCKED Q_ASSERT(DUChain::lock()->currentThreadHasWriteLock());

class DUCHAIN_EXPORT DUChainLock
{
public:
  DUChainLock();
  ~DUChainLock();

  /**
   * Acquires a read lock. Will not return until the lock is aquired
   * or timeout is reached (10 seconds).
   *
   * Any number of read locks can be aquired at once, but not while
   * there is a write lock.  Read locks can be recursive.
   */
  bool lockForRead();

  /**
   * Releases a previously acquired read lock.
   */
  void releaseReadLock();

  /**
   * Determines if the current thread has a read lock.
   */
  bool currentThreadHasReadLock();

  /**
   * Acquires a write lock. Will not return until the lock is aquired
   * or timeout is reached (10 seconds).
   *
   * There can be no other read or write locks held at the same time.
   * Write locks cannot be recursive.
   */
  bool lockForWrite();

  /**
   * Releases a previously acquired write lock.
   */
  void releaseWriteLock();

  /**
   * Determines if the current thread has a write lock.
   */
  bool currentThreadHasWriteLock();

private:
  QMutex m_mutex;
  Qt::HANDLE m_writer;
  QSet<Qt::HANDLE> m_readers;
};

class DUCHAIN_EXPORT DUChainReadLocker
{
public:
  DUChainReadLocker(DUChainLock* duChainLock) : m_lock(duChainLock) { lock(); }
  ~DUChainReadLocker() { unlock(); }

  bool lock() {
    bool l = false;
    if (m_lock) {
      l = m_lock->lockForRead();
      Q_ASSERT(l);
    };
    return l;
  }

  void unlock() {
    if (m_lock) {
      m_lock->releaseReadLock();
    }
  }

private:
  DUChainLock* m_lock;
};

class DUCHAIN_EXPORT DUChainWriteLocker
{
public:
  DUChainWriteLocker(DUChainLock* duChainLock) : m_lock(duChainLock) { lock(); }
  ~DUChainWriteLocker() { unlock(); }

  bool lock() {
    bool l = false;
    if (m_lock) {
      l = m_lock->lockForWrite();
      Q_ASSERT(l);
    };
    return l;
  }

  void unlock() {
    if (m_lock) {
      m_lock->releaseWriteLock();
    }
  }

private:
  DUChainLock* m_lock;
};

#endif // NDEBUG

#endif // DUCHAINLOCK_H

// kate: indent-width 2;

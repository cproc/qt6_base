// Copyright (C) 2013 David Faure <faure+bluesystems@kde.org>
// Copyright (C) 2017 Intel Corporation.
// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "private/qlockfile_p.h"

#include "QtCore/qtemporaryfile.h"
#include "QtCore/qfileinfo.h"
#include "QtCore/qdebug.h"
#include "QtCore/qdatetime.h"
#include "QtCore/qfileinfo.h"
#include "QtCore/qcache.h"
#include "QtCore/qglobalstatic.h"
#include "QtCore/qmutex.h"

#include "private/qcore_unix_p.h" // qt_safe_open
#include "private/qabstractfileengine_p.h"
#include "private/qfilesystementry_p.h"
#include "private/qtemporaryfile_p.h"

#if !defined(Q_OS_INTEGRITY)
#include <sys/file.h>  // flock
#endif

#if defined(Q_OS_RTEMS) || defined(Q_OS_GENODE)
// flock() does not work in these OSes and produce warnings when we try to use
#  undef LOCK_EX
#  undef LOCK_NB
#endif

#include <sys/types.h> // kill
#include <signal.h>    // kill
#include <unistd.h>    // gethostname

#if defined(Q_OS_MACOS)
#   include <libproc.h>
#elif defined(Q_OS_LINUX)
#   include <unistd.h>
#   include <cstdio>
#elif defined(Q_OS_HAIKU)
#   include <kernel/OS.h>
#elif defined(Q_OS_BSD4) && !defined(QT_PLATFORM_UIKIT)
#   include <sys/cdefs.h>
#   include <sys/param.h>
#   include <sys/sysctl.h>
# if !defined(Q_OS_NETBSD) && 0
#   include <sys/user.h>
# endif
#endif

QT_BEGIN_NAMESPACE

// ### merge into qt_safe_write?
static qint64 qt_write_loop(int fd, const char *data, qint64 len)
{
    qint64 pos = 0;
    while (pos < len) {
        const qint64 ret = qt_safe_write(fd, data + pos, len - pos);
        if (ret == -1) // e.g. partition full
            return pos;
        pos += ret;
    }
    return pos;
}

/*
 * Details about file locking on Unix.
 *
 * There are three types of advisory locks on Unix systems:
 *  1) POSIX process-wide locks using fcntl(F_SETLK)
 *  2) BSD flock(2) system call
 *  3) Linux-specific file descriptor locks using fcntl(F_OFD_SETLK)
 * There's also a mandatory locking feature by POSIX, which is deprecated on
 * Linux and users are advised not to use it.
 *
 * The first problem is that the POSIX API is braindead. POSIX.1-2008 says:
 *
 *   All locks associated with a file for a given process shall be removed when
 *   a file descriptor for that file is closed by that process or the process
 *   holding that file descriptor terminates.
 *
 * The Linux manpage is clearer:
 *
 *  * If a process closes _any_ file descriptor referring to a file, then all
 *    of the process's locks on that file are released, regardless of the file
 *    descriptor(s) on which the locks were obtained. This is bad: [...]
 *
 *  * The threads in a process share locks. In other words, a multithreaded
 *    program can't use record locking to ensure that threads don't
 *    simultaneously access the same region of a file.
 *
 * So in order to use POSIX locks, we'd need a global mutex that stays locked
 * while the QLockFile is locked. For that reason, Qt does not use POSIX
 * advisory locks anymore.
 *
 * The next problem is that POSIX leaves undefined the relationship between
 * locks with fcntl(), flock() and lockf(). In some systems (like the BSDs),
 * all three use the same record set, while on others (like Linux) the locks
 * are independent, except if locking over NFS mounts, in which case they're
 * actually the same. Therefore, it's a very bad idea to mix them in the same
 * process.
 *
 * We therefore use only flock(2).
 */

static bool setNativeLocks(int fd)
{
#if defined(LOCK_EX) && defined(LOCK_NB)
    if (flock(fd, LOCK_EX | LOCK_NB) == -1) // other threads, and other processes on a local fs
        return false;
#else
    Q_UNUSED(fd);
#endif
    return true;
}

QLockFile::LockError QLockFilePrivate::tryLock_sys()
{
    const QByteArray lockFileName = QFile::encodeName(fileName);
    const int fd = qt_safe_open(lockFileName.constData(), O_RDWR | O_CREAT | O_EXCL, 0666);
    if (fd < 0) {
        switch (errno) {
        case EEXIST:
            return QLockFile::LockFailedError;
        case EACCES:
        case EROFS:
            return QLockFile::PermissionError;
        default:
            return QLockFile::UnknownError;
        }
    }
    // Ensure nobody else can delete the file while we have it
    if (!setNativeLocks(fd)) {
        const int errnoSaved = errno;
        qWarning() << "setNativeLocks failed:" << qt_error_string(errnoSaved);
    }

    QByteArray fileData = lockFileContents();
    if (qt_write_loop(fd, fileData.constData(), fileData.size()) < fileData.size()) {
        qt_safe_close(fd);
        if (!QFile::remove(fileName))
            qWarning("QLockFile: Could not remove our own lock file %ls.", qUtf16Printable(fileName));
        return QLockFile::UnknownError; // partition full
    }

    // We hold the lock, continue.
    fileHandle = fd;

    // Sync to disk if possible. Ignore errors (e.g. not supported).
#if defined(_POSIX_SYNCHRONIZED_IO) && _POSIX_SYNCHRONIZED_IO > 0
    fdatasync(fileHandle);
#else
    fsync(fileHandle);
#endif

    return QLockFile::NoError;
}

bool QLockFilePrivate::removeStaleLock()
{
    const QByteArray lockFileName = QFile::encodeName(fileName);
    const int fd = qt_safe_open(lockFileName.constData(), O_WRONLY, 0666);
    if (fd < 0) // gone already?
        return false;
    bool success = setNativeLocks(fd) && (::unlink(lockFileName) == 0);
    close(fd);
    return success;
}

bool QLockFilePrivate::isProcessRunning(qint64 pid, const QString &appname)
{
    if (::kill(pid_t(pid), 0) == -1 && errno == ESRCH)
        return false; // PID doesn't exist anymore

    const QString processName = processNameByPid(pid);
    if (!processName.isEmpty()) {
        QFileInfo fi(appname);
        if (fi.isSymLink())
            fi.setFile(fi.symLinkTarget());
        if (processName != fi.fileName())
            return false;   // PID got reused by a different application.
    }

    return true;
}

QString QLockFilePrivate::processNameByPid(qint64 pid)
{
#if defined(Q_OS_MACOS)
    char name[1024];
    proc_name(pid, name, sizeof(name) / sizeof(char));
    return QFile::decodeName(name);
#elif defined(Q_OS_LINUX)
    if (!qt_haveLinuxProcfs())
        return QString();

    char exePath[64];
    sprintf(exePath, "/proc/%lld/exe", pid);

    QByteArray buf = qt_readlink(exePath);
    if (buf.isEmpty()) {
        // The pid is gone. Return some invalid process name to fail the test.
        return QStringLiteral("/ERROR/");
    }

    // remove the " (deleted)" suffix, if any
    static const char deleted[] = " (deleted)";
    if (buf.endsWith(deleted))
        buf.chop(strlen(deleted));

    return QFileSystemEntry(buf, QFileSystemEntry::FromNativePath()).fileName();
#elif defined(Q_OS_HAIKU)
    thread_info info;
    if (get_thread_info(pid, &info) != B_OK)
        return QString();
    return QFile::decodeName(info.name);
#elif defined(Q_OS_BSD4) && !defined(QT_PLATFORM_UIKIT) && 0
# if defined(Q_OS_NETBSD)
    struct kinfo_proc2 kp;
    int mib[6] = { CTL_KERN, KERN_PROC2, KERN_PROC_PID, (int)pid, sizeof(struct kinfo_proc2), 1 };
# elif defined(Q_OS_OPENBSD)
    struct kinfo_proc kp;
    int mib[6] = { CTL_KERN, KERN_PROC, KERN_PROC_PID, (int)pid, sizeof(struct kinfo_proc), 1 };
# else
    struct kinfo_proc kp;
    int mib[4] = { CTL_KERN, KERN_PROC, KERN_PROC_PID, (int)pid };
# endif
    size_t len = sizeof(kp);
    u_int mib_len = sizeof(mib)/sizeof(u_int);

    if (sysctl(mib, mib_len, &kp, &len, NULL, 0) < 0)
        return QString();

# if defined(Q_OS_OPENBSD) || defined(Q_OS_NETBSD)
    if (kp.p_pid != pid)
        return QString();
    QString name = QFile::decodeName(kp.p_comm);
# else
    if (kp.ki_pid != pid)
        return QString();
    QString name = QFile::decodeName(kp.ki_comm);
# endif
    return name;
#elif defined(Q_OS_QNX)
    char exePath[PATH_MAX];
    sprintf(exePath, "/proc/%lld/exefile", pid);

    int fd = qt_safe_open(exePath, O_RDONLY);
    if (fd == -1)
        return QString();

    QT_STATBUF sbuf;
    if (QT_FSTAT(fd, &sbuf) == -1) {
        qt_safe_close(fd);
        return QString();
    }

    QByteArray buffer(sbuf.st_size, Qt::Uninitialized);
    buffer.resize(qt_safe_read(fd, buffer.data(), sbuf.st_size - 1));
    if (buffer.isEmpty()) {
        // The pid is gone. Return some invalid process name to fail the test.
        return QStringLiteral("/ERROR/");
    }
    return QFileSystemEntry(buffer, QFileSystemEntry::FromNativePath()).fileName();
#else
    Q_UNUSED(pid);
    return QString();
#endif
}

void QLockFile::unlock()
{
    Q_D(QLockFile);
    if (!d->isLocked)
        return;
    close(d->fileHandle);
    d->fileHandle = -1;
    if (!QFile::remove(d->fileName)) {
        qWarning() << "Could not remove our own lock file" << d->fileName << "maybe permissions changed meanwhile?";
        // This is bad because other users of this lock file will now have to wait for the stale-lock-timeout...
    }
    d->lockError = QLockFile::NoError;
    d->isLocked = false;
}

QT_END_NAMESPACE

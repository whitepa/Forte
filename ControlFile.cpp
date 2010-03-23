// ControlFile.cpp
// #include "ControlFile.h"
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// macros
#define CONTROL_FILE_VERSION    2
#define CF_STATUS_QUEUED        0x00000000
#define CF_STATUS_DONE          0xFFFFFFFF

#define CF_READ_LOCK_HEADER FileSystem::AdvisoryAutoUnlock lock(m_fd, sizeof(header_t), sizeof(Header), false)
#define CF_WRITE_LOCK_HEADER FileSystem::AdvisoryAutoUnlock lock(m_fd, sizeof(header_t), sizeof(Header), true)


// statics
template < typename Header, typename Record >
const unsigned ControlFile<Header, Record>::s_lock_timeout = 60;

// CReplicationControlFile definition
template < typename Header, typename Record >
bool ControlFile<Header, Record>::exists() const
{
    struct stat st;
    bool ret = (FileSystem::get()->stat(m_filename, &st) == 0);
    hlog(HLOG_DEBUG2, "ControlFile::%s() = %s", __FUNCTION__,
         (ret ? "true" : "false"));
    return ret;
}


template < typename Header, typename Record >
void ControlFile<Header, Record>::createEmpty()
{
    hlog(HLOG_DEBUG2, "ControlFile::%s()", __FUNCTION__);
    header_t header;
    FString stmp;
    AutoFD fd;

    // delete any old one
    FileSystem::get()->unlink(m_filename);

    // open control file for read-write
    if ((m_fd = ::open(m_filename, O_RDWR|O_CREAT, 0600)) == -1)
        throw ForteControlFileException(FStringFC(), "Failed to create control file '%s': %s",
                                         m_filename.c_str(), FileSystem::get()->strerror(errno).c_str());

    // write header
    lseek64(m_fd, 0, SEEK_SET);
    header.version = CONTROL_FILE_VERSION;
    header.header_size = sizeof(header_t);
    header.user_header_size = sizeof(Header);
    header.record_size = sizeof(Record);
    header.next_available = 0;
    header.n_total = 0;
    header.n_claimed = 0;
    header.n_remaining = 0;
    header.last_progress_time = time(NULL);

    if (write(m_fd, &header, sizeof(header)) != sizeof(header))
        throw ForteControlFileException(FStringFC(), "Failed to write control file header to '%s': %s",
                                         m_filename.c_str(), FileSystem::get()->strerror(errno).c_str());
    fsync(m_fd);
}

template < typename Header, typename Record >
void ControlFile<Header, Record>::unlink()
{
    hlog(HLOG_DEBUG2, "ControlFile::%s()", __FUNCTION__);
    FileSystem::get()->unlink(m_filename);
}

template < typename Header, typename Record >
void ControlFile<Header, Record>::setHeader(const Header &userHeader)
{
    checkOpen();
    // update the user header
    {
        FileSystem::AdvisoryAutoUnlock lock(m_fd, sizeof(header_t), sizeof(Header), true);
        lseek64(m_fd, sizeof(header_t), SEEK_SET); // seek past our header
        if (write(m_fd, &userHeader, sizeof(Header)) != sizeof(Header))
            throw ForteControlFileException(FStringFC(), "failed to write user header (%lu bytes) to control file '%s'",
                                             sizeof(Header), m_filename.c_str());
        fdatasync(m_fd);
    }
}
template < typename Header, typename Record >
void ControlFile<Header, Record>::getHeader(Header &userHeader)
{
    checkOpen();
    {
        FileSystem::AdvisoryAutoUnlock lock(m_fd, sizeof(header_t), sizeof(Header), false);
        lseek64(m_fd, sizeof(header_t), SEEK_SET); // seek past our header
        if (::read(m_fd, &userHeader, sizeof(Header)) != sizeof(Header))
            throw ForteControlFileException(FStringFC(), "failed to read user header (%lu bytes) to control file '%s'",
                                             sizeof(Header), m_filename.c_str());
    }    
}

template < typename Header, typename Record >
uint64_t ControlFile<Header, Record>::enqueue(const Record &r)
{
    hlog(HLOG_DEBUG2, "ControlFile::%s([record])", __FUNCTION__);
    off64_t orig_eof = 0;

    checkOpen();

    // update the header
    {
        //FileSystem::AdvisoryAutoUnlock lock(m_fd, 0, sizeof(header_t), true);
        CF_WRITE_LOCK_HEADER;
        
        header_t header;
        readHeader(header);
        
        orig_eof = lseek64(m_fd, 0, SEEK_END);
        if (header.n_total == 0)
            header.next_available = orig_eof;

        ++header.n_total;
        ++header.n_remaining;
        
        // enqueue the record
        FileSystem::AdvisoryAutoUnlock recordLock(m_fd, 0, 0, true, SEEK_END); // lock after end of file

        record_t record;
        memcpy(&record.record, &r, sizeof(Record));
        record.status = CF_STATUS_QUEUED;

        if (write(m_fd, &record, sizeof(record_t)) != sizeof(record_t))
        {
            ftruncate(m_fd, orig_eof); // undo everything
            throw ForteControlFileException(FStringFC(), "failed to write record to control file '%s'",
                                             m_filename.c_str());
        }
        writeHeader(header);
    
        fdatasync(m_fd);
    }
    return orig_eof;
}

template < typename Header, typename Record >
void ControlFile<Header, Record>::checkOpen(void)
{
    if (m_fd != AutoFD::NONE)
        // already open
        return;

    if ((m_fd = ::open(m_filename, O_RDWR|O_NONBLOCK, 0600)) == -1)
        throw ForteControlFileException(FStringFC(), "failed to open control file '%s': %s",
                                         m_filename.c_str(), FileSystem::get()->strerror(errno).c_str());

    // read header
    lseek64(m_fd, 0, SEEK_SET);
    header_t header;
    size_t bytesRead;
    if ((bytesRead = ::read(m_fd, &header, sizeof(header))) != sizeof(header))
        throw ForteControlFileException(FStringFC(), "Failed to read control file header to '%s': %s, "
                                         "expected %llu bytes, got %llu",
                                         m_filename.c_str(), 
                                         FileSystem::get()->strerror(errno).c_str(),
                                         (u64) sizeof(header),
                                         (u64) bytesRead);

    // get and validate record/header size
    if (header.header_size != sizeof(header_t) || 
        header.user_header_size != sizeof(Header) ||
        header.record_size != sizeof(Record))
        throw ForteControlFileException(FStringFC(), "Invalid control file '%s'", m_filename.c_str());        
}


template < typename Header, typename Record >
bool ControlFile<Header, Record>::claim(off64_t &offset/*OUT*/, Record &record/*OUT*/)
{
    hlog(HLOG_DEBUG3, "ControlFile::%s()", __FUNCTION__);

    //struct in_addr addr;
//     off64_t offset;
//     FString stmp;

    checkOpen();

    {
        CF_WRITE_LOCK_HEADER;
        header_t header;
        readHeader(header);

        // all done?
        if (header.n_remaining == 0) return false;

        // read next available record
        offset = header.next_available;
        lseek64(m_fd, offset, SEEK_SET);

        record_t wrapper;
        if (::read(m_fd, &wrapper, sizeof(wrapper)) != sizeof(wrapper))
            throw ForteControlFileException(FStringFC(), "failed to read next available record in control file '%s': %s",
                                             m_filename.c_str(), FileSystem::get()->strerror(errno).c_str());

        // verify status
        if (wrapper.status != CF_STATUS_QUEUED)
            throw ForteControlFileException(FStringFC(), "control file '%s' in invalid state. "
                                             "expected CF_STATUS_QUEUED but got %u",
                                             m_filename.c_str(), wrapper.status);

        // copy record to output param
        memcpy(&record, &wrapper.record, sizeof(record));

//    inet_aton(NetUtil::get()->getMyPrivateIP(), &addr); TODO
        //wrapper.status = addr.s_addr;
        wrapper.status = 1;
        
        // re-write record to claim it
        lseek64(m_fd, offset, SEEK_SET);

        if (write(m_fd, &wrapper, sizeof(wrapper)) != sizeof(wrapper))
        {
            throw ForteControlFileException(FStringFC(), "failed to re-write record in control file '%s': %s",
                                             m_filename.c_str(), FileSystem::get()->strerror(errno).c_str());
        }

        // update header
        header.n_claimed++;
        header.n_remaining--;
        
        // need to find the next unclaimed record?
        if (header.n_remaining > 0)
        {
            off64_t nextOffset = offset + sizeof(wrapper);
            size_t bytesRead;
            while ((bytesRead = ::read(m_fd, &wrapper, sizeof(wrapper))) == sizeof(wrapper))
            {
                hlog(HLOG_DEBUG4, "got status %u for offset %llu", wrapper.status, nextOffset);
                if (wrapper.status == CF_STATUS_QUEUED)
                {
                    header.next_available = nextOffset;
                    break;
                }
                nextOffset += sizeof(wrapper);
            }
            if (bytesRead == 0)
            {
                throw ForteControlFileException("should have found a next record");                
            }
        }

        // rewrite the updated header
        writeHeader(header);
    }
    hlog(HLOG_DEBUG3, "returning offset %llu from claim", offset);
    // done
    return true;
}

template < typename Header, typename Record >
void ControlFile<Header, Record>::unclaim(off64_t offset)
{
    hlog(HLOG_DEBUG3, "ControlFile::%s(%llu)", __FUNCTION__, offset);

    struct in_addr addr;
//     off64_t offset;
//     FString stmp;

    checkOpen();
    {
        CF_WRITE_LOCK_HEADER;
        header_t header;
        readHeader(header);

        // seek to the offset
        lseek(m_fd, offset, SEEK_SET);
        record_t wrapper;
        if (::read(m_fd, &wrapper, sizeof(wrapper)) != sizeof(wrapper))
            throw ForteControlFileException(FStringFC(), "failed to read next available record in control file '%s': %s",
                                             m_filename.c_str(), FileSystem::get()->strerror(errno).c_str());

        // verify status
        // make sure we have it claimed
        if (wrapper.status != 1) //TODO: change status to verify ip
            throw ForteControlFileException(FStringFC(), "control file record '%s' is not claimed",
                                             m_filename.c_str());

//    inet_aton(NetUtil::get()->getMyPrivateIP(), &addr); TODO
        
        // re-write record to unclaim it
        lseek64(m_fd, offset, SEEK_SET);
        wrapper.status = CF_STATUS_QUEUED;
        if (write(m_fd, &wrapper, sizeof(wrapper)) != sizeof(wrapper))
        {
            throw ForteControlFileException("failed to re-write record in control file '%s': %s",
                                             m_filename.c_str(), FileSystem::get()->strerror(errno).c_str());
        }

        // update header
        header.n_claimed--;
        header.n_remaining++;

        // update the next_available
        if (header.next_available > offset || header.n_remaining == 1)
            header.next_available = offset;

        // rewrite the updated header
        writeHeader(header);
    }
    // done
    return true;
}

template < typename Header, typename Record >
void ControlFile<Header, Record>::complete(off64_t offset, const Record &r)
{
    hlog(HLOG_DEBUG3, "ControlFile::%s(%llu)", __FUNCTION__, offset);

    //struct in_addr addr;
//     off64_t offset;
//     FString stmp;

    checkOpen();
    {
        CF_WRITE_LOCK_HEADER;
        header_t header;
        readHeader(header);

        // seek to the offset
        lseek(m_fd, offset, SEEK_SET);
        record_t wrapper;
        if (::read(m_fd, &wrapper, sizeof(wrapper)) != sizeof(wrapper))
            throw ForteControlFileException(FStringFC(), "failed to read next available record in control file '%s': %s",
                                             m_filename.c_str(), FileSystem::get()->strerror(errno).c_str());

        // verify status
        // make sure we have it claimed
        //if (wrapper.status != addr.s_addr) //TODO: update status to ip
        if (wrapper.status != 1)
            throw ForteControlFileException(FStringFC(), 
                                             "control file record at offset %llu returned"
                                             " status %u instead of claimed",
                                             (unsigned long long) offset, wrapper.status);

//    inet_aton(NetUtil::get()->getMyPrivateIP(), &addr); TODO

        // copy record into wrapper
        memcpy(&wrapper.record, &r, sizeof(Record));
        
        // re-write record to complete it
        lseek64(m_fd, offset, SEEK_SET);
        wrapper.status = CF_STATUS_DONE;
        if (write(m_fd, &wrapper, sizeof(wrapper)) != sizeof(wrapper))
        {
            throw ForteControlFileException(FStringFC(), "failed to re-write record in control file '%s': %s",
                                             m_filename.c_str(), FileSystem::get()->strerror(errno).c_str());
        }

        // update header
        header.n_claimed--;

        // rewrite the updated header
        writeHeader(header);
    }
    // done
}
template < typename Header, typename Record >
void ControlFile<Header, Record>::read(uint64_t recnum, 
                                        Record &r /*OUT*/, 
                                        unsigned int &status /*OUT*/)
{
    hlog(HLOG_DEBUG3, "ControlFile::%s()", __FUNCTION__);

    checkOpen();
    {
        CF_READ_LOCK_HEADER;
        header_t header;
        readHeader(header);

        // verify recnum is feasible
        if (recnum >= header.n_total)
            throw ForteControlFileException(FStringFC(), "recnum %llu is out of range (max is %llu)",
                                             (unsigned long long) recnum, 
                                             (unsigned long long) header.n_total - 1);

        off64_t offset = header.header_size + header.user_header_size + recnum * sizeof(record_t);
        // seek to the offset1
        lseek(m_fd, offset, SEEK_SET);
        record_t wrapper;
        if (::read(m_fd, &wrapper, sizeof(wrapper)) != sizeof(wrapper))
            throw ForteControlFileException(FStringFC(), 
                                             "failed to read record number %llu in control file '%s': %s",
                                             (unsigned long long) recnum, 
                                             m_filename.c_str(), 
                                             FileSystem::get()->strerror(errno).c_str());

//    inet_aton(NetUtil::get()->getMyPrivateIP(), &addr); TODO

        // set up outputs
        memcpy(&r, &wrapper.record, sizeof(Record));
        status = wrapper.status;
    }
}

template < typename Header, typename Record >
void ControlFile<Header, Record>::getProgress(uint64_t& complete /*OUT*/,
                                               uint64_t& total    /*OUT*/,
                                               uint64_t& claimed  /*OUT*/,
                                               time_t& last_progress_time /*OUT*/,
                                               time_t update_if_older_than /*IN*/)
{
    checkOpen();
    {
        CF_WRITE_LOCK_HEADER;
        header_t header;
        readHeader(header);
        // set outputs
        complete = header.n_total - (header.n_claimed + header.n_remaining);
        total = header.n_total;
        claimed = header.n_claimed;
        if (header.last_progress_time < update_if_older_than)
        {
            // update the last update time
            header.last_progress_time = time(0);
            writeHeader(header);
        }
    }
}

template < typename Header, typename Record >
void ControlFile<Header, Record>::readHeader(header_t &header)
{
    // locking must be performed by the caller
    lseek64(m_fd, 0, SEEK_SET);
    if (::read(m_fd, &header, sizeof(header_t)) != sizeof(header_t))
        throw ForteControlFileException(FStringFC(), "failed to read header (%lu bytes) from control file '%s'",
                                         sizeof(header_t), m_filename.c_str());
}

template < typename Header, typename Record >
void ControlFile<Header, Record>::writeHeader(const header_t &header)
{
    // locking must be performed by the caller
    lseek64(m_fd, 0, SEEK_SET); // seek to 0, rewrite header
    if (write(m_fd, &header, sizeof(header_t)) != sizeof(header_t))
        throw ForteControlFileException(FStringFC(), "failed to write header (%lu bytes) to control file '%s'",
                                         sizeof(header_t), m_filename.c_str());
    fdatasync(m_fd);
}

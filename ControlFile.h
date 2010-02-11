#ifndef __forte_ControlFile_h
#define __forte_ControlFile_h

#ifndef _LARGEFILE64_SOURCE
#define _LARGEFILE64_SOURCE
#endif

#include "Forte.h"
// CControlFile is a template class which allows creation and
// management of a generic 'control file' for parallelizing a large
// operation into much smaller work units.

// This template supports a single user-defined header type, as well
// as a user-defined record type.

// POSIX advisory locking is used for concurrency.  A CControlFile
// object should only be used from a single thread.  Multiple
// CControlFile objects may be used on the same file to allow
// multithreading.

// Work units are added to the control file with the enqueue() method.
// A thread may claim a work unit with the claim() method.

EXCEPTION_SUBCLASS(CForteException, CForteControlFileException);

template < typename Header, typename Record >
class CControlFile
{
public:
    // constants
    static const unsigned s_lock_timeout;
    static const size_t s_max_cache_size;

    typedef struct {
        uint32_t version;
        uint16_t header_size;
        uint16_t user_header_size;
        uint16_t record_size;
        off64_t next_available;
        uint64_t n_total;
        uint64_t n_claimed;
        uint64_t n_remaining;
        time_t last_progress_time;
    } header_t;
    typedef struct record_t {
        Record record;
        unsigned int status;
    };

public:
    // ctor/dtor
    CControlFile(const FString& filename) : m_filename(filename) { }
    virtual ~CControlFile() { }

    // interface
    bool exists() const;                        // checks for file existence
    void createEmpty();                         // always creates a new file with just a header
    void unlink();                              // removes any file which exists

    void setHeader(const Header &header);
    void getHeader(Header &header /* OUT */);

    /// enqueue a new record, returns the record ID
    ///
    uint64_t enqueue(const Record& r);

    /// claim the next available record.
    ///
    bool claim(off64_t &offset/*OUT*/, Record &r /*OUT*/);
    
    /// unclaim a claimed record, does not update the record. This record
    /// will then be available for future 'claim' operations.
    ///
    void unclaim(off64_t offset);

    /// complete and update a record
    ///
    void complete(off64_t offset, const Record &r);

    /// read a record #
    ///
    void read(uint64_t recnum, 
              Record &r /*OUT*/,
              unsigned int &status /*OUT*/);

    /// update a record, will not be marked complete
    ///
//    void update(off64_t offset, Record &r);

    /// get the current progress on the batch.  If last progress time is older than
    /// update_if_older_than, it will be updated.
    void getProgress(uint64_t& complete /*OUT*/,
                     uint64_t& total    /*OUT*/,
                     uint64_t& claimed  /*OUT*/,
                     time_t& last_progress_time /*OUT*/,
                     time_t update_if_older_than /*IN*/);
    
    // accessors
    inline FString getFilename() const { return m_filename; }
    
private:
    // helpers
    void checkOpen(void);
    void readHeader(header_t &header);
    void writeHeader(const header_t &header);

protected:
    // data members
    FString m_filename;
    AutoFD m_fd;
};

#include "ControlFile.cpp"

#endif

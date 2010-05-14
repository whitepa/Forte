#ifndef __forte_ControlFile_h
#define __forte_ControlFile_h

#ifndef _LARGEFILE64_SOURCE
#define _LARGEFILE64_SOURCE
#endif

#include "Forte.h"
// ControlFile is a template class which allows creation and
// management of a generic 'control file' for parallelizing a large
// operation into much smaller work units.

// This template supports a single user-defined header type, as well
// as a user-defined record type.

// POSIX advisory locking is used for concurrency.  A ControlFile
// object should only be used from a single thread.  Multiple
// ControlFile objects may be used on the same file to allow
// multithreading.

// Work units are added to the control file with the enqueue() method.
// A thread may claim a work unit with the claim() method.

EXCEPTION_SUBCLASS(Exception, ForteControlFileException);

template < typename Header, typename Record >
class ControlFile
{
public:
    // constants
    static const unsigned sLockTimeout;
    static const size_t sMaxCacheSize;

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
    ControlFile(const FString& filename) : mFilename(filename) { }
    virtual ~ControlFile() { }

    // interface
    bool Exists() const;                        // checks for file existence
    void CreateEmpty();                         // always creates a new file with just a header
    void Unlink();                              // removes any file which exists

    void SetHeader(const Header &header);
    void GetHeader(Header &header /* OUT */);

    /// enqueue a new record, returns the record ID
    ///
    uint64_t Enqueue(const Record& r);

    /// claim the next available record.
    ///
    bool Claim(off64_t &offset/*OUT*/, Record &r /*OUT*/);
    
    /// unclaim a claimed record, does not update the record. This record
    /// will then be available for future 'claim' operations.
    ///
    void Unclaim(off64_t offset);

    /// complete and update a record
    ///
    void Complete(off64_t offset, const Record &r);

    /// read a record #
    ///
    void Read(uint64_t recnum, 
              Record &r /*OUT*/,
              unsigned int &status /*OUT*/);

    /// update a record, will not be marked complete
    ///
//    void Update(off64_t offset, Record &r);

    /// get the current progress on the batch.  If last progress time is older than
    /// update_if_older_than, it will be updated.
    void GetProgress(uint64_t& complete /*OUT*/,
                     uint64_t& total    /*OUT*/,
                     uint64_t& claimed  /*OUT*/,
                     time_t& last_progress_time /*OUT*/,
                     time_t update_if_older_than /*IN*/);
    
    // accessors
    inline FString GetFilename() const { return mFilename; }
    
private:
    // helpers
    void CheckOpen(void);
    void ReadHeader(header_t &header);
    void WriteHeader(const header_t &header);
protected:
    // data members
    FString mFilename;
    AutoFD mFD;
};

#include "ControlFile.cpp"

#endif

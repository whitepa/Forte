#include "Forte.h"
#include "ansi_test.h"

const int CLUSTER_LOCK_TEST_NUM_LOCKS = 1500;
const int CLUSTER_LOCK_TEST_NUM_LOOPS = 10;

// functions
void* test(void* p)
{
    FString err;

    while (1)
    {
        //printf(".");
        //fflush(stdout);
        for (int i=0; i<CLUSTER_LOCK_TEST_NUM_LOOPS; i++)
        {
            try
            {
                ClusterLock lock(FString(i), 5, err);
            }
            catch (EClusterLockUnvailable &e)
            {
                printf("Temp unavail(exiting): %s\n", e.what().c_str());
                return NULL;
            }
            catch (EClusterLockFile &e)
            {
                printf("file error: %s\n", e.what().c_str());
                return NULL;
            }
            catch (EClusterLock &e)
            {
                printf("lock error: %s\n", e.what().c_str());
            }
            catch (Exception &e)
            {
                printf("%s\n", e.what().c_str());
            }
        }	
    }

    return NULL;
}


// main
int main(int argc, char *argv[])
{
    bool all_pass;

    ClusterLock::LOCK_PATH = "/fsscale0/lock/";

    pthread_t threads[CLUSTER_LOCK_TEST_NUM_LOCKS];

    // get a bunch of cluster locks
    for (int i=0; i<CLUSTER_LOCK_TEST_NUM_LOCKS; i++)
    {
        pthread_create(&threads[i], NULL, test, NULL);	
    }

    test(NULL);

    // done
    return (all_pass ? 0 : 1);
}


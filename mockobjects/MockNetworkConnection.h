#ifndef _Replication_MockNetworkConnection_
#define _Replication_MockNetworkConnection_

#include "Forte.h"
#include "Exception.h"

namespace Forte
{
    EXCEPTION_CLASS(EMockNetworkConnection);
    EXCEPTION_SUBCLASS(EMockNetworkConnection, EUnableToCreateSocketPair);
    EXCEPTION_SUBCLASS(EMockNetworkConnection, EUnableToForkNetworkNode);
    EXCEPTION_SUBCLASS(EMockNetworkConnection, ENodeHasException);

/**
 * This class helps us create a two node mock network for unit testing
 * 
 */
    class MockNetworkConnection
    {
    public:
        /**
         * Mock network node class all mock network nodes should implement
         */
        class MockNetworkNode
        {
        public:
        
            MockNetworkNode() {};
            virtual ~MockNetworkNode() {};
        
            /**
             * Function which needs to be implemented by every network node
             *
             * @param fd This is the fd which is connected to the other
             *           node on the mock network
             */
            virtual void DoNetworkActivity(int fd) = 0;

            /**
             * Function used internally by MockNetwork
             */
            void runAndCloseFD(int fd, bool exit) 
            {
                try
                {
                    DoNetworkActivity(fd);
                    close(fd);
                }
                catch (...)
                {
                    close(fd);
                    throw;
                }

                if (exit)
                {
                    // this is the node whose state is 
                    // not saved
                    ::exit(0);
                }
            }
        };

        typedef boost::shared_ptr<MockNetworkNode> MockNetworkNodePtr;

        /**
         * Constructor. Takes the two network nodes for the mock network
         * 
         * @param nodeWhoseStateIsSaved This is the network node whose process state is saved
         *                              after network activity is done. So if you want to obtain
         *                              any saved data from the network activity. This node should
         *                              save it.
         * @param nodeWhoseStateIsLost This network node process state is lost. Any changes made in
         *                             this class will not be saved.
         */ 
        MockNetworkConnection(const MockNetworkNodePtr& nodeWhoseStateIsSaved,
                              const MockNetworkNodePtr& nodeWhoseStateIsLost);
 
        virtual ~MockNetworkConnection();

        /**
         * Function which runs the mock network. Connects the two network nodes. 
         *
         */
        void RunMockNetwork();

    protected:
        MockNetworkNodePtr mNodeWhoseStateIsSaved;
        MockNetworkNodePtr mNodeWhoseStateIsLost;
    };

};
#endif

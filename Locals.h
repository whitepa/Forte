#ifndef __Forte_Locals__
#define __Forte_Locals__

#include <map>
#include <boost/tuple/tuple.hpp>
#include <boost/preprocessor/repetition/enum_params_with_a_default.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/mpl/transform.hpp>
#include <boost/mpl/reverse_fold.hpp>
#include <boost/mpl/size.hpp>
#include "Exception.h"

namespace Forte
{
    EXCEPTION_CLASS(ELocals);
    EXCEPTION_SUBCLASS2(ELocals, EFailedToFindVariable,
                        "Failed to find variable");

    /**
     * The Locals class is used in conjunction with EnableStats. Use this class to indicate
     * the local member variables in the "Derived" class that are used as stat variables.
     * For an example on how to use this class look at ./unittest/EnableStatsUnitTest.cpp
     */
    template <
        typename Derived,
        BOOST_PP_ENUM_PARAMS_WITH_A_DEFAULT(
            10, typename T, boost::mpl::na)
        >
        struct Locals {

            /**
             * The mpl vector of all the local member variable types
             */
            typedef boost::mpl::vector<BOOST_PP_ENUM_PARAMS(10, T)> LocalTypes;

            template <typename X>
            struct ToTupleElementType {
                typedef X (Derived::* type);
            };

            /**
             * The mpl vector of pointers to all the local member variables
             * LocalType1 Derived::*,
             * LocalType2 Derived::*, ...
             */
            typedef typename boost::mpl::transform<
                LocalTypes,
                ToTupleElementType< boost::mpl::_ >
            >::type LocalTupleTypes;

            /**
             * The Tuple typedef for the different LocalTypes
             */
            typedef typename boost::mpl::reverse_fold<
                LocalTupleTypes,
                boost::tuples::null_type,
                boost::tuples::cons<
                    boost::mpl::_2,
                    boost::mpl::_1
                    >
                >::type TupleType;

            template <int index, typename TupleElementType>
            void AssignLocal(
                const Forte::FString& localName,
                TupleElementType tupleIndexElement) {

                mVarNames[index] = localName;
                boost::get<index>(mVars) = tupleIndexElement;
            }

            template <typename ReturnType, typename U>
                ReturnType GetLocal(
                    U* from,
                    const Forte::FString& name) {

                std::map<Forte::FString, ReturnType> allLocals =
                    GetAllLocals<ReturnType>(from);

                if (allLocals.find(name) != allLocals.end()) {
                    return allLocals[name];
                }

                boost::throw_exception(EFailedToFindVariable(name));
            }

            template <typename ReturnType, typename U>
                std::map<Forte::FString, ReturnType> GetAllLocals(U *from) {

                std::map<Forte::FString, ReturnType> result;

                foreach_tuple_element(
                    mVars,
                    SaveTupleElementInMap<
                        ReturnType,
                        U,
                        std::map<Forte::FString, ReturnType> >(
                            from,
                            std::vector<FString>(
                                mVarNames, mVarNames + GetNumberOfLocals()),
                            result)
                    );

                return result;
            };

            size_t GetNumberOfLocals() {
                return  boost::mpl::size<LocalTypes>::type::value; }

            bool LocalExists(const Forte::FString &name) {
                int size = GetNumberOfLocals();
                for (int i = 0; i < size; ++i)
                {
                    if (mVarNames[i] == name)
                        return true;
                }

                return false;
            }

        private:
            // functor which will save tuple elements in map
            template <typename ReturnType, typename U, typename MapType>
            struct SaveTupleElementInMap {
                SaveTupleElementInMap(
                    U *from, std::vector<FString> varNames, MapType &map)
                : mFrom (from),
                    mVarNames (varNames),
                    mMap (map) {}

                template <typename T>
                void operator() (T& t, const int index) {
                    mMap[mVarNames[index]] =
                         static_cast<ReturnType>(
                             static_cast<Derived*>(mFrom)->*t
                             );
                }

            private:
                U *mFrom;
                std::vector<FString> mVarNames;
                MapType &mMap;
            };

            // Iterate through tuple elements
            template<typename tuple_type, typename F, int Index, int Max>
                inline typename boost::enable_if_c<Index == Max, void>::type
                foreach_tuple_impl (tuple_type & t, F f) {

            }

            template<typename tuple_type, typename F, int Index, int Max>
                inline typename boost::enable_if_c<Index < Max, void>::type
                foreach_tuple_impl (tuple_type & t, F f) {

                f(boost::get<Index>(t), Index);
                foreach_tuple_impl<tuple_type, F, Index + 1, Max>(t, f);
            }

            template<typename tuple_type, typename F>
                void foreach_tuple_element(tuple_type & t, F f) {
                foreach_tuple_impl<
                    tuple_type,
                    F,
                    0,
                    boost::mpl::size<LocalTypes>::type::value
                    >(t, f);
            }

        private:
            TupleType mVars;
            FString mVarNames[boost::mpl::size<LocalTypes>::type::value];
        };
}

#endif
